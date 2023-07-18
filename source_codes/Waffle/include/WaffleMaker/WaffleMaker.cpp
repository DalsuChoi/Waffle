#include "WaffleMaker.h"
#include <Waffle.h>

WaffleMaker::WaffleMaker(double lr)
    : weight_decay(ADAM_WEIGHT_DECAY), per(MAX_MEMORY_SIZE, SMALL_NUMBER, PER_ALPHA, 0, 0)
{
    initialize_model(lr);
}

void WaffleMaker::insert_experience(torch::Tensor &state, torch::Tensor &action, torch::Tensor &final_reward)
{
    experience.emplace_back(state, action, final_reward);

    double priority = get_priority(1, state, action, final_reward).item<double>();
    per.add(priority, experience.size() - 1);
}

double WaffleMaker::update_model(int batch_size)
{
    if (experience.empty())
    {
        return -1;
    }

    auto state_batch = torch::empty({batch_size, 1, nCell_state_lat, nCell_state_lon});
    auto action_batch = torch::empty({batch_size, 1, NUM_WAFFLE_KNOBS});
    auto reward_batch = torch::empty({batch_size, 1});
    float ws[Waffle::batch_size];

    std::vector<int> idxs;
    std::vector<uint64> sampled_idx_experience;
    per.sample(batch_size, ws, idxs, sampled_idx_experience);
    for (int i = 0; i < batch_size; i++)
    {
        auto &e = experience[sampled_idx_experience[i]];
        state_batch[i][0] = e.state;
        action_batch[i] = e.action;
        reward_batch[i] = e.final_reward;
    }

    state_batch = state_batch.to(torch::kCUDA);
    action_batch = action_batch.to(torch::kCUDA);
    reward_batch = reward_batch.to(torch::kCUDA);

    auto f = model->forward(state_batch, action_batch);
    auto loss = torch::nn::MSELoss()(f, reward_batch);
    double result = loss.item<double>();
    std::cout << "loss: " << loss.item<double>() << std::endl;

    model_optimizer->zero_grad();
    loss.backward();
    model_optimizer->step();

    model->eval();
    {
        torch::NoGradGuard no_grad;
        auto f2 = model->forward(state_batch, action_batch);
        auto priority =
            torch::sqrt(torch::nn::MSELoss(torch::nn::MSELossOptions().reduction(torch::kNone))(f2, reward_batch));
        for (int i = 0; i < batch_size; i++)
        {
            int idx_sum_tree = idxs[i];
            per.update(idx_sum_tree, priority[i].item<double>());
        }
    }
    model->train();

    return result;
}

void WaffleMaker::save_model(const std::string &model_query_file_name)
{
    model->eval();
    torch::save(model, "model_" + model_query_file_name + ".pt");
    model->train();
}

void WaffleMaker::load_model(std::string name)
{
    torch::load(model, name);
    model->eval();
}

torch::Tensor WaffleMaker::get_priority(int batch_size, torch::Tensor state, torch::Tensor action, torch::Tensor reward)
{
    state = state.to(torch::kCUDA);
    action = action.to(torch::kCUDA);
    reward = reward.to(torch::kCUDA);

    state = state.reshape({batch_size, 1, nCell_state_lat, nCell_state_lon});
    action = action.reshape({batch_size, 1, NUM_WAFFLE_KNOBS});
    reward = reward.reshape({batch_size, 1});

    torch::Tensor priority;
    model->eval();
    {
        torch::NoGradGuard no_grad;
        auto f = model->forward(state, action);
        priority = torch::sqrt(torch::nn::MSELoss(torch::nn::MSELossOptions().reduction(torch::kNone))(f, reward));
        priority = priority.to(torch::kCPU);
    }
    model->train();

    return priority;
}

torch::Tensor WaffleMaker::exploitation(torch::Tensor state)
{
    auto state_batch = torch::empty({Waffle::recent, 1, nCell_state_lat, nCell_state_lon});
    torch::Tensor action_batch = (torch::empty({Waffle::recent, 1, NUM_WAFFLE_KNOBS}) * 2) - 1;
    for (int i = 0; i < Waffle::recent; i++)
    {
        state_batch[i][0] = state;
        action_batch[i] = recent_action[i];
    }

    state_batch = state_batch.to(torch::kCUDA);
    action_batch = action_batch.to(torch::kCUDA);

    torch::Tensor reward;
    model->eval();
    {
        torch::NoGradGuard no_grad;
        reward = model->forward(state_batch, action_batch);
    }
    model->train();

    const int _k = 1;
    reward = reward.reshape({Waffle::recent});
    auto topK = torch::topk(reward, _k);
    int idx_in_reward = std::get<1>(topK)[_k - 1].item<int>();

    action_batch = action_batch.to(torch::kCPU);
    return action_batch[idx_in_reward].clone().detach();
}

torch::Tensor WaffleMaker::exploration(torch::Tensor state, int batch_size, const double top)
{
    auto state_batch = torch::empty({batch_size, 1, nCell_state_lat, nCell_state_lon});
    torch::Tensor action_batch = (torch::rand({batch_size, 1, NUM_WAFFLE_KNOBS}) * 2) - 1;
    for (int i = 0; i < batch_size; i++)
    {
        state_batch[i][0] = state;
    }

    state_batch = state_batch.to(torch::kCUDA);
    action_batch = action_batch.to(torch::kCUDA);

    torch::Tensor reward;
    model->eval();
    {
        torch::NoGradGuard no_grad;
        reward = model->forward(state_batch, action_batch);
    }
    model->train();

    const int _k = std::max(static_cast<int>(batch_size * top / 100), 1);
    reward = reward.reshape({batch_size});
    auto topK = torch::topk(reward, _k, -1, true, false);
    auto &topK_reward = std::get<0>(topK);

    topK_reward = torch::clamp(topK_reward, 0.00001);
    int idx_in_topK_reward = torch::multinomial(topK_reward, 1).item<int>();
    int idx_in_reward = std::get<1>(topK)[idx_in_topK_reward].item<int>();

    action_batch = action_batch.to(torch::kCPU);
    return action_batch[idx_in_reward].clone().detach();
}

double WaffleMaker::get_reward(torch::Tensor state, torch::Tensor action)
{
    state = state.to(torch::kCUDA);
    action = action.to(torch::kCUDA);

    state = state.reshape({1, 1, nCell_state_lat, nCell_state_lon});
    action = action.reshape({1, 1, NUM_WAFFLE_KNOBS});

    torch::Tensor reward;
    model->eval();
    {
        torch::NoGradGuard no_grad;
        reward = model->forward(state, action);
    }
    model->train();

    return reward.item<double>();
}

void WaffleMaker::initialize_model(double _lr)
{
    std::cout << "WaffleMaker::initialize_model" << std::endl;

    lr = _lr;

    model = std::make_shared<Model>(Model());
    model_optimizer = std::make_shared<torch::optim::Adam>(
        torch::optim::Adam(model->parameters(), torch::optim::AdamOptions().lr(lr).weight_decay(weight_decay)));
    model->to(torch::kCUDA);
}