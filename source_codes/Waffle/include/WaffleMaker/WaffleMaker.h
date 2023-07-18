#ifndef WAFFLEMAKER_WAFFLEMAKER_H
#define WAFFLEMAKER_WAFFLEMAKER_H

#include "model.h"
#include "utils.h"

class WaffleMaker
{
public:
    WaffleMaker(double lr);

    torch::Tensor exploration(torch::Tensor state, int batch_size, double top);
    torch::Tensor exploitation(torch::Tensor state);
    double update_model(int batch_size);
    torch::Tensor get_priority(int batch_size, torch::Tensor state, torch::Tensor action, torch::Tensor reward);
    void insert_experience(torch::Tensor& state, torch::Tensor& action, torch::Tensor& final_reward);
    void save_model(const std::string& model_query_file_name);
    void load_model(std::string name);
    void initialize_model(double lr);
    double get_reward(torch::Tensor state, torch::Tensor action);

    std::vector<Experience> experience;
    std::deque<torch::Tensor> recent_action;

    PER per;
private:
    double lr;
    const double weight_decay;

    std::shared_ptr<Model> model;
    std::shared_ptr<torch::optim::Adam> model_optimizer;
};

#endif
