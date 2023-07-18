#include "InterfaceResource.h"
#include "Waffle.h"
#include "json.hpp"
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

using namespace nlohmann;

InterfaceResource::InterfaceResource()
{
    _resource = std::make_shared<Resource>();

    _resource->set_path("/{operation: "
                        "set_user_parameters|try|evaluate|best|convergence|statistics|knobs|"
                        "info}"
                        "/{p0: [0-1]\\.?[0-9]*}"
                        "/{p1: [0-1]\\.?[0-9]*}"
                        "/{p2: [0-9]*}");

    _resource->set_method_handler("GET", [&](const auto session) { get_handler(session); });
}

std::vector<std::string> InterfaceResource::get_path_parameters(const std::shared_ptr<Session> session) const
{
    std::vector<std::string> result;

    const auto &request = session->get_request();
    const auto operation = request->get_path_parameter("operation");
    result.push_back(operation);

    if (operation == "set_user_parameters")
    {
        auto _w_time = request->get_path_parameter("p0").c_str();
        result.emplace_back(_w_time);

        auto _w_memory = request->get_path_parameter("p1").c_str();
        result.emplace_back(_w_memory);

        auto _x = request->get_path_parameter("p2").c_str();
        result.emplace_back(_x);
    }

    return result;
}

void InterfaceResource::get_handler(const std::shared_ptr<Session> session)
{
    std::vector<std::string> parameters = get_path_parameters(session);

    if (parameters[0] == "set_user_parameters")
    {
        set_user_parameters(parameters);
    }
    else if (parameters[0] == "try")
    {
        std::string json_result = try_new_hyperparameters();
        session->close(OK, json_result, {{"Content-Length", std::to_string(json_result.size())}});
    }
    else if (parameters[0] == "evaluate")
    {
        evaluate_hyperparameters();
    }
    else if (parameters[0] == "best")
    {
        std::string json_result = best_hyperparameter();
        session->close(OK, json_result, {{"Content-Length", std::to_string(json_result.size())}});
    }
    else if (parameters[0] == "convergence")
    {
        convergence();
    }
    else if (parameters[0] == "statistics")
    {
        std::string json_result = statistics();
        session->close(OK, json_result, {{"Content-Length", std::to_string(json_result.size())}});
    }
    else if (parameters[0] == "knobs")
    {
        std::string json_result = current_knob_setting();
        session->close(OK, json_result, {{"Content-Length", std::to_string(json_result.size())}});
    }
    else if (parameters[0] == "info")
    {
        std::string json_result = additional_info();
        session->close(OK, json_result, {{"Content-Length", std::to_string(json_result.size())}});
    }
}

std::shared_ptr<Resource> InterfaceResource::get_resource() const
{
    return _resource;
}

void InterfaceResource::set_user_parameters(std::vector<std::string> &parameters)
{
    std::cout << std::endl;
    std::cout << "set_user_parameters" << std::endl;

    Waffle::w_time = std::stod(parameters[1]);
    Waffle::w_memory = std::stod(parameters[2]);
    assert(Waffle::w_time + Waffle::w_memory == 1);
    Waffle::x_for_regrid = std::stoi(parameters[3]);

    std::cout << "w_time: " << Waffle::w_time << std::endl;
    std::cout << "w_memory: " << Waffle::w_memory << std::endl;
    std::cout << "x_for_regrid: " << Waffle::x_for_regrid << std::endl;
}

void InterfaceResource::convergence()
{
    std::cout << std::endl;
    std::cout << "Change convergence" << std::endl;

    Waffle::CONVERGE = !Waffle::CONVERGE;
}

std::string InterfaceResource::statistics()
{
    std::unique_lock<std::mutex> lock(Waffle::m_update_interface_statistics);

    json jsonResult = {{"episode", std::to_string(Waffle::interface_episode)},
                       {"insertion", std::to_string(Waffle::interface_average_insert_time)},
                       {"deletion", std::to_string(Waffle::interface_average_delete_time)},
                       {"range", std::to_string(Waffle::interface_average_range_time)},
                       {"knn", std::to_string(Waffle::interface_average_knn_time)},
                       {"reward", std::to_string(Waffle::interface_average_reward)},
                       {"loss", std::to_string(Waffle::interface_average_loss)},
                       {"memory", std::to_string(Waffle::interface_average_memory)}};

    Waffle::interface_episode = 0;
    Waffle::interface_average_insert_time = 0;
    Waffle::interface_average_delete_time = 0;
    Waffle::interface_average_range_time = 0;
    Waffle::interface_average_knn_time = 0;
    Waffle::interface_average_reward = 0;
    Waffle::interface_average_loss = 0;
    Waffle::interface_average_memory = 0;

    lock.unlock();

    return jsonResult.dump();
}

std::string InterfaceResource::current_knob_setting()
{
    std::unique_lock<std::mutex> lock(Waffle::m_current_knob_setting);
    json jsonResult = {{"latSpace", std::to_string(Waffle::WaffleMaker_nCell_space_lat)},
                       {"lonSpace", std::to_string(Waffle::WaffleMaker_nCell_space_lon)},
                       {"maxObjectsPerCell", std::to_string(Waffle::WaffleMaker_MOPC)},
                       {"latChunk", std::to_string(Waffle::WaffleMaker_nCell_chunk_lat)},
                       {"lonChunk", std::to_string(Waffle::WaffleMaker_nCell_chunk_lon)}};

    lock.unlock();

    return jsonResult.dump();
}

std::string InterfaceResource::additional_info()
{
    json jsonResult = {{"objects", std::to_string(Waffle::current_object_num)},
                       {"queries", std::to_string(Waffle::total_processed_query)},
                       {"regrids", std::to_string(Waffle::total_regrids)}};

    return jsonResult.dump();
}

std::string InterfaceResource::try_new_hyperparameters()
{
    std::cout << std::endl;
    std::cout << "try_new_hyperparameters" << std::endl;

    if (Waffle::evaluate)
    {
        if (Waffle::evaluate_episode >= 1)
        {
            double reward = Waffle::evaluate_reward / Waffle::evaluate_episode;
            Waffle::tuning_results.emplace(reward, Waffle::lr, Waffle::batch_size, Waffle::candidates, Waffle::T,
                                           Waffle::recent);

            std::cout << std::endl << "Record the tuning result" << std::endl;
            std::cout << "Reward: " << reward << std::endl;
            std::cout << "Hyperparameter: {" << Waffle::lr << "," << Waffle::batch_size << "," << Waffle::candidates
                      << "," << Waffle::T << "," << Waffle::recent << "}" << std::endl
                      << std::endl;
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> rand_lr(hyper_lr[0], hyper_lr[1]);
    Waffle::lr = rand_lr(gen);

    std::uniform_int_distribution<int> rand_batch(0, hyper_batch.size() - 1);
    int idx_batch = rand_batch(gen);
    Waffle::batch_size = hyper_batch[idx_batch];

    std::uniform_int_distribution<int> rand_candidates(hyper_candidates[0], hyper_candidates[1]);
    Waffle::candidates = rand_candidates(gen);

    std::uniform_int_distribution<int> rand_T(hyper_T[0], hyper_T[1]);
    Waffle::T = rand_T(gen);

    std::uniform_int_distribution<int> rand_recent(hyper_recent[0], hyper_recent[1]);
    Waffle::recent = rand_recent(gen);

    if (Waffle::evaluate || Waffle::CONVERGE)
    {
        Waffle::try_new_hyperparameter = true;

        std::unique_lock<std::mutex> lock(Waffle::m_start_waffle);
        Waffle::CV_start_waffle.wait(lock);
    }

    Waffle::CONVERGE = false;
    Waffle::evaluate = false;

    std::cout << "Learning rate: " << Waffle::lr << std::endl;
    std::cout << "Batch size: " << Waffle::batch_size << std::endl;
    std::cout << "|candidates|: " << Waffle::candidates << std::endl;
    std::cout << "T: " << Waffle::T << std::endl;
    std::cout << "|recent|: " << Waffle::recent << std::endl;
    std::cout << std::endl;

    json jsonResult = {{"lr", std::to_string(Waffle::lr)},
                       {"batch_size", std::to_string(Waffle::batch_size)},
                       {"candidates", std::to_string(Waffle::candidates)},
                       {"T", std::to_string((int)Waffle::T)},
                       {"recent", std::to_string(Waffle::recent)}};

    Waffle::CV_start_waffle.notify_all();

    return jsonResult.dump();
}

void InterfaceResource::evaluate_hyperparameters()
{

    std::cout << std::endl;
    std::cout << "evaluate_hyperparameters" << std::endl;

    Waffle::CONVERGE = true;
    Waffle::evaluate = true;

    Waffle::evaluate_reward = 0;
    Waffle::evaluate_episode = 0;
}

std::string InterfaceResource::best_hyperparameter()
{
    std::cout << std::endl;
    std::cout << "best_hyperparameter" << std::endl;

    if (Waffle::evaluate_episode >= 1)
    {
        double reward = Waffle::evaluate_reward / Waffle::evaluate_episode;
        Waffle::tuning_results.emplace(reward, Waffle::lr, Waffle::batch_size, Waffle::candidates, Waffle::T,
                                       Waffle::recent);

        std::cout << std::endl << "Record the tuning result" << std::endl;
        std::cout << "Reward: " << reward << std::endl;
        std::cout << "Hyperparameter: {" << Waffle::lr << "," << Waffle::batch_size << "," << Waffle::candidates << ","
                  << Waffle::T << "," << Waffle::recent << "}" << std::endl
                  << std::endl;
    }

    const Hyperparameter &best = Waffle::tuning_results.top();
    Waffle::lr = best.lr;
    Waffle::batch_size = best.batch_size;
    Waffle::candidates = best.candidates;
    Waffle::T = best.T;
    Waffle::recent = best.recent;

    Waffle::try_new_hyperparameter = true;
    std::unique_lock<std::mutex> lock(Waffle::m_start_waffle);
    Waffle::CV_start_waffle.wait(lock);

    Waffle::CONVERGE = false;
    Waffle::evaluate = false;

    std::cout << "Learning rate: " << Waffle::lr << std::endl;
    std::cout << "Batch size: " << Waffle::batch_size << std::endl;
    std::cout << "|candidates|: " << Waffle::candidates << std::endl;
    std::cout << "T: " << Waffle::T << std::endl;
    std::cout << "|recent|: " << Waffle::recent << std::endl;

    json jsonResult = {{"lr", std::to_string(Waffle::lr)},
                       {"batch_size", std::to_string(Waffle::batch_size)},
                       {"candidates", std::to_string(Waffle::candidates)},
                       {"T", std::to_string((int)Waffle::T)},
                       {"recent", std::to_string(Waffle::recent)}};

    return jsonResult.dump();
}