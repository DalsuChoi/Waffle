#ifndef CHUNK_H
#define CHUNK_H

#include "LockManager.h"
#include "Parameters.h"
#include "utils.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

struct Object;
struct Chunk;

class Object
{
  public:
    Object() = default;
    Object(IDType ID, double lat, double lon);
    IDType get_ID() const;
    void set_ID(IDType id);
    double get_lat() const;
    double get_lon() const;

  private:
    IDType ID;
    double lat;
    double lon;
};

struct Chunk
{
  public:
    Chunk(int _CHUNK_SIZE_LATITUDE, int _CHUNK_SIZE_LONGITUDE, int _MAX_IN_CELL);
    ~Chunk();

    bool insertion(int cell_lat_in_chunk, int cell_lon_in_chunk, const Object &member);
    bool update_object(int object_position, const Object &object);
    bool delete_last_object(int cell_ID);
    std::pair<int, int> find_object(int cell_lat_in_chunk, int cell_lon_in_chunk, IDType object_ID) const;
    Object get_last_object(int cell_ID) const;
    const Object *get_objects() const;
    const int *get_num_objects() const;
    Chunk *get_next() const;
    void set_next(Chunk *next);
    Chunk *get_prev() const;
    void set_prev(Chunk *prev);
    void return_all_objects(std::vector<int> &result) const;
    void return_all_objects(std::vector<int> &result, int start_lat, int start_lon, int end_lat, int end_lon) const;
    void return_selected_objects(std::vector<int> &result, int new_cell_start_lat, int new_cell_end_lat,
                                 int new_cell_start_lon, int new_cell_end_lon, double start_lat, double start_lon,
                                 double end_lat, double end_lon) const;
    void return_all_objects_in_cell(
        double query_lat, double query_lon, int cell_lat_in_chunk, int cell_lon_in_chunk,
        std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> &q) const; // kNN
    bool is_empty_cell(int cell_ID);
    static unsigned get_memory_usage_of_one_chunk(int nCell_chunk_lat, int nCell_chunk_lon, int MOPC);

  private:
    int *num_objects;
    Object *objects;
    int num_total_objects = 0;
    Chunk *next;
    Chunk *prev;

    const int nCell_chunk_lat;
    const int nCell_chunk_lon;
    const int MOPC;
};

#endif