#include "model.h"
#include "Waffle.h"

Model::Model()
{
    assert(nCell_state_lat % 2 == 0);
    assert(nCell_state_lon % 2 == 0);

    network_initialization();
    weight_initialization();
    register_all_module();
}

torch::Tensor Model::forward(torch::Tensor state, torch::Tensor action)
{
    assert(state.sizes().size() == 4);
    assert(action.sizes().size() == 3);

    for (int i = 0; i < conv_layers; i++)
    {
        state = Conv2d[i](state);
        state = BatchNorm2d[i](state);
        state = torch::relu(state);
        state = MaxPool2d[i](state);
    }
    state = state.view({-1, fc_size[0]});

    action = action.view({-1, NUM_WAFFLE_KNOBS});
    action = fc_action(action);
    action = batchNorm_fc_action(action);
    action = torch::relu(action);
    action = torch::dropout(action, 0.5, is_training());

    auto concat = torch::cat({state, action}, 1);
    concat = fc2(concat);
    concat = batchNorm_fc2(concat);
    concat = torch::relu(concat);
    concat = torch::dropout(concat, 0.5, is_training());

    concat = fc3(concat);
    concat = batchNorm_fc3(concat);
    concat = torch::relu(concat);
    concat = torch::dropout(concat, 0.5, is_training());

    concat = fc4(concat);
    concat = batchNorm_fc4(concat);
    concat = torch::relu(concat);
    concat = torch::dropout(concat, 0.5, is_training());

    concat = fc5(concat);

    return concat;
}

void Model::register_all_module()
{
    for (int i = 0; i < conv_layers; i++)
    {
        std::string name = "cnn_" + std::to_string(i);
        register_module(name, Conv2d[i]);
        name = "batchNorm_cnn_" + std::to_string(i);
        register_module(name, BatchNorm2d[i]);
        name = "pool_cnn_" + std::to_string(i);
        register_module(name, MaxPool2d[i]);
    }
    register_module("fc_action", fc_action);
    register_module("batchNorm_fc_action", batchNorm_fc_action);
    register_module("fc2", fc2);
    register_module("batchNorm_fc2", batchNorm_fc2);
    register_module("fc3", fc3);
    register_module("batchNorm_fc3", batchNorm_fc3);
    register_module("fc4", fc4);
    register_module("batchNorm_fc4", batchNorm_fc4);
    register_module("fc5", fc5);
}

void Model::weight_initialization()
{
    for (int i = 0; i < conv_layers; i++)
    {
        Conv2d[i]->weight = torch::nn::init::kaiming_normal_(Conv2d[i]->weight);
    }
    fc_action->weight = torch::nn::init::kaiming_normal_(fc_action->weight);
    fc2->weight = torch::nn::init::kaiming_normal_(fc2->weight);
    fc3->weight = torch::nn::init::kaiming_normal_(fc3->weight);
    fc4->weight = torch::nn::init::kaiming_normal_(fc4->weight);
    fc5->weight = torch::nn::init::kaiming_normal_(fc5->weight);
}

void Model::network_initialization()
{
    int height = nCell_state_lat;
    int width = nCell_state_lon;
    for (int i = 0; i < conv_layers; i++)
    {
        Conv2d.emplace_back(torch::nn::Conv2dOptions(cnn_channel[i], cnn_channel[i + 1], cnn_kernel[i])
                                .stride(cnn_stride[i])
                                .padding(cnn_padding[i]));
        width = (width + cnn_padding[i] * 2 - cnn_kernel[i]) / cnn_stride[i] + 1;
        height = (height + cnn_padding[i] * 2 - cnn_kernel[i]) / cnn_stride[i] + 1;

        BatchNorm2d.emplace_back(torch::nn::BatchNorm2dOptions(cnn_channel[i + 1]));

        MaxPool2d.emplace_back(
            torch::nn::MaxPool2dOptions({pool_kernel[i], pool_kernel[i]}).stride({pool_stride[i], pool_stride[i]}));
        width = (width - pool_kernel[i]) / pool_stride[i] + 1;
        height = (height - pool_kernel[i]) / pool_stride[i] + 1;
    }
    fc_size[0] = cnn_channel.back() * width * height;

    fc_action = torch::nn::Linear(NUM_WAFFLE_KNOBS, action_extension);
    batchNorm_fc_action = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(action_extension));
    fc2 = torch::nn::Linear(fc_size[0] + action_extension, fc_size[1]);
    batchNorm_fc2 = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(fc_size[1]));
    fc3 = torch::nn::Linear(fc_size[1], fc_size[2]);
    batchNorm_fc3 = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(fc_size[2]));
    fc4 = torch::nn::Linear(fc_size[2], fc_size[3]);
    batchNorm_fc4 = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(fc_size[3]));
    fc5 = torch::nn::Linear(fc_size[3], fc_size[4]);
}