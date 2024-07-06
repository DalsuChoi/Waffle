#ifndef GRIDINDEXMANAGER_H
#define GRIDINDEXMANAGER_H

#include "Chunk.h"
#include <iostream>
#include <set>
#include <unordered_set>
#include <gtest/gtest_prod.h>

class WaffleIndexManager
{
  public:
    WaffleIndexManager(int _nCell_space_lat, int _nCell_space_lon, int _MOPC, int _nCell_chunk_lat, int _nCell_chunk_lon,
                     int role, uint64_t num_total_objects);
    ~WaffleIndexManager();
    void set_internal_parameters(double o_min_lat, double o_min_lon, double o_max_lat, double o_max_lon);
    bool insertion(const Object &object, std::vector<Lock> &current_locks, bool from_regrid);
    bool deletion(const Object &member, std::vector<Lock> &current_locks, bool from_regrid); // TODO Really need it?
    bool deletion(IDType ID, int cell_lat, int cell_lon, std::vector<Lock> &current_locks, bool from_regrid);
    bool range(double start_lat, double start_lon, double end_lat, double end_lon, std::vector<IDType> &result,
               std::vector<Lock> &current_locks) const;
    bool ring(int chunk_start_lat, int chunk_end_lat, int chunk_start_lon, int chunk_end_lon, int cell_start_lat,
              int cell_end_lat, int cell_start_lon, int cell_end_lon, double start_lat, double start_lon,
              double end_lat, double end_lon, std::vector<IDType> &result) const;
    bool kNN(int k, double query_lat, double query_lon, double start_real_lat, double start_real_lon,
             double end_real_lat, double end_real_lon, std::vector<IDType> &result,
             std::vector<Lock> &current_locks) const;

    int get_MOPC() const;
    int get_num_chunks_lat() const;
    int get_num_chunks_lon() const;
    double get_min_lat() const;
    double get_min_lon() const;
    double get_max_lat() const;
    double get_max_lon() const;
    const int get_nCell_chunk_lat() const;
    const int get_nCell_chunk_lon() const;
    uint64_t get_num_total_objects() const;
    LockManager *get_lock_manager();

    void set_role(int role);
    std::pair<int, int> calculate_cell_coordinate(double lat, double lon) const;

    std::atomic<unsigned> num_chunks;
    std::atomic<unsigned> memory_one_chunk;

    Chunk **chunks;

    CellCoordinate get_cell_coordinate(IDType ID) const;

  private:
    FRIEND_TEST(UnitTestWaffleIndexManager, WaffleIndexManager);
    FRIEND_TEST(UnitTestWaffleIndexManager, setInternalParameters);

    // The knobs of this Waffle index
    const int nCell_space_lat;
    const int nCell_space_lon;
    int MOPC;
    const int nCell_chunk_lat;
    const int nCell_chunk_lon;

    // A geographical space
    double min_lat, min_lon, max_lat, max_lon;

    // The difference between boundary latitude/longitude values in a single cell
    double cell_size_lat;
    double cell_size_lon;

    // The number of chunks along latitude/longitude
    int num_chunks_lat;
    int num_chunks_lon;

    // The maximum number of objects
    uint64_t num_total_objects;

    LockManager *lock_manager = nullptr;

    int role;

    double distance_from_query_to_level(double query_lat, double query_lon, int cell_lat, int cell_lon,
                                        int level) const;
    double distance_from_query_to_cell(double query_lat, double query_lon, int cell_query_lat, int cell_query_lon,
                                       int cell_target_lat, int cell_target_lon) const;

    void return_all_objects_in_cell(double query_lat, double query_lon, int cell_lat, int cell_lon,
                                    std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> &q) const;

    bool is_empty_cell(int cell_lat, int cell_lon) const;

    CellCoordinate* ID_cell;
};

#endif