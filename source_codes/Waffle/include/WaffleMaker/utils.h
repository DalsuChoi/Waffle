#ifndef WAFFLEMAKER_UTILS_H
#define WAFFLEMAKER_UTILS_H
#include "model.h"
#include <deque>
#include <iostream>
#include <random>
#include <torch/torch.h>

class Experience
{
  public:
    Experience() = default;
    Experience(torch::Tensor &state, torch::Tensor &action, torch::Tensor &final_reward);

    torch::Tensor state;
    torch::Tensor action;
    torch::Tensor final_reward;

    bool operator<(const Experience &right) const
    {
        if (this->final_reward.item<double>() < right.final_reward.item<double>())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

class TemporaryExperience
{
  public:
    TemporaryExperience(torch::Tensor &state, torch::Tensor &action, std::vector<double> rewards);

    torch::Tensor state;
    torch::Tensor action;
    std::vector<double> rewards;
};

class Sample_type
{
  public:
    Sample_type(int idx, double priority, uint64 idx_experience);
    int idx;
    double priority;
    uint64 idx_experience;
};

class SumTree
{
  public:
    SumTree(int capacity);
    double get_total_priority() const;
    void update_priority(int idx, double p);
    void add(double priority, uint64 idx_experience);
    Sample_type get(double s);
    int get_num_entries() const;

    std::vector<uint64> replay_buffer;
    int num_entries;

  private:
    int current;
    int capacity;
    std::vector<double> tree;

    void propagate(int idx, double change);
    int retrieve(int idx, double s) const;
};

// Prioritized Experience Replay
class PER
{
  public:
    PER(int capacity, double e, double a, double beta, double beta_increment_per_sampling);
    ~PER();
    size_t size() const;
    void add(double TD_error, uint64 idx_experience);
    void sample(int batch_size, float *ws, std::vector<int> &idxs, std::vector<uint64> &sampled_idx_experience);
    void update(int idx, double TD_error);

  private:
    double get_priority(double TD_error);

    double e;
    double a;
    double beta;
    double beta_increment_per_sampling;
    int capacity;
    SumTree *sum_tree;
    std::random_device rd;
    std::mt19937 gen;
};

#endif