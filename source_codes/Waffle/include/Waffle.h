#ifndef WAFFLE_H
#define WAFFLE_H

#include "Chunk.h"
#include "GridIndexManager.h"
#include "Parameters.h"
#include "TransactionManager.h"
#include "interface/InterfaceService.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <Parameters.h>
#include <WaffleMaker/WaffleMaker.h>
#include <WaffleMaker/utils.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <tbb/concurrent_hash_map.h>
#include <thread>
#include <torch/torch.h>

class Waffle
{
    friend class RegridManager;

  public:
    Waffle(std::string _query_file_path, uint64_t num_total_objects);

    virtual ~Waffle();

    static const std::atomic_bool &get_during_regrid();
    static void set_during_regrid(const bool &during_regrid);
    static const std::atomic_bool &get_prepare_regrid();
    static void set_prepare_regrid(const bool &prepare_regrid);

    static uint64 WaffleMaker_insert_time;
    static uint64 WaffleMaker_insert_num;
    static uint64 WaffleMaker_delete_time;
    static uint64 WaffleMaker_delete_num;
    static uint64 WaffleMaker_range_time;
    static uint64 WaffleMaker_range_num;
    static uint64 WaffleMaker_knn_time;
    static uint64 WaffleMaker_knn_num;

    static float *current_state;
    static double state_cell_size_lat;
    static double state_cell_size_lon;
    static std::atomic_bool WaffleMaker_writing_new_knob_setting;
    static std::atomic_bool WaffleMaker_wait_for_reward;

    static std::atomic_int num_running_query;

    static std::atomic_int client_object;
    static std::atomic_int regrid_object;

    static std::atomic_bool exit_experiment;
    static std::atomic_bool stop_regrid;
    static std::atomic<uint64> total_regrids;

    static uint64 user_insertion_time;
    static uint64 user_insertion_num;
    static uint64 user_deletion_time;
    static uint64 user_deletion_num;
    static uint64 user_range_time;
    static uint64 user_range_num;
    static uint64 user_knn_time;
    static uint64 user_knn_num;

    static uint64 num_total_objects;

    static uint64 interface_episode;
    static double interface_insert_time;
    static double interface_insert_num;
    static double interface_delete_time;
    static double interface_delete_num;
    static double interface_range_time;
    static double interface_range_num;
    static double interface_knn_time;
    static double interface_knn_num;
    static double interface_reward;
    static double interface_loss;
    static double interface_memory;
    static double interface_num_regrids;

    static double interface_average_insert_time;
    static double interface_average_delete_time;
    static double interface_average_range_time;
    static double interface_average_knn_time;
    static double interface_average_reward;
    static double interface_average_loss;
    static double interface_average_memory;

    static uint64 episode_insert_time;
    static uint64 episode_insert_num;
    static uint64 episode_delete_time;
    static uint64 episode_delete_num;
    static uint64 episode_range_time;
    static uint64 episode_range_num;
    static uint64 episode_knn_time;
    static uint64 episode_knn_num;
    static double episode_memory_usage;
    static uint64 episode_memory_count;
    std::atomic<double> episode_loss;
    std::atomic<double> episode_num_updates;

    void update_interface_statistics(double reward, double loss, double memory);
    static std::mutex m_update_interface_statistics;

    static std::mutex m_start_waffle;
    static std::condition_variable CV_start_waffle;

    static std::mutex m_current_knob_setting;

    static double w_time;
    static double w_memory;
    static int x_for_regrid;
    static int converge_regrids;
    static double lr;
    static int batch_size;
    static int candidates;
    static double T;
    static int recent;

    static std::atomic<bool> CONVERGE;

    static std::atomic<bool> evaluate;
    static std::atomic<double> evaluate_reward;
    static std::atomic<int> evaluate_episode;
    static std::priority_queue<Hyperparameter> tuning_results;
    static std::atomic<bool> try_new_hyperparameter;

    static std::atomic_bool during_regrid;
    static std::atomic_bool prepare_regrid;

    static int WaffleMaker_nCell_chunk_lat;
    static int WaffleMaker_nCell_chunk_lon;
    static int WaffleMaker_nCell_space_lat;
    static int WaffleMaker_nCell_space_lon;
    static int WaffleMaker_MOPC;

    static std::atomic<uint64> current_object_num;
    static std::atomic<uint64> total_processed_query;

    static std::mutex m_time;
    static std::chrono::time_point<std::chrono::high_resolution_clock> regrid_finish_time;

  private:
    GridIndexManager *original_index;
    GridIndexManager *new_index;

    TransactionManager transaction_manager;

    void client();

    double compute_final_reward(double original_insertion_reward, double original_deletion_reward,
                                double original_range_reward, double original_knn_reward,
                                double original_memory_reward);

    std::atomic_bool terminate_Waffle;
    const std::string query_file_path;

    std::thread client_thread;
    std::thread WaffleMaker_thread;

    void WaffleMaker_main();
    static std::mutex WaffleMaker_Mutex;
    static std::condition_variable WaffleMaker_CV;

    static std::mutex WaffleMaker_Mutex2;

    static std::mutex mutex_same_object;
    static std::condition_variable CV_same_object;

    std::atomic<uint64> current_episode;

    double entire_min_lon;
    double entire_min_lat;
    double entire_max_lon;
    double entire_max_lat;

    void split_line(std::string &one_line, std::vector<std::string> &split, char delimiter);

    void start_regrid();
    static std::mutex regrid_mutex;
    static std::condition_variable regrid_CV;
    int num_query_after_previous_regrid;

    void initialize_WaffleMaker_reward_variables();

    torch::Tensor action_lower_bound;
    torch::Tensor action_upper_bound;

    double get_insertion_reward(uint64 total_insert_time, uint64 num_insert);
    double get_deletion_reward(uint64 total_delete_time, uint64 num_delete);
    double get_range_reward(uint64 total_range_time, uint64 num_range);
    double get_knn_reward(uint64 total_knn_time, uint64 num_knn);
    double get_memory_reward(uint64 num_chunks, uint64 mChunk);

    double min_insertion_reward;
    double max_insertion_reward;
    double min_deletion_reward;
    double max_deletion_reward;
    double min_range_reward;
    double max_range_reward;
    double min_knn_reward;
    double max_knn_reward;
    double min_memory_reward;
    double max_memory_reward;

    int step;
    int random_steps;

    void insert_random_experiences(WaffleMaker *wm, std::vector<TemporaryExperience> &random_experiences);

    double get_estimated_memory_usage(const uint64 num_chunks, const uint64 mChunk);
    void initialize_episode_statistics();
};

#endif