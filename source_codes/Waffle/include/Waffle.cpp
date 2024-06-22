#include "Waffle.h"
#include "RegridManager.h"
#include "TransactionManager.h"
#include <Parameters.h>
#include <WaffleMaker/WaffleMaker.h>
#include <cassert>
#include <random>
#include <torch/torch.h>
#include <unistd.h>

std::atomic_bool Waffle::during_regrid;
std::atomic_bool Waffle::prepare_regrid;

int Waffle::WaffleMaker_nCell_chunk_lat = 0;
int Waffle::WaffleMaker_nCell_chunk_lon = 0;
int Waffle::WaffleMaker_nCell_space_lat = 0;
int Waffle::WaffleMaker_nCell_space_lon = 0;
int Waffle::WaffleMaker_MOPC = 0;

uint64 Waffle::WaffleMaker_insert_time;
uint64 Waffle::WaffleMaker_insert_num;
uint64 Waffle::WaffleMaker_delete_time;
uint64 Waffle::WaffleMaker_delete_num;
uint64 Waffle::WaffleMaker_range_time;
uint64 Waffle::WaffleMaker_range_num;
uint64 Waffle::WaffleMaker_knn_time;
uint64 Waffle::WaffleMaker_knn_num;

std::atomic_bool Waffle::WaffleMaker_writing_new_knob_setting;
std::atomic_bool Waffle::WaffleMaker_wait_for_reward;
std::atomic_int Waffle::num_running_query;
std::mutex Waffle::WaffleMaker_Mutex;
std::condition_variable Waffle::WaffleMaker_CV;
std::mutex Waffle::WaffleMaker_Mutex2;
std::mutex Waffle::mutex_same_object;
std::condition_variable Waffle::CV_same_object;
std::atomic_int Waffle::client_object;
std::atomic_int Waffle::regrid_object;

std::mutex Waffle::regrid_mutex;
std::condition_variable Waffle::regrid_CV;

std::atomic_bool Waffle::exit_experiment;
std::atomic_bool Waffle::stop_regrid;
std::atomic<uint64> Waffle::total_regrids;

uint64 Waffle::user_insertion_time;
uint64 Waffle::user_insertion_num;
uint64 Waffle::user_deletion_time;
uint64 Waffle::user_deletion_num;
uint64 Waffle::user_range_time;
uint64 Waffle::user_range_num;
uint64 Waffle::user_knn_time;
uint64 Waffle::user_knn_num;

uint64 Waffle::num_total_objects;

std::mutex Waffle::m_update_interface_statistics;
uint64 Waffle::interface_episode = 0;
double Waffle::interface_insert_time = 0;
double Waffle::interface_insert_num = 0;
double Waffle::interface_delete_time = 0;
double Waffle::interface_delete_num = 0;
double Waffle::interface_range_time = 0;
double Waffle::interface_range_num = 0;
double Waffle::interface_knn_time = 0;
double Waffle::interface_knn_num = 0;
double Waffle::interface_reward = 0;
double Waffle::interface_loss = 0;
double Waffle::interface_memory = 0;
double Waffle::interface_num_regrids = 0;

double Waffle::interface_average_insert_time = 0;
double Waffle::interface_average_delete_time = 0;
double Waffle::interface_average_range_time = 0;
double Waffle::interface_average_knn_time = 0;
double Waffle::interface_average_reward = 0;
double Waffle::interface_average_loss = 0;
double Waffle::interface_average_memory = 0;

std::mutex Waffle::m_start_waffle;
std::condition_variable Waffle::CV_start_waffle;

std::mutex Waffle::m_current_knob_setting;

double Waffle::w_time;
double Waffle::w_memory;
int Waffle::x_for_regrid;
int Waffle::converge_regrids;
double Waffle::lr;
int Waffle::batch_size;
int Waffle::candidates;
double Waffle::T;
int Waffle::recent;

std::atomic<bool> Waffle::CONVERGE;

std::atomic<bool> Waffle::evaluate;
std::atomic<double> Waffle::evaluate_reward;
std::atomic<int> Waffle::evaluate_episode;
std::priority_queue<Hyperparameter> Waffle::tuning_results;
std::atomic<bool> Waffle::try_new_hyperparameter;

std::atomic<uint64> Waffle::current_object_num;
std::atomic<uint64> Waffle::total_processed_query;

std::chrono::time_point<std::chrono::high_resolution_clock> Waffle::regrid_finish_time;
std::mutex Waffle::m_time;

uint64 Waffle::episode_insert_time = 0;
uint64 Waffle::episode_insert_num = 0;
uint64 Waffle::episode_delete_time = 0;
uint64 Waffle::episode_delete_num = 0;
uint64 Waffle::episode_range_time = 0;
uint64 Waffle::episode_range_num = 0;
uint64 Waffle::episode_knn_time = 0;
uint64 Waffle::episode_knn_num = 0;
double Waffle::episode_memory_usage = 0;
uint64 Waffle::episode_memory_count = 0;

Waffle::Waffle(std::string _query_file_path, uint64_t _num_total_objects)
    : terminate_Waffle(false), query_file_path(_query_file_path)
{
    assert(w_time + w_memory == 1);
    num_total_objects = _num_total_objects;
    Waffle::CONVERGE = false;
    Waffle::evaluate = false;
    Waffle::try_new_hyperparameter = false;

    step = 0;
    random_steps = *std::max_element(std::begin(hyper_batch), std::end(hyper_batch));

    Waffle::WaffleMaker_writing_new_knob_setting = true;
    Waffle::WaffleMaker_wait_for_reward = true;

    during_regrid = false;
    prepare_regrid = false;

    action_lower_bound = torch::empty({1, NUM_WAFFLE_KNOBS});
    action_upper_bound = torch::ones({1, NUM_WAFFLE_KNOBS});
    action_lower_bound[0][0].data() = (float)MIN_nCell_chunk_lat / (float)MAX_nCell_chunk_lat;
    action_lower_bound[0][1].data() = (float)MIN_nCell_chunk_lon / (float)MAX_nCell_chunk_lon;
    action_lower_bound[0][2].data() = (float)MIN_nCell_space_lat / (float)MAX_nCell_space_lat;
    action_lower_bound[0][3].data() = (float)MIN_nCell_space_lon / (float)MAX_nCell_space_lon;
    action_lower_bound[0][4].data() = (float)MIN_MOPC / (float)MAX_MOPC;

    num_query_after_previous_regrid = 0;
    current_episode = 0;

    user_insertion_time = 0;
    user_insertion_num = 0;
    user_deletion_time = 0;
    user_deletion_num = 0;
    user_range_time = 0;
    user_range_num = 0;
    user_knn_time = 0;
    user_knn_num = 0;

    Waffle::current_object_num = 0;

    Waffle::exit_experiment = false;
    Waffle::stop_regrid = false;
    Waffle::total_regrids = 0;

    Waffle::num_running_query = 0;
    Waffle::regrid_object = INTEGER_MIN;
    Waffle::client_object = INTEGER_MIN;
    initialize_WaffleMaker_reward_variables();

    initialize_episode_statistics();

    WaffleMaker_thread = std::thread{&Waffle::WaffleMaker_main, this};

    std::unique_lock<std::mutex> lock(Waffle::WaffleMaker_Mutex);
    while (Waffle::WaffleMaker_writing_new_knob_setting)
    {
        Waffle::WaffleMaker_CV.wait(lock);
    }

    original_index = new WaffleIndexManager(Waffle::WaffleMaker_nCell_space_lat, Waffle::WaffleMaker_nCell_space_lon,
                                          Waffle::WaffleMaker_MOPC, Waffle::WaffleMaker_nCell_chunk_lat, Waffle::WaffleMaker_nCell_chunk_lon,
                                          ORIGINAL_INDEX, Waffle::num_total_objects);
    Waffle::WaffleMaker_writing_new_knob_setting = true;

    new_index = nullptr;
    client_thread = std::thread{&Waffle::client, this};

    Waffle::min_insertion_reward = std::numeric_limits<double>::max();
    Waffle::max_insertion_reward = std::numeric_limits<double>::lowest();
    Waffle::min_deletion_reward = std::numeric_limits<double>::max();
    Waffle::max_deletion_reward = std::numeric_limits<double>::lowest();
    Waffle::min_range_reward = std::numeric_limits<double>::max();
    Waffle::max_range_reward = std::numeric_limits<double>::lowest();
    Waffle::min_knn_reward = std::numeric_limits<double>::max();
    Waffle::max_knn_reward = std::numeric_limits<double>::lowest();
    Waffle::min_memory_reward = std::numeric_limits<double>::max();
    Waffle::max_memory_reward = std::numeric_limits<double>::lowest();

    Waffle::regrid_finish_time = std::chrono::high_resolution_clock::now();
}

Waffle::~Waffle()
{
    client_thread.join();
    WaffleMaker_thread.join();
}

void Waffle::client()
{
    std::ifstream query_file(query_file_path);
    Waffle::total_processed_query = 0;

    while (true)
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::mutex> lock_time(Waffle::m_time);
        auto elapsed_time =
            std::chrono::duration_cast<std::chrono::seconds>(current_time - Waffle::regrid_finish_time).count();
        if (elapsed_time >= x_for_regrid)
        {
            Waffle::regrid_finish_time = current_time;
            start_regrid();
        }
        lock_time.unlock();

        Waffle::num_running_query++;
        if (Waffle::get_prepare_regrid())
        {
            Waffle::num_running_query--;
            continue;
        }

        if (Waffle::exit_experiment)
        {
            break;
        }

        std::string query;
        std::getline(query_file, query);
        std::vector<std::string> split_query;
        split_line(query, split_query, ' ');

        int query_type;
        if (split_query[0] == "TERMINATE")
        {
            query_type = QUERYTYPE_TERMINATE;
        }
        else if (split_query[0] == "INSERT")
        {
            query_type = QUERYTYPE_INSERTION;
        }
        else if (split_query[0] == "DELETE")
        {
            query_type = QUERYTYPE_DELETION;
        }
        else if (split_query[0] == "RANGE")
        {
            query_type = QUERYTYPE_RANGE;
        }
        else if (split_query[0] == "KNN")
        {
            query_type = QUERYTYPE_KNN;
        }
        else if (split_query[0] == "SETSPACE")
        {
            query_type = QUERYTYPE_SETSPACE;
        }
        else if (split_query[0] == "EPISODE")
        {
            current_episode++;
            Waffle::num_running_query--;

            double insertion_reward = get_insertion_reward(Waffle::episode_insert_time, Waffle::episode_insert_num);
            double deletion_reward = get_deletion_reward(Waffle::episode_delete_time, Waffle::episode_delete_num);
            double range_reward = get_range_reward(Waffle::episode_range_time, Waffle::episode_range_num);
            double knn_reward = get_knn_reward(Waffle::episode_knn_time, Waffle::episode_knn_num);
            double memory_reward = -1 * Waffle::episode_memory_usage / Waffle::episode_memory_count;
            double episode_reward =
                compute_final_reward(insertion_reward, deletion_reward, range_reward, knn_reward, memory_reward);

            std::unique_lock<std::mutex> lock(m_update_interface_statistics);
            if (step > random_steps)
            {
                interface_episode = current_episode;
                interface_average_insert_time = episode_insert_time / episode_insert_num;
                interface_average_delete_time = episode_delete_time / episode_delete_num;
                interface_average_range_time = episode_range_time / episode_range_num;
                interface_average_knn_time = episode_knn_time / episode_knn_num;
                interface_average_reward = episode_reward;
                Waffle::CONVERGE ? interface_average_loss = 0
                                 : interface_average_loss = episode_loss / episode_num_updates;
                interface_average_memory = Waffle::episode_memory_usage / Waffle::episode_memory_count;

                if (evaluate)
                {
                    evaluate_reward = evaluate_reward + episode_reward;
                    evaluate_episode++;
                }
            }
            Waffle::interface_insert_time = 0;
            Waffle::interface_insert_num = 0;
            Waffle::interface_delete_time = 0;
            Waffle::interface_delete_num = 0;
            Waffle::interface_range_time = 0;
            Waffle::interface_range_num = 0;
            Waffle::interface_knn_time = 0;
            Waffle::interface_knn_num = 0;
            Waffle::interface_reward = 0;
            Waffle::interface_loss = 0;
            Waffle::interface_memory = 0;
            Waffle::interface_num_regrids = 0;

            initialize_episode_statistics();

            lock.unlock();

            continue;
        }
        else if (split_query[0] == "TICK")
        {
            Waffle::num_running_query--;
            continue;
        }
        else
        {
            Waffle::num_running_query--;
            continue;
        }

        if (!during_regrid && (query_type == QUERYTYPE_INSERTION || query_type == QUERYTYPE_DELETION ||
                               query_type == QUERYTYPE_RANGE || query_type == QUERYTYPE_KNN))
        {
            num_query_after_previous_regrid++;
        }

        switch (query_type)
        {
        case QUERYTYPE_TERMINATE: {
            Waffle::terminate_Waffle = true;
            Waffle::WaffleMaker_CV.notify_all();

            Waffle::num_running_query--;
            while(Waffle::during_regrid){}
            Waffle::WaffleMaker_CV.notify_all();

            query_file.close();

            break;
        }
        case QUERYTYPE_INSERTION: {
            // query type, object ID, longitude, latitude
            int object_ID = std::stoi(split_query[1]);
            if (Waffle::during_regrid)
            { // To avoid processing the same object
                Waffle::client_object = object_ID;
                if (Waffle::client_object == Waffle::regrid_object)
                {
                    std::unique_lock<std::mutex> lock(Waffle::mutex_same_object);
                    Waffle::CV_same_object.wait(lock);
                }
            }

            double n_lon = std::stod(split_query[2]);
            double n_lat = std::stod(split_query[3]);

            transaction_manager.process_insertion_query(object_ID, n_lat, n_lon, original_index, new_index,Waffle::during_regrid);

            if (during_regrid)
            {
                Waffle::client_object = INTEGER_MIN;
            }
            break;
        }
        case QUERYTYPE_DELETION: {
            // query type, object ID
            int object_ID = std::stoi(split_query[1]);
            if (Waffle::during_regrid)
            { // To avoid processing the same object
                Waffle::client_object = object_ID;
                if (Waffle::client_object == Waffle::regrid_object)
                {
                    std::unique_lock<std::mutex> lock(Waffle::mutex_same_object);
                    Waffle::CV_same_object.wait(lock);
                }
            }

            transaction_manager.process_deletion_query(object_ID, original_index, new_index, Waffle::during_regrid);

            if (during_regrid)
            {
                Waffle::client_object = INTEGER_MIN;
            }
            break;
        }
        case QUERYTYPE_RANGE: {
            // query type, start longitude, start latitude, end longitude, end latitude
            double start_lon = std::stod(split_query[1]);
            double start_lat = std::stod(split_query[2]);
            double end_lon = std::stod(split_query[3]);
            double end_lat = std::stod(split_query[4]);

            std::vector<IDType> result;
            transaction_manager.process_range_query(start_lon, start_lat, end_lon, end_lat, result, original_index,
                                                    new_index, Waffle::during_regrid);
            break;
        }
        case QUERYTYPE_KNN: {
            // query type, start longitude, start latitude, end longitude, end latitude
            double start_lon = std::stod(split_query[1]);
            double start_lat = std::stod(split_query[2]);
            double end_lon = std::stod(split_query[3]);
            double end_lat = std::stod(split_query[4]);
            int k = std::stoi(split_query[5]);

            // A target coordinate is set to the center of the given range.
            double center_lat = (start_lat + end_lat) / 2;
            double center_lon = (start_lon + end_lon) / 2;

            std::vector<IDType> result;
            transaction_manager.process_knn_query(k, start_lon, start_lat, end_lon, end_lat, center_lat, center_lon,
                                                  result, original_index, new_index, during_regrid);
            break;
        }
        case QUERYTYPE_SETSPACE: {
            const double min_lon = std::stod(split_query[1]);
            const double min_lat = std::stod(split_query[2]);
            const double max_lon = std::stod(split_query[3]);
            const double max_lat = std::stod(split_query[4]);

            std::cout << "A geographical space: ([" << min_lat << "," << max_lat << "), "
                      << "[" << min_lon << "," << max_lon << "))" << std::endl;

            WaffleMaker::state.calculate_state_cell_size(min_lon, min_lat, max_lon, max_lat);

            original_index->set_internal_parameters(min_lat, min_lon, max_lat, max_lon);
            break;
        }
        default: {
            std::cerr << "INVALID QUERY TYPE" << std::endl;
            break;
        }
        }
        Waffle::num_running_query--;

        if (++Waffle::total_processed_query % 1000000 == 0)
        {
            std::cout << "\033[1;34m" << total_processed_query << " queries are processed."
                      << "\033[0m" << std::endl;
        }

        if (Waffle::total_processed_query % 100000 == 0)
        {
            episode_memory_usage += get_estimated_memory_usage(original_index->num_chunks, original_index->memory_one_chunk);
            if (during_regrid)
            {
                episode_memory_usage += get_estimated_memory_usage(new_index->num_chunks, new_index->memory_one_chunk);
            }
            episode_memory_count++;
        }
    }
}

const std::atomic_bool &Waffle::get_during_regrid()
{
    return during_regrid;
}

void Waffle::set_during_regrid(const bool &during_regrid)
{
    Waffle::during_regrid = during_regrid;
}

const std::atomic_bool &Waffle::get_prepare_regrid()
{
    return prepare_regrid;
}

void Waffle::set_prepare_regrid(const bool &prepare_regrid)
{
    Waffle::prepare_regrid = prepare_regrid;
}

void Waffle::WaffleMaker_main()
{
    WaffleMaker *wm = new WaffleMaker(Waffle::lr);

    torch::Tensor next_state = torch::from_blob(WaffleMaker::state.grid, {WaffleMaker::state.nCell_state_lat, WaffleMaker::state.nCell_state_lon});
    torch::Tensor state = next_state.clone().detach();

    double final_reward = DOUBLE_MAX;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> uniform_rand(0, 1);

    bool EXPLOITATION;

    std::vector<TemporaryExperience> random_experiences;

    while (true)
    {
        step++;

        torch::Tensor action;
        torch::Tensor original_action;

        if (step <= random_steps)
        {
            action = torch::rand({1, NUM_WAFFLE_KNOBS}) * 2 - 1;
            EXPLOITATION = false;
        }
        else
        {
            if (CONVERGE)
            {
                action = wm->exploitation(next_state);
                EXPLOITATION = true;
            }
            else
            {
                action = wm->exploration(next_state, Waffle::candidates, Waffle::T);
                EXPLOITATION = false;
            }
        }

        original_action = action.clone().detach();
        action = action * 0.5 + 0.5;
        action = action * (1 - action_lower_bound) + action_lower_bound;

        std::unique_lock<std::mutex> lock(Waffle::m_current_knob_setting);
        Waffle::WaffleMaker_nCell_chunk_lat =
            std::max((int)(action[0][0].item<double>() * MAX_nCell_chunk_lat), MIN_nCell_chunk_lat);
        Waffle::WaffleMaker_nCell_chunk_lon =
            std::max((int)(action[0][1].item<double>() * MAX_nCell_chunk_lon), MIN_nCell_chunk_lon);
        Waffle::WaffleMaker_nCell_space_lat =
            std::max((int)(action[0][2].item<double>() * MAX_nCell_space_lat), MIN_nCell_space_lat);
        Waffle::WaffleMaker_nCell_space_lon =
            std::max((int)(action[0][3].item<double>() * MAX_nCell_space_lon), MIN_nCell_space_lon);
        Waffle::WaffleMaker_MOPC = std::max((int)(action[0][4].item<double>() * MAX_MOPC), MIN_MOPC);
        lock.unlock();
        std::cout << "\033[1;32m"
                  << "WaffleMaker] A new knob setting: "
                  << "{" << Waffle::WaffleMaker_nCell_space_lat << "," << Waffle::WaffleMaker_nCell_space_lon << ","
                  << Waffle::WaffleMaker_MOPC << "," << Waffle::WaffleMaker_nCell_chunk_lat << ","
                  << Waffle::WaffleMaker_nCell_chunk_lon << "}"
                  << "\033[0m" << std::endl;
        Waffle::WaffleMaker_writing_new_knob_setting = false;

        std::unique_lock<std::mutex> lock_WaffleMaker_Mutex(Waffle::WaffleMaker_Mutex);
        lock_WaffleMaker_Mutex.unlock();
        Waffle::WaffleMaker_CV.notify_all();

        double loss = 0;
        if (!CONVERGE && step > random_steps)
        {
            loss = wm->update_model(Waffle::batch_size);

            episode_loss = episode_loss + loss;
            episode_num_updates = episode_num_updates + 1;
        }

        state = next_state.clone().detach();

        while (Waffle::WaffleMaker_wait_for_reward)
        {
            std::unique_lock<std::mutex> lock2(Waffle::WaffleMaker_Mutex2);
            std::unique_lock<std::mutex> lock(Waffle::WaffleMaker_Mutex);

            Waffle::WaffleMaker_CV.wait(lock);
            lock2.unlock();

            if (Waffle::try_new_hyperparameter)
            {
                delete wm;
                wm = new WaffleMaker(Waffle::lr);
                insert_random_experiences(wm, random_experiences);

                Waffle::CONVERGE = false;
                Waffle::evaluate = false;
                Waffle::try_new_hyperparameter = false;

                Waffle::CV_start_waffle.notify_all();
                break;
            }

            if (terminate_Waffle)
            {
                return;
            }

            if ((CONVERGE || evaluate) && step >= 2)
            {
                auto test_state = torch::from_blob(WaffleMaker::state.grid, {WaffleMaker::state.nCell_state_lat, WaffleMaker::state.nCell_state_lon},
                                                   torch::TensorOptions().dtype(torch::kFloat32))
                                      .clone()
                                      .detach();

                bool keep = false;
                auto candidate_action = wm->exploitation(test_state);

                if (torch::equal(original_action, candidate_action))
                {
                    keep = true;
                }

                std::cout << "\033[1;32m";
                keep ? std::cout << "Keep the current knob setting" : std::cout << "Change to the new knob setting";
                std::cout << std::endl;
                std::cout << "\033[0m";

                if (keep)
                { // candidate knob setting == original knob setting
                    Waffle::WaffleMaker_wait_for_reward = true;
                    Waffle::stop_regrid = true;

                    double insertion_reward =
                        get_insertion_reward(Waffle::WaffleMaker_insert_time, Waffle::WaffleMaker_insert_num);
                    double deletion_reward =
                        get_deletion_reward(Waffle::WaffleMaker_delete_time, Waffle::WaffleMaker_delete_num);
                    double range_reward =
                        get_range_reward(Waffle::WaffleMaker_range_time, Waffle::WaffleMaker_range_num);
                    double knn_reward = get_knn_reward(Waffle::WaffleMaker_knn_time, Waffle::WaffleMaker_knn_num);
                    double memory_reward = get_memory_reward(original_index->num_chunks, original_index->memory_one_chunk);
                    double final_reward = compute_final_reward(insertion_reward, deletion_reward, range_reward,
                                                               knn_reward, memory_reward);
                    update_interface_statistics(final_reward, 0, memory_reward * -1);

                    initialize_WaffleMaker_reward_variables();

                    lock.unlock();
                }
                else
                { // candidate knob setting != original knob setting
                    break;
                }
            }
        }
        if (terminate_Waffle)
        {
            return;
        }

        final_reward = DOUBLE_MAX;
        if (!CONVERGE && Waffle::WaffleMaker_insert_num != 0 && Waffle::WaffleMaker_delete_num != 0 &&
            Waffle::WaffleMaker_range_num != 0 && Waffle::WaffleMaker_knn_num != 0)
        {
            double insertion_reward =
                get_insertion_reward(Waffle::WaffleMaker_insert_time, Waffle::WaffleMaker_insert_num);
            double deletion_reward =
                get_deletion_reward(Waffle::WaffleMaker_delete_time, Waffle::WaffleMaker_delete_num);
            double range_reward = get_range_reward(Waffle::WaffleMaker_range_time, Waffle::WaffleMaker_range_num);
            double knn_reward = get_knn_reward(Waffle::WaffleMaker_knn_time, Waffle::WaffleMaker_knn_num);
            double memory_reward = get_memory_reward(original_index->num_chunks, original_index->memory_one_chunk);

            if (EXCLUDE_TOO_EARLY_EXPERIENCE < step && step <= random_steps)
            {
                // Before min/max rewards are fixed.
                std::vector<double> rewards = {insertion_reward, deletion_reward, range_reward, knn_reward,
                                               memory_reward};
                random_experiences.emplace_back(state, original_action, rewards);

                Waffle::min_insertion_reward = std::min((double)Waffle::min_insertion_reward, insertion_reward);
                Waffle::max_insertion_reward = std::max((double)Waffle::max_insertion_reward, insertion_reward);

                Waffle::min_deletion_reward = std::min((double)Waffle::min_deletion_reward, deletion_reward);
                Waffle::max_deletion_reward = std::max((double)Waffle::max_deletion_reward, deletion_reward);

                Waffle::min_range_reward = std::min((double)Waffle::min_range_reward, range_reward);
                Waffle::max_range_reward = std::max((double)Waffle::max_range_reward, range_reward);

                Waffle::min_knn_reward = std::min((double)Waffle::min_knn_reward, knn_reward);
                Waffle::max_knn_reward = std::max((double)Waffle::max_knn_reward, knn_reward);

                Waffle::min_memory_reward = std::min((double)Waffle::min_memory_reward, memory_reward);
                Waffle::max_memory_reward = std::max((double)Waffle::max_memory_reward, memory_reward);

                if (step == random_steps)
                {
                    std::cout << "The min/max rewards are determined." << std::endl;
                    std::cout << "min_insertion_reward: " << Waffle::min_insertion_reward
                              << ", max_insertion_reward: " << Waffle::max_insertion_reward << std::endl;
                    std::cout << "min_deletion_reward: " << Waffle::min_deletion_reward
                              << ", max_deletion_reward: " << Waffle::max_deletion_reward << std::endl;
                    std::cout << "min_range_reward: " << Waffle::min_range_reward
                              << ", max_range_reward: " << Waffle::max_range_reward << std::endl;
                    std::cout << "min_knn_reward: " << Waffle::min_knn_reward
                              << ", max_knn_reward: " << Waffle::max_knn_reward << std::endl;
                    std::cout << "min_memory_reward: " << Waffle::min_memory_reward
                              << ", max_memory_reward: " << Waffle::max_memory_reward << std::endl;

                    insert_random_experiences(wm, random_experiences);
                }
            }
            else
            { // step > random_steps
                final_reward =
                    compute_final_reward(insertion_reward, deletion_reward, range_reward, knn_reward, memory_reward);
                std::cout << "Reward: " << final_reward << std::endl;
                update_interface_statistics(final_reward, loss, memory_reward * -1);
            }
        }
        initialize_WaffleMaker_reward_variables();
        Waffle::WaffleMaker_wait_for_reward = true;

        next_state = torch::from_blob(WaffleMaker::state.grid, {WaffleMaker::state.nCell_state_lat, WaffleMaker::state.nCell_state_lon},
                                      torch::TensorOptions().dtype(torch::kFloat32));
        next_state = next_state.clone().detach();

#if defined(OUTPUT_STATE)
        std::ofstream of_state("state_" + std::to_string(step));
        int num_cells = WaffleMaker::state.nCell_state_lat * WaffleMaker::state.nCell_state_lon;
        for (int i = 0; i < num_cells; i++)
        {
            of_state << WaffleMaker::state.grid[i] << ",";
        }
        of_state.close();
#endif

        if (!CONVERGE && step > random_steps && final_reward != DOUBLE_MAX)
        {
            auto final_reward_tensor = torch::tensor({final_reward});
            wm->insert_experience(state, original_action, final_reward_tensor);
            if (!EXPLOITATION)
            {
                if (wm->recent_action.size() == Waffle::recent)
                {
                    wm->recent_action.pop_front();
                }
                wm->recent_action.push_back(original_action.clone().detach());
            }
        }
    }

    Waffle::exit_experiment = true;
    Waffle::WaffleMaker_CV.notify_all();
}

double Waffle::compute_final_reward(double original_insertion_reward, double original_deletion_reward,
                                    double original_range_reward, double original_knn_reward,
                                    double original_memory_reward)
{
    const double normalized_insertion_reward = ((original_insertion_reward - (double)Waffle::min_insertion_reward) /
                                                (double)(Waffle::max_insertion_reward - Waffle::min_insertion_reward));

    const double normalized_deletion_reward = ((original_deletion_reward - (double)Waffle::min_deletion_reward) /
                                               (double)(Waffle::max_deletion_reward - Waffle::min_deletion_reward));

    const double normalized_range_reward = ((original_range_reward - (double)Waffle::min_range_reward) /
                                            (double)(Waffle::max_range_reward - Waffle::min_range_reward));

    const double normalized_knn_reward = ((original_knn_reward - (double)Waffle::min_knn_reward) /
                                          (double)(Waffle::max_knn_reward - Waffle::min_knn_reward));

    const double normalized_memory_reward = ((original_memory_reward - (double)Waffle::min_memory_reward) /
                                             (double)(Waffle::max_memory_reward - Waffle::min_memory_reward));

    // Time reward
    double final_reward =
        (normalized_insertion_reward + normalized_deletion_reward + normalized_range_reward + normalized_knn_reward) *
        0.25 * w_time;
    // Memory reward
    final_reward += normalized_memory_reward * w_memory;

    return final_reward;
}

void Waffle::split_line(std::string &one_line, std::vector<std::string> &split, char delimiter)
{
    split.clear();

    std::stringstream ss(one_line);
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, delimiter);
        split.push_back(substr);
    }
}

void Waffle::initialize_WaffleMaker_reward_variables()
{
    Waffle::WaffleMaker_insert_time = 0;
    Waffle::WaffleMaker_insert_num = 0;
    Waffle::WaffleMaker_delete_time = 0;
    Waffle::WaffleMaker_delete_num = 0;
    Waffle::WaffleMaker_range_time = 0;
    Waffle::WaffleMaker_range_num = 0;
    Waffle::WaffleMaker_knn_time = 0;
    Waffle::WaffleMaker_knn_num = 0;

    num_query_after_previous_regrid = 0;
}

double Waffle::get_insertion_reward(uint64 total_insert_time, uint64 num_insert)
{
    return -static_cast<double>(total_insert_time) / ((double)num_insert);
}

double Waffle::get_deletion_reward(uint64 total_delete_time, uint64 num_delete)
{
    return -static_cast<double>(total_delete_time) / ((double)num_delete);
}

double Waffle::get_range_reward(uint64 total_range_time, uint64 num_range)
{
    return -static_cast<double>(total_range_time) / ((double)num_range);
}

double Waffle::get_knn_reward(uint64 total_knn_time, uint64 num_knn)
{
    return -static_cast<double>(total_knn_time) / ((double)num_knn);
}

double Waffle::get_memory_reward(const uint64 num_chunks, const uint64 memory_one_chunk)
{
    double _num_chunks = num_chunks;
    double _mChunk = memory_one_chunk;
    _mChunk *= (0.000001);
    double memory_reward = (_num_chunks * _mChunk);
    memory_reward *= -1;
    return memory_reward;
}

void Waffle::start_regrid()
{
    if (Waffle::get_prepare_regrid() || Waffle::get_during_regrid())
    {
        return;
    }

    Waffle::set_prepare_regrid(true);
    std::thread regrid{RegridManager(original_index, new_index)};
    regrid.detach();

    std::unique_lock<std::mutex> lock(Waffle::regrid_mutex);
    Waffle::regrid_CV.wait(lock);
}

void Waffle::update_interface_statistics(const double reward, const double loss, const double memory)
{
    std::unique_lock<std::mutex> lock(m_update_interface_statistics);

    Waffle::interface_insert_time += WaffleMaker_insert_time;
    Waffle::interface_insert_num += WaffleMaker_insert_num;
    Waffle::interface_delete_time += WaffleMaker_delete_time;
    Waffle::interface_delete_num += WaffleMaker_delete_num;
    Waffle::interface_range_time += WaffleMaker_range_time;
    Waffle::interface_range_num += WaffleMaker_range_num;
    Waffle::interface_knn_time += WaffleMaker_knn_time;
    Waffle::interface_knn_num += WaffleMaker_knn_num;
    Waffle::interface_reward += reward;
    Waffle::interface_loss += loss;
    Waffle::interface_memory += memory;
    Waffle::interface_num_regrids++;
}

void Waffle::insert_random_experiences(WaffleMaker *wm, std::vector<TemporaryExperience> &random_experiences)
{
    for (auto &experience : random_experiences)
    {
        double reward = compute_final_reward(experience.rewards[0], experience.rewards[1], experience.rewards[2],
                                             experience.rewards[3], experience.rewards[4]);
        auto reward_tensor = torch::tensor({reward});
        wm->insert_experience(experience.state, experience.action, reward_tensor);
    }
}

double Waffle::get_estimated_memory_usage(const uint64 num_chunks, const uint64 memory_one_chunk)
{
    double _num_chunks = num_chunks;
    double _mChunk = memory_one_chunk;
    _mChunk *= (0.000001);
    return (_num_chunks * _mChunk);
}

void Waffle::initialize_episode_statistics()
{
    episode_insert_time = 0;
    episode_insert_num = 0;
    episode_delete_time = 0;
    episode_delete_num = 0;
    episode_range_time = 0;
    episode_range_num = 0;
    episode_knn_time = 0;
    episode_knn_num = 0;
    episode_memory_usage = 0;
    episode_memory_count = 0;
    episode_loss = 0;
    episode_num_updates = 0;
}