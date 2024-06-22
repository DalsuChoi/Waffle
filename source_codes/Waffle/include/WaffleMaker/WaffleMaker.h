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

    struct State
    {
        float *grid;
        int nCell_state_lat;
        int nCell_state_lon;

        // The difference between boundary latitude/longitude values in a single state cell
        double state_cell_size_lat;
        double state_cell_size_lon;

        ~State();
        void initialize(const int nCell_state_lat, const int nCell_state_lon);
        void calculate_state_cell_size(const double min_lon, const double min_lat, const double max_lon, const double max_lat);
        void update_grid(double object_lat, double object_lon, double min_lat, double min_lon, bool increase);
    };
    static State state;

private:
    double lr;
    const double weight_decay;

    std::shared_ptr<Model> model;
    std::shared_ptr<torch::optim::Adam> model_optimizer;
};

#endif
