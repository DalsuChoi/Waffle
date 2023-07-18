#ifndef WAFFLEMAKER_MODEL_H
#define WAFFLEMAKER_MODEL_H

#include "Parameters.h"
#include <torch/torch.h>

class Model : public torch::nn::Module
{
  public:
    Model();
    torch::Tensor forward(torch::Tensor state, torch::Tensor action);

  private:
    int conv_layers = 4;
    std::vector<int> cnn_channel = {1, 16, 32, 64, 128};
    std::vector<int> cnn_kernel = {5, 3, 3, 3};
    std::vector<int> cnn_stride = {1, 1, 1, 1};
    std::vector<int> cnn_padding = {2, 1, 1, 1};
    std::vector<int> pool_kernel = {2, 2, 2, 2};
    std::vector<int> pool_stride = {2, 2, 2, 2};

    std::vector<int> fc_size = {INTEGER_MAX, 512, 256, 128, 1};
    int action_extension = 512;

    std::vector<torch::nn::Conv2d> Conv2d;
    std::vector<torch::nn::BatchNorm2d> BatchNorm2d;
    std::vector<torch::nn::MaxPool2d> MaxPool2d;

    torch::nn::Linear fc_action = nullptr;
    torch::nn::BatchNorm1d batchNorm_fc_action = nullptr;
    torch::nn::Linear fc2 = nullptr;
    torch::nn::BatchNorm1d batchNorm_fc2 = nullptr;
    torch::nn::Linear fc3 = nullptr;
    torch::nn::BatchNorm1d batchNorm_fc3 = nullptr;
    torch::nn::Linear fc4 = nullptr;
    torch::nn::BatchNorm1d batchNorm_fc4 = nullptr;
    torch::nn::Linear fc5 = nullptr;

    void network_initialization();
    void weight_initialization();
    void register_all_module();
};

#endif