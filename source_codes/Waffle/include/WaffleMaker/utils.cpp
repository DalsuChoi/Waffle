#include <Waffle.h>
#include <random>
#include <utility>
#include <utils.h>

#include "Parameters.h"
#include "utils.h"

Experience::Experience(torch::Tensor &state, torch::Tensor &action, torch::Tensor &final_reward)
{
    Experience::state = state.clone().detach();
    Experience::action = action.clone().detach();
    Experience::final_reward = final_reward.clone().detach();
}

SumTree::SumTree(int capacity)
{
    SumTree::current = 0;
    SumTree::capacity = capacity;
    SumTree::tree = std::vector<double>(2 * capacity - 1);
    SumTree::replay_buffer = std::vector<uint64>(capacity);
    SumTree::num_entries = 0;
}

void SumTree::propagate(int idx, double change)
{
    int parent = (idx - 1) / 2;
    tree[parent] += change;
    if (parent != 0)
    {
        propagate(parent, change);
    }
}

int SumTree::retrieve(int idx, double s) const
{
    int left_child = 2 * idx + 1;
    int right_child = left_child + 1;

    if (left_child >= tree.size())
    {
        return idx;
    }

    if (s <= tree[left_child])
    {
        return retrieve(left_child, s);
    }
    else
    {
        return retrieve(right_child, s - tree[left_child]);
    }
}

double SumTree::get_total_priority() const
{
    return tree[0];
}

void SumTree::update_priority(int tree_idx, double priority)
{
    double change = priority - tree[tree_idx];
    tree[tree_idx] = priority;
    propagate(tree_idx, change);
}

void SumTree::add(double priority, uint64 idx_experience)
{
    replay_buffer[current] = idx_experience;
    update_priority(current + capacity - 1, priority);

    current++;
    if (current >= capacity)
    {
        current = 0;
    }

    if (num_entries < capacity)
    {
        num_entries++;
    }
}

Sample_type SumTree::get(double s)
{
    int tree_idx = retrieve(0, s);
    int buffer_idx = tree_idx - capacity + 1;
    return Sample_type(tree_idx, tree[tree_idx], replay_buffer[buffer_idx]);
}

int SumTree::get_num_entries() const
{
    return num_entries;
}

Sample_type::Sample_type(int idx, double priority, uint64 idx_experience)
    : idx(idx), priority(priority), idx_experience(idx_experience)
{
}

PER::PER(int capacity, double e, double a, double beta, double beta_increment_per_sampling)
{
    sum_tree = new SumTree(capacity);
    PER::capacity = capacity;
    PER::e = e;
    PER::a = a;
    PER::beta = beta;
    PER::beta_increment_per_sampling = beta_increment_per_sampling;
    gen = std::mt19937(rd());
}

PER::~PER()
{
    delete sum_tree;
}

double PER::get_priority(double TD_error)
{
    return std::pow(std::abs(TD_error) + e, a);
}

void PER::add(double TD_error, uint64 idx_experience)
{
    double priority = get_priority(TD_error);
    sum_tree->add(priority, idx_experience);
}

void PER::sample(const int batch_size, float *ws, std::vector<int> &idxs, std::vector<uint64> &sampled_idx_experience)
{
    double segment = sum_tree->get_total_priority() / batch_size;

    beta = std::min(1.0, beta + beta_increment_per_sampling);

    double max_w = DOUBLE_MIN;
    for (int i = 0; i < batch_size; i++)
    {
        double low = segment * i;
        double high = segment * (i + 1);

        std::uniform_real_distribution<double> uniform_rand(low, high);
        double s = uniform_rand(gen);

        Sample_type sample = sum_tree->get(s);

        double sampling_probability = sample.priority / sum_tree->get_total_priority();
        double w = sampling_probability * sum_tree->get_num_entries();
        w = std::pow(w, -beta);
        ws[i] = (float)w;
        max_w = std::max(max_w, w);

        idxs.push_back(sample.idx);
        sampled_idx_experience.push_back(sample.idx_experience);
    }

    for (int i = 0; i < batch_size; i++)
    {
        ws[i] /= max_w;
    }
}

void PER::update(int tree_idx, double TD_error)
{
    double priority = get_priority(TD_error);
    sum_tree->update_priority(tree_idx, priority);
}

size_t PER::size() const
{
    return sum_tree->get_num_entries();
}

TemporaryExperience::TemporaryExperience(torch::Tensor &state, torch::Tensor &action, std::vector<double> rewards)
{
    TemporaryExperience::state = state.clone().detach();
    TemporaryExperience::action = action.clone().detach();
    TemporaryExperience::rewards = std::move(rewards);
}

Hyperparameter::Hyperparameter(double _reward, double _lr, int _batch_size, int _candidates, double _T, int _recent)
{
    reward = _reward;
    lr = _lr;
    batch_size = _batch_size;
    candidates = _candidates;
    T = _T;
    recent = _recent;
}