#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <limits>
#include <random>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "utils.h"
#include "parameters.h"
#include <Eigen/Dense>

std::vector<RealCoord> node;

std::vector<std::pair<int,int>> edge;
std::vector<std::vector<int>> graph;

std::vector<Object> objects;
uint64_t global_tick = 0;
std::vector<std::vector<std::vector<int>>> grid_nodes;
double min_lat_D = std::numeric_limits<double>::max();
double max_lat_D = std::numeric_limits<double>::lowest();
double min_lon_D = std::numeric_limits<double>::max();
double max_lon_D = std::numeric_limits<double>::lowest();

double range_size_lat_min;
double range_size_lat_max;
double range_size_lon_min;
double range_size_lon_max;
std::uniform_real_distribution<double> rand_lat;
std::uniform_real_distribution<double> rand_lon;

void prepare(std::string file_path, std::ofstream& of_query);
void initial_insertion(std::ofstream& of_query);
RealCoord decide_real_position_on_road(int start_idx, int end_idx);
int decide_next_destination_node(int previous_start_node, int new_start_node);
void one_episode(std::ofstream& of_query, const int ticks_per_episode);
void random_insertion(uint64_t target_id, std::string& query);
void insertion_into_center_of_space(uint64_t target_id, std::string& query);
void random_move_and_scan_queries(const int start_id, const int end_id, const int num_queries, std::ofstream& of_query);
void scan_query(int start_id, int end_id, std::ofstream& of_query);
void random_move(int id);
std::pair<int, int> cal_cell_coord(double lat, double lon);
void output_insertion_query(const int id, std::ofstream& of_query);

std::random_device rd_D;
std::mt19937 gen_D(rd_D());
std::uniform_int_distribution<uint64_t> rand_node;
uint64_t num_scan_query = 0;

int main(int argc, char const *argv[]) {

    if(argc != 4) {
        std::cerr << "1. The absolute path to the road dataset" << std::endl;
        std::cerr << "2. The output file name with the absolute path" << std::endl;
        std::cerr << "3. The number of episodes" << std::endl;
        return 1;
    }

    std::string output_file_path(argv[2]);
    std::ofstream of_query(output_file_path);

    std::string input_file_path(argv[1]);
    prepare(input_file_path, of_query);

    const uint64_t total_episode = std::stoull(std::string(argv[3]));

    initial_insertion(of_query);

    for(int episode=0; episode<total_episode; episode++) {
        one_episode(of_query, ticks_per_episode);
        std::cout << "Episode: " << episode << std::endl;
        of_query << "EPISODE " << episode << std::endl;
    }

    of_query << "TERMINATE" << std::endl;

    return 0;
}


void prepare(std::string file_path, std::ofstream& of_query)
{
    objects = std::vector<Object>(MAX_OBJECT);
    std::ifstream original_file(file_path);

    std::string one_line;
    std::getline(original_file, one_line);

    std::map<RealCoord, int> coord_idx;

    while(std::getline(original_file, one_line)) {
        std::stringstream ss(one_line);
        std::vector<std::string> splitQuery;
        while( ss.good() )
        {
            std::string substr;
            getline( ss, substr, ' ' );
            splitQuery.push_back(substr);
        }
        splitQuery[1] = splitQuery[1].substr(1, splitQuery[1].size()-1);
        double start_lon = stod(splitQuery[1]);

        splitQuery[2].pop_back();
        double start_lat = stod(splitQuery[2]);
        int start_node_idx;
        if(coord_idx.find({start_lat, start_lon}) == coord_idx.end()) {
            node.emplace_back(start_lat, start_lon);
            coord_idx[{start_lat, start_lon}] = node.size()-1;
            start_node_idx = node.size()-1;
        }
        else {
            start_node_idx = coord_idx[{start_lat, start_lon}];
        }

        double end_lon = stod(splitQuery[3]);
        splitQuery[4].pop_back();
        double end_lat = stod(splitQuery[4]);
        int end_node_idx;
        if(coord_idx.find({end_lat, end_lon}) == coord_idx.end()) {
            node.emplace_back(end_lat, end_lon);
            coord_idx[{end_lat, end_lon}] = node.size()-1;
            end_node_idx = node.size()-1;
        }
        else {
            end_node_idx = coord_idx[{end_lat, end_lon}];
        }

        if( !(start_lat == end_lat && start_lon == end_lon) ){
            edge.emplace_back(start_node_idx, end_node_idx);
        }
    }
    std::cout << "The number of nodes: " << node.size() << std::endl;

    sort( edge.begin(), edge.end() );
    edge.erase( unique( edge.begin(), edge.end() ), edge.end() );
    std::cout << "The number of edges: " << edge.size() << std::endl;

    for(auto one_node: node){
        min_lat_D = std::min(min_lat_D, one_node.lat);
        max_lat_D = std::max(max_lat_D, one_node.lat);
        min_lon_D = std::min(min_lon_D, one_node.lon);
        max_lon_D = std::max(max_lon_D, one_node.lon);
    }

    grid_nodes = std::vector<std::vector<std::vector<int>>>(1000, std::vector<std::vector<int>>(1000));
    for(int i=0; i<node.size(); i++) {
        const RealCoord& n = node[i];
        auto cell_coord = cal_cell_coord(n.lat, n.lon);
        grid_nodes[cell_coord.first][cell_coord.second].push_back(i);
    }

    double lat_length = (max_lat_D - min_lat_D);
    double lon_length = (max_lon_D - min_lon_D);
    range_size_lat_min = lat_length * (range_size_percent_min * 0.01) * 0.5;
    range_size_lon_min = lon_length * (range_size_percent_min * 0.01) * 0.5;
    range_size_lat_max = lat_length * (range_size_percent_max * 0.01) * 0.5;
    range_size_lon_max = lon_length * (range_size_percent_max * 0.01) * 0.5;
    rand_lat = std::uniform_real_distribution<double>(range_size_lat_min, range_size_lat_max);
    rand_lon = std::uniform_real_distribution<double>(range_size_lon_min, range_size_lon_max);

    graph = std::vector<std::vector<int>>(node.size());
    for(auto one_edge: edge){
        int start = one_edge.first;
        int end = one_edge.second;

        graph[start].push_back(end);
        graph[end].push_back(start);
    }

    for(std::vector<int>& e: graph){
        sort(e.begin(), e.end());
        e.erase( unique( e.begin(), e.end() ), e.end() );
    }

    rand_node = std::uniform_int_distribution<uint64_t>(0, node.size()-1);

    std::ostringstream ss;
    ss << std::setprecision(precision);
    ss << "SETSPACE " << min_lon_D << " " << min_lat_D << " " << max_lon_D << " " << max_lat_D;
    of_query << ss.str() << std::endl;
}

void initial_insertion(std::ofstream& of_query) {
    for(int id=0; id<MIN_OBJECT; id++) {
        std::string query;
        random_insertion(id, query);
        of_query << query << std::endl;
    }
}

void one_episode(std::ofstream& of_query, const int ticks_per_episode) {
    assert(ticks_per_episode % 4 == 0);
    int quarter_ticks = ticks_per_episode / 4;

    assert((MAX_OBJECT-MIN_OBJECT) % quarter_ticks == 0);
    int delta_center = CENTER_INSERTION / quarter_ticks;
    int delta_random = RANDOM_INSERTION / quarter_ticks;
    int delta = delta_center + delta_random;

    for(int tick=0; tick<ticks_per_episode; tick++) {
        if(tick < quarter_ticks) { // Minimum phase
            std::cout << "Minimum phase" << std::endl;
            random_move_and_scan_queries(0, MIN_OBJECT-1, query_per_tick, of_query);
        }
        else if(tick < quarter_ticks*2) { // Growing phase
            int _tick = tick - quarter_ticks;
            int start_id = MIN_OBJECT + delta * _tick;
            int end_id = MIN_OBJECT + delta * (_tick + 1);
            std::cout << "Object with ID [" << start_id << " ~ " << end_id - 1
                      << "] is inserted." << std::endl;
            int num_queries = 0;
            for (int id = start_id; id < end_id; id++) {
                if(id % range_or_knn_frequency == 0) {
                    scan_query(0, id-1, of_query);
                    num_queries++;
                }

                std::string query;
                (id < start_id+delta_center)? insertion_into_center_of_space(id, query): random_insertion(id, query);
                of_query << query << std::endl;
                num_queries++;
            }
            random_move_and_scan_queries(0, start_id - 1, query_per_tick-num_queries, of_query);
        }
        else if(tick < quarter_ticks*3) { // Maximum phase
            std::cout << "Maximum phase" << std::endl;
            random_move_and_scan_queries(0, MAX_OBJECT-1, query_per_tick, of_query);
        }
        else { // Shrinking phase
            int _tick = tick - quarter_ticks*3;
            int start_id = MAX_OBJECT - delta * (_tick + 1);
            int end_id = MAX_OBJECT - delta * _tick;
            std::cout << "Object with ID [" << start_id << " ~ " << end_id - 1 << "] is deleted."
                      << std::endl;
            int num_queries = 0;
            for (int id = start_id; id < end_id; id++) {
                if(id % range_or_knn_frequency == 0) {
                    scan_query(0, start_id-1, of_query);
                    num_queries++;
                }
                std::string query = "DELETE " + std::to_string(id);
                of_query << query << std::endl;
                objects[id].deleted = true;
                num_queries++;
            }
            random_move_and_scan_queries(0, start_id - 1, query_per_tick-num_queries, of_query);
        }

        global_tick++;
        std::cout << "Tick: " << global_tick << std::endl;
        of_query << "TICK " << global_tick << std::endl;
    }
}

void random_move_and_scan_queries(const int start_id, const int end_id, const int num_queries, std::ofstream& of_query) {
    std::uniform_int_distribution<int> rand(start_id, end_id);
    for(int i=0; i<num_queries; i++) {
        if(i % range_or_knn_frequency == 0) {
            scan_query(start_id, end_id, of_query);
        }
        else {
            int id = rand(gen_D);
            for(int j=0; j<random_move_trial; j++) {
                random_move(id);
            }
            objects[id].current = decide_real_position_on_road(objects[id].start_node_idx, objects[id].end_node_idx);
            output_insertion_query(id, of_query);
        }
    }
}

void scan_query(const int start_id, const int end_id, std::ofstream& of_query) {
    std::uniform_int_distribution<int> rand(start_id, end_id); // [a,b]
    int random_id = rand(gen_D);

    auto& target_object = objects[random_id];
    double center_lon = target_object.current.lon;
    double center_lat = target_object.current.lat;

    double range_size_lat;
    double range_size_lon;
    if(range_size_lat_min == range_size_lat_max && range_size_lon_min == range_size_lon_max){
        range_size_lat = range_size_lat_max;
        range_size_lon = range_size_lon_max;
    }
    else{
        range_size_lat = rand_lat(gen_D);
        range_size_lon = rand_lon(gen_D);
    }

    double start_lon = std::max(center_lon - range_size_lon, min_lon_D);
    double start_lat = std::max(center_lat - range_size_lat, min_lat_D);
    double end_lon = std::min(center_lon + range_size_lon, max_lon_D);
    double end_lat = std::min(center_lat + range_size_lat, max_lat_D);

    std::ostringstream ss;
    ss << std::setprecision(precision);
    bool true_range_false_knn;
    (num_scan_query++ % 2 == 0)? true_range_false_knn = true: true_range_false_knn = false;
    true_range_false_knn? ss << "RANGE ": ss << "KNN ";
    ss << start_lon << " " << start_lat << " " << end_lon << " " << end_lat;
    if(!true_range_false_knn) {
        ss << " " << k;
    }
    of_query << ss.str() << std::endl;
}

void random_move(int id)
{
    Object &o = objects[id];
    int previous_start_node = o.start_node_idx;
    int new_start_node = o.end_node_idx;
    int destination_node = decide_next_destination_node(previous_start_node, new_start_node);

    o.start_node_idx = new_start_node;
    o.end_node_idx = destination_node;
}

void output_insertion_query(const int id, std::ofstream& of_query) {

    std::ostringstream ss;
    ss << std::setprecision(precision);
    ss << "INSERT " << id << " "
       << objects[id].current.lon << " "
       << objects[id].current.lat;

    of_query << ss.str() << std::endl;
}

int decide_next_destination_node(const int previous_start_node, const int new_start_node)
{
    std::vector<int>& candidates = graph[new_start_node];
    if(candidates.size() == 1){
        return candidates[0];
    }

    std::uniform_int_distribution<int> rand_next(0, candidates.size() - 1);
    while(true){
        int i = rand_next(gen_D);
        if(candidates[i] != previous_start_node){
            return candidates[i];
        }
    }

    return -1;
}

void random_insertion(uint64_t target_id, std::string& query)
{
    Object& o = objects[target_id];

    int start_idx = rand_node(gen_D);
    int end_idx = decide_next_destination_node(-1, start_idx);

    o.start_node_idx = start_idx;
    o.end_node_idx = end_idx;
    o.current = decide_real_position_on_road(start_idx, end_idx);
    o.deleted = false;

    std::ostringstream ss;
    ss << std::setprecision(precision);
    ss << "INSERT " << target_id << " "
       << objects[target_id].current.lon << " "
       << objects[target_id].current.lat;

    query = ss.str();
}

RealCoord decide_real_position_on_road(int start_idx, int end_idx)
{
    const RealCoord& start = node[start_idx];
    const RealCoord& end = node[end_idx];

    double x1 = start.lat;
    double x2 = end.lat;
    if(x1 == x2){
        std::uniform_real_distribution<double> random(std::min(start.lon, end.lon),
                                                      std::max(start.lon, end.lon));
        double random_y = random(gen_D);
        return RealCoord(x1, random_y);
    }
    std::uniform_real_distribution<double> random(std::min(x1, x2), std::max(x1, x2));
    double random_x = random(gen_D);
    double slope = (end.lon - start.lon) / (end.lat - start.lat);
    double random_y = (slope * random_x) + (start.lon - slope * start.lat);

    return {random_x, random_y};
}

void insertion_into_center_of_space(uint64_t target_id, std::string& query)
{
    int start_idx;
    while(true) {
        Eigen::MatrixXd covar(2, 2);
        covar << 0.44, .1,
                .1, 0.44;

        normal_random_variable sample{covar};

        double sample_lat;
        double sample_lon;

        while (true) {
            Eigen::VectorXd a = sample();
            if (-4 <= a[0] && a[0] <= 4 &&
                -4 <= a[1] && a[1] <= 4) {
                sample_lat = a[0];
                sample_lon = a[1];
                break;
            }
        }

        sample_lat = std::min(min_lat_D + ((max_lat_D - min_lat_D) * (sample_lat + 4) / 8), max_lat_D);
        sample_lon = std::min(min_lon_D + ((max_lon_D - min_lon_D) * (sample_lon + 4) / 8), max_lon_D);

        auto cell_coord = cal_cell_coord(sample_lat, sample_lon);

        const std::vector<int> &candidates = grid_nodes[cell_coord.first][cell_coord.second];
        if(candidates.empty()) {
            continue;
        }
        double min_distance = std::numeric_limits<double>::max();

        for (int i: candidates) {
            const RealCoord& n = node[i];
            double distance = pow(n.lat - sample_lat, 2) + pow(n.lon - sample_lon, 2);
            if (distance < min_distance) {
                min_distance = distance;
                start_idx = i;
            }
        }
        break;
    }

    Object& o = objects[target_id];
    auto destination_node = decide_next_destination_node(-1, start_idx);

    o.start_node_idx = start_idx;
    o.end_node_idx = destination_node;
    o.current = decide_real_position_on_road(start_idx, destination_node);
    o.deleted = false;

    std::ostringstream ss;
    ss << std::setprecision(precision);
    ss << "INSERT " << target_id << " "
       << objects[target_id].current.lon << " "
       << objects[target_id].current.lat;

    query = ss.str();
}

std::pair<int, int> cal_cell_coord(double lat, double lon)
{
    int cell_lat = std::min((int)((lat - min_lat_D) / (max_lat_D - min_lat_D) * 1000), 999);
    int cell_lon = std::min((int)((lon - min_lon_D) / (max_lon_D - min_lon_D) * 1000), 999);

    return std::make_pair(cell_lat, cell_lon);
}