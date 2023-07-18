#ifndef GRIDINDEXMANAGER_H
#define GRIDINDEXMANAGER_H

#include "Chunk.h"
#include <iostream>
#include <set>
#include <tbb/concurrent_vector.h>
#include <unordered_set>

class GridIndexManager
{
  public:
    GridIndexManager(int _nCell_chunk_lat, int _nCell_chunk_lon, int _nCell_space_lat, int _nCell_space_lon, int _MOPC,
                     int role, uint64_t num_total_objects);
    ~GridIndexManager();
    void set_internal_parameters(double o_min_lat, double o_min_lon, double o_max_lat, double o_max_lon);
    bool insertion(const Object &object, std::vector<Lock> &current_locks, bool from_regrid = false);
    bool deletion(const Object &member, std::vector<Lock> &current_locks, bool from_regrid = false);
    bool deletion(ID_TYPE ID, int cell_lat, int cell_lon, std::vector<Lock> &current_locks, bool from_regrid = false);
    bool range(double start_lat, double start_lon, double end_lat, double end_lon, std::vector<ID_TYPE> &result,
               std::vector<Lock> &current_locks) const;
    bool ring(int chunk_start_lat, int chunk_end_lat, int chunk_start_lon, int chunk_end_lon, int cell_start_lat,
              int cell_end_lat, int cell_start_lon, int cell_end_lon, double start_lat, double start_lon,
              double end_lat, double end_lon, std::vector<ID_TYPE> &result) const;
    bool kNN(int k, double query_lat, double query_lon, double start_real_lat, double start_real_lon,
             double end_real_lat, double end_real_lon, std::vector<int> &result,
             std::vector<Lock> &current_locks) const;

    int get_MOPC() const;
    int get_num_chunks_lat() const;
    int get_num_chunks_lon() const;
    double getOMinLat() const;
    double getOMinLon() const;
    double getOMaxLat() const;
    double getOMaxLon() const;
    const int get_nCell_chunk_lat() const;
    const int get_nCell_chunk_lon() const;
    LockManager *get_lock_manager();

    void set_role(int role);
    std::pair<int, int> cal_cell_coord(double lat, double lon) const;

    std::atomic<unsigned> num_chunks;
    std::atomic<unsigned> mChunk;

    tbb::concurrent_vector<CELL_COORD> ID_cell;
    Chunk **total_chunks;

  private:
    const int nCell_chunk_lat;
    const int nCell_chunk_lon;
    const int nCell_space_lat;
    const int nCell_space_lon;
    int MOPC;

    double o_minLat, o_minLon, o_maxLat, o_maxLon;
    double real_one_cell_lat;
    double real_one_cell_lon;

    int num_chunks_lat;
    int num_chunks_lon;

    LockManager *lockManager = nullptr;

    int role;

    bool level_possible(int cell_query_lat, int cell_query_lon, int level, int cell_min_lat, int cell_min_lon,
                        int cell_max_lat, int cell_max_lon) const;

    double distance_from_query_to_level(double query_lat, double query_lon, int cell_lat, int cell_lon,
                                        int level) const;
    double distance_from_query_to_cell(double query_lat, double query_lon, int cell_query_lat, int cell_query_lon,
                                       int cell_target_lat, int cell_target_lon) const;

    void return_all_objects_in_cell(double query_lat, double query_lon, int cell_lat, int cell_lon,
                                    std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> &q) const;

    bool is_empty_cell(int cell_lat, int cell_lon) const;
};

#endif