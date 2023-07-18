#include "GridIndexManager.h"
#include "Parameters.h"
#include "Waffle.h"
#include <cassert>
#include <queue>

bool GridIndexManager::insertion(const Object &object, std::vector<Lock> &current_locks, bool from_regrid)
{
    auto PHASE_START = std::chrono::high_resolution_clock::now();

    auto cell = cal_cell_coord(object.get_lat(), object.get_lon());
    int cell_lat_in_space = cell.first;
    int cell_lon_in_space = cell.second;
    int chunk_lat = cell_lat_in_space / nCell_chunk_lat;
    int chunk_lon = cell_lon_in_space / nCell_chunk_lon;
    int cell_lat_in_chunk = cell_lat_in_space % nCell_chunk_lat;
    int cell_lon_in_chunk = cell_lon_in_space % nCell_chunk_lon;

    if (!(cell_lat_in_space >= 0 && cell_lat_in_space < nCell_space_lat && cell_lon_in_space >= 0 &&
          cell_lon_in_space < nCell_space_lon))
    {
        return false;
    }

    int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;
    int cell_ID = cell_lat_in_chunk * nCell_chunk_lon + cell_lon_in_chunk;

    bool have = false;
    for (auto &lock : current_locks)
    {
        if (lock.get_which_index() == role && lock.get_chunk_id() == chunk_ID && lock.get_lock_type() == EXCLUSIVE_LOCK)
        {
            have = true;
        }
    }
    if (!have)
    {
        lockManager->X_lock(chunk_ID);
        current_locks.emplace_back(role, chunk_ID, EXCLUSIVE_LOCK);
    }

    int cp_start = chunk_ID * 2;
    Chunk *&chunk_start = total_chunks[cp_start];
    Chunk *&chunk_end = total_chunks[cp_start + 1];
    bool new_chunk_added = false;
    if (chunk_start == nullptr)
    {
        chunk_start = new Chunk(nCell_chunk_lat, nCell_chunk_lon, MOPC);
        chunk_end = chunk_start;
        new_chunk_added = true;
    }

    Chunk *target_chunk = chunk_start;
    while (target_chunk != nullptr)
    {

        if (target_chunk->get_num_objects()[cell_ID] < MOPC)
        {
            break;
        }
        target_chunk = target_chunk->get_next();
    }
    if (target_chunk != nullptr)
    {
        target_chunk->insertion(cell_lat_in_chunk, cell_lon_in_chunk, object);
    }
    else
    {
        Chunk *old_end = chunk_end;

        auto *new_end = new Chunk(nCell_chunk_lat, nCell_chunk_lon, MOPC);
        new_chunk_added = true;

        chunk_end = new_end;
        new_end->set_prev(old_end);
        old_end->set_next(new_end);
        assert(old_end != chunk_end && chunk_start != chunk_end);

        bool success = chunk_end->insertion(cell_lat_in_chunk, cell_lon_in_chunk, object);
        assert(success);
    }

    auto PHASE_END = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(PHASE_END - PHASE_START).count();

    if (new_chunk_added)
    {
        num_chunks++;
    }

    if (!from_regrid)
    {
        int stateX = std::min((int)std::floor((object.get_lat() - getOMinLat()) / Waffle::state_cell_size_lat),
                              (int)nCell_state_lat - 1);
        int stateY = std::min((int)std::floor((object.get_lon() - getOMinLon()) / Waffle::state_cell_size_lon),
                              (int)nCell_state_lon - 1);
        Waffle::current_state[stateX * nCell_state_lon + stateY]++;
    }

    // Updating the data structure is excluded from the execution time, which is common to the compared methods.
    ID_cell[object.get_ID()] = {cell_lat_in_space, cell_lon_in_space};

    if (!from_regrid)
    { // A user query
        Waffle::user_insertion_num++;
        Waffle::user_insertion_time += query_time;

        Waffle::episode_insert_num++;
        Waffle::episode_insert_time += query_time;

        if (!Waffle::get_during_regrid())
        {
            Waffle::WaffleMaker_insert_num++;
            Waffle::WaffleMaker_insert_time += query_time;
        }
    }

    return true;
}

bool GridIndexManager::deletion(const Object &member, std::vector<Lock> &current_locks, bool from_regrid)
{
    auto cell = cal_cell_coord(member.get_lat(), member.get_lon());
    int cell_lat_in_space = cell.first;
    int cell_lon_in_space = cell.second;
    return deletion(member.get_ID(), cell_lat_in_space, cell_lon_in_space, current_locks, from_regrid);
}

GridIndexManager::GridIndexManager(int _nCell_chunk_lat, int _nCell_chunk_lon, int _nCell_space_lat,
                                   int _nCell_space_lon, int _MOPC, int role, uint64_t num_total_objects)
    : nCell_chunk_lat(_nCell_chunk_lat), nCell_chunk_lon(_nCell_chunk_lon), nCell_space_lat(_nCell_space_lat),
      nCell_space_lon(_nCell_space_lon), MOPC(_MOPC), role(role), ID_cell(num_total_objects, {INTEGER_MAX, INTEGER_MAX})
{
    total_chunks = nullptr;
    lockManager = nullptr;
    num_chunks = 0;
    mChunk =
        (sizeof(int) * nCell_chunk_lat * nCell_chunk_lon + (sizeof(Object)) * nCell_chunk_lat * nCell_chunk_lon * MOPC);
}

GridIndexManager::~GridIndexManager()
{
    int unique_chunk_positions = num_chunks_lat * num_chunks_lon;
    for (int chunk_position_1D = 0; chunk_position_1D < unique_chunk_positions; chunk_position_1D++)
    {
        int cp_start = chunk_position_1D * 2;
        Chunk *&chunk_start = total_chunks[cp_start];
        if (chunk_start == nullptr)
        {
            continue;
        }

        for (Chunk *chunk = chunk_start; chunk != nullptr;)
        {
            Chunk *temp = chunk;
            chunk = chunk->get_next();
            delete temp;
        }
    }
    delete[] total_chunks;
    delete lockManager;
}

void GridIndexManager::set_internal_parameters(double o_min_lat, double o_min_lon, double o_max_lat, double o_max_lon)
{
    GridIndexManager::o_minLat = o_min_lat;
    GridIndexManager::o_minLon = o_min_lon;
    GridIndexManager::o_maxLat = o_max_lat;
    GridIndexManager::o_maxLon = o_max_lon;
    GridIndexManager::real_one_cell_lat = (o_max_lat - o_min_lat) / nCell_space_lat;
    GridIndexManager::real_one_cell_lon = (o_max_lon - o_min_lon) / nCell_space_lon;
    GridIndexManager::num_chunks_lat = ceil(static_cast<double>(nCell_space_lat) / nCell_chunk_lat);
    GridIndexManager::num_chunks_lon = ceil(static_cast<double>(nCell_space_lon) / nCell_chunk_lon);

    int unique_chunk_positions = num_chunks_lat * num_chunks_lon;
    total_chunks = new Chunk *[unique_chunk_positions * 2];
    std::fill_n(total_chunks, unique_chunk_positions * 2, nullptr);

    lockManager = new LockManager(num_chunks_lat, num_chunks_lon);

    Waffle::state_cell_size_lat = (o_max_lat - o_min_lat) / nCell_state_lat;
    Waffle::state_cell_size_lon = (o_max_lon - o_min_lon) / nCell_state_lon;
}

int GridIndexManager::get_MOPC() const
{
    return MOPC;
}

int GridIndexManager::get_num_chunks_lat() const
{
    return num_chunks_lat;
}

int GridIndexManager::get_num_chunks_lon() const
{
    return num_chunks_lon;
}

const int GridIndexManager::get_nCell_chunk_lat() const
{
    return nCell_chunk_lat;
}

const int GridIndexManager::get_nCell_chunk_lon() const
{
    return nCell_chunk_lon;
}

double GridIndexManager::getOMinLat() const
{
    return o_minLat;
}

double GridIndexManager::getOMinLon() const
{
    return o_minLon;
}

double GridIndexManager::getOMaxLat() const
{
    return o_maxLat;
}

double GridIndexManager::getOMaxLon() const
{
    return o_maxLon;
}

LockManager *GridIndexManager::get_lock_manager()
{
    return lockManager;
}

void GridIndexManager::set_role(int role)
{
    GridIndexManager::role = role;
}

std::pair<int, int> GridIndexManager::cal_cell_coord(double lat, double lon) const
{
    int cell_lat = std::min((int)((lat - o_minLat) / real_one_cell_lat), nCell_space_lat - 1);
    int cell_lon = std::min((int)((lon - o_minLon) / real_one_cell_lon), nCell_space_lon - 1);

    return std::make_pair(cell_lat, cell_lon);
}

bool GridIndexManager::range(double start_lat, double start_lon, double end_lat, double end_lon,
                             std::vector<int> &result, std::vector<Lock> &current_locks) const
{
    auto PHASE_START = std::chrono::high_resolution_clock::now();

    auto cell_start = cal_cell_coord(start_lat, start_lon);
    int cell_startLat = std::max(cell_start.first, 0);
    int cell_startLon = std::max(cell_start.second, 0);

    auto cell_end = cal_cell_coord(end_lat, end_lon);
    int cell_endLat = std::min(cell_end.first, nCell_space_lat - 1);
    int cell_endLon = std::min(cell_end.second, nCell_space_lon - 1);

    if ((cell_startLat > cell_endLat) || (cell_startLon > cell_endLon))
    {
        return false;
    }

    int chunk_start_lat = cell_startLat / nCell_chunk_lat;
    int chunk_end_lat = cell_endLat / nCell_chunk_lat;

    int chunk_start_lon = cell_startLon / nCell_chunk_lon;
    int chunk_end_lon = cell_endLon / nCell_chunk_lon;

    for (int chunk_lat = chunk_start_lat; chunk_lat <= chunk_end_lat; chunk_lat++)
    {
        int start = chunk_lat * num_chunks_lon + chunk_start_lon;
        int end = start + chunk_end_lon - chunk_start_lon;
        for (int chunk_ID = start; chunk_ID <= end; chunk_ID++)
        {
            lockManager->S_lock(chunk_ID);
            current_locks.emplace_back(role, chunk_ID, SHARED_LOCK);
        }
    }

    for (int chunk_lat = chunk_start_lat + 1; chunk_lat <= chunk_end_lat - 1; chunk_lat++)
    {
        for (int chunk_lon = chunk_start_lon + 1; chunk_lon <= chunk_end_lon - 1; chunk_lon++)
        {
            int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;
            int cp_start = chunk_ID * 2;
            Chunk *&chunk_start = total_chunks[cp_start];
            Chunk *&chunk_end = total_chunks[cp_start + 1];
            if (chunk_start == nullptr)
            {
                continue;
            }

            for (Chunk *cp = chunk_start; cp != nullptr; cp = cp->get_next())
            {
                cp->return_all_objects(result);
            }
        }
    }

    ring(chunk_start_lat, chunk_end_lat, chunk_start_lon, chunk_end_lon, cell_startLat, cell_endLat, cell_startLon,
         cell_endLon, start_lat, start_lon, end_lat, end_lon, result);

    auto PHASE_END = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(PHASE_END - PHASE_START).count();

    if (!Waffle::get_during_regrid())
    {
        Waffle::WaffleMaker_range_time += query_time;
        Waffle::WaffleMaker_range_num++;
    }

    if (!Waffle::get_during_regrid())
    {
        Waffle::user_range_num++;
        Waffle::episode_range_num++;
    }
    else
    {
        if (role == NEW_INDEX)
        {
            Waffle::user_range_num++;
            Waffle::episode_range_num++;
        }
    }
    Waffle::user_range_time += query_time;
    Waffle::episode_range_time += query_time;

    return true;
}

bool GridIndexManager::ring(int chunk_start_lat, int chunk_end_lat, int chunk_start_lon, int chunk_end_lon,
                            int cell_start_lat, int cell_end_lat, int cell_start_lon, int cell_end_lon,
                            double start_lat, double start_lon, double end_lat, double end_lon,
                            std::vector<ID_TYPE> &result) const
{
    int walk_lat[4] = {0, 1, 0, -1};
    int walk_lon[4] = {1, 0, -1, 0};
    int direction = 0;

    int chunk_lat = chunk_start_lat;
    int chunk_lon = chunk_start_lon;

    while (true)
    {
        int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;
        int cp_start = chunk_ID * 2;
        Chunk *&chunk_start = total_chunks[cp_start];
        Chunk *&chunk_end = total_chunks[cp_start + 1];

        if (chunk_start != nullptr)
        {
            int new_cell_start_lat = std::max(cell_start_lat - chunk_lat * nCell_chunk_lat, 0);
            int new_cell_end_lat = std::min(cell_end_lat - chunk_lat * nCell_chunk_lat, nCell_chunk_lat - 1);
            int new_cell_start_lon = std::max(cell_start_lon - chunk_lon * nCell_chunk_lon, 0);
            int new_cell_end_lon = std::min(cell_end_lon - chunk_lon * nCell_chunk_lon, nCell_chunk_lon - 1);

            for (Chunk *cp = chunk_start; cp != nullptr; cp = cp->get_next())
            {
                cp->return_selected_objects(result, new_cell_start_lat, new_cell_end_lat, new_cell_start_lon,
                                            new_cell_end_lon, start_lat, start_lon, end_lat, end_lon);
            }
        }

        int trial = 0;
        while (true)
        {
            chunk_lat += walk_lat[direction];
            chunk_lon += walk_lon[direction];
            trial++;

            if (chunk_lat == chunk_start_lat && chunk_lon == chunk_start_lon)
            {
                return true;
            }

            if (chunk_lat < chunk_start_lat || chunk_lat > chunk_end_lat || chunk_lon < chunk_start_lon ||
                chunk_lon > chunk_end_lon)
            {
                if (trial == 2)
                {
                    return true;
                }

                chunk_lat -= walk_lat[direction];
                chunk_lon -= walk_lon[direction];

                direction++;
                if (direction == 4)
                {
                    return true;
                }
            }
            else
            {
                break;
            }
        }
    }
}

bool GridIndexManager::kNN(const int k, const double query_lat, const double query_lon, const double start_real_lat,
                           const double start_real_lon, const double end_real_lat, const double end_real_lon,
                           std::vector<ID_TYPE> &result, std::vector<Lock> &current_locks) const
{
    auto PHASE_START = std::chrono::high_resolution_clock::now();

    auto cell_query = cal_cell_coord(query_lat, query_lon);
    const int cell_query_lat = std::max(cell_query.first, 0);
    const int cell_query_lon = std::max(cell_query.second, 0);

    auto cell_start = cal_cell_coord(start_real_lat, start_real_lon);
    const int cell_start_lat = std::max(cell_start.first, 0);
    const int cell_start_lon = std::max(cell_start.second, 0);

    auto cell_end = cal_cell_coord(end_real_lat, end_real_lon);
    const int cell_end_lat = cell_end.first;
    const int cell_end_lon = cell_end.second;

    const int chunk_start_lat = cell_start_lat / nCell_chunk_lat;
    const int chunk_start_lon = cell_start_lon / nCell_chunk_lon;

    const int chunk_end_lat = cell_end_lat / nCell_chunk_lat;
    const int chunk_end_lon = cell_end_lon / nCell_chunk_lon;

    for (int chunk_lat = chunk_start_lat; chunk_lat <= chunk_end_lat; chunk_lat++)
    {
        int start = chunk_lat * num_chunks_lon + chunk_start_lon;
        int end = start + chunk_end_lon - chunk_start_lon;
        for (int chunk_ID = start; chunk_ID <= end; chunk_ID++)
        {
            lockManager->S_lock(chunk_ID);

            current_locks.emplace_back(role, chunk_ID, SHARED_LOCK);
        }
    }

    std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> q;

    if (!is_empty_cell(cell_query_lat, cell_query_lon))
    {
        return_all_objects_in_cell(query_lat, query_lon, cell_query_lat, cell_query_lon, q);
    }

    int level = 1;
    if (level_possible(cell_query_lat, cell_query_lon, level, cell_start_lat, cell_start_lon, cell_end_lat,
                       cell_end_lon))
    {
        double distance = distance_from_query_to_level(query_lat, query_lon, cell_query_lat, cell_query_lon, level);
        q.push(kNN_q_member(distance, KNN_Q_MEMBER_LEVEL));
    }

    int current_k = 0;
    while (!q.empty())
    {
        kNN_q_member member = q.top();
        q.pop();

        if (member.type == KNN_Q_MEMBER_OBJECT)
        {
            if (start_real_lat <= member.real_lat && member.real_lat <= end_real_lat &&
                start_real_lon <= member.real_lon && member.real_lon <= end_real_lon)
            {
                current_k++;
                result.push_back(member.id);

                if (current_k == k)
                {
                    break;
                }
            }
        }
        else if (member.type == KNN_Q_MEMBER_CELL)
        {
            return_all_objects_in_cell(query_lat, query_lon, member.cell_lat, member.cell_lon, q);
        }
        else if (member.type == KNN_Q_MEMBER_LEVEL)
        {
            int cell_lat_up = cell_query_lat - level;
            if (cell_start_lat <= cell_lat_up && cell_lat_up <= cell_end_lat)
            {
                int cell_min = std::max(cell_query_lon - level, cell_start_lon);
                int cell_max = std::min(cell_query_lon + level, cell_end_lon);
                for (int cell_lon = cell_min; cell_lon <= cell_max; cell_lon++)
                {

                    if (!is_empty_cell(cell_lat_up, cell_lon))
                    {
                        double distance = distance_from_query_to_cell(query_lat, query_lon, cell_query_lat,
                                                                      cell_query_lon, cell_lat_up, cell_lon);
                        q.push(kNN_q_member(query_lat, query_lon, cell_lat_up, cell_lon, distance));
                    }
                }
            }

            int cell_lat_down = cell_query_lat + level;
            if (cell_start_lat <= cell_lat_down && cell_lat_down <= cell_end_lat)
            {
                int cell_min = std::max(cell_query_lon - level, cell_start_lon);
                int cell_max = std::min(cell_query_lon + level, cell_end_lon);
                for (int cell_lon = cell_min; cell_lon <= cell_max; cell_lon++)
                {
                    if (!is_empty_cell(cell_lat_down, cell_lon))
                    {
                        double distance = distance_from_query_to_cell(query_lat, query_lon, cell_query_lat,
                                                                      cell_query_lon, cell_lat_down, cell_lon);
                        q.push(kNN_q_member(query_lat, query_lon, cell_lat_down, cell_lon, distance));
                    }
                }
            }

            int cell_lon_left = cell_query_lon - level;
            if (cell_start_lon <= cell_lon_left && cell_lon_left <= cell_end_lon)
            {
                int cell_min = std::max(cell_query_lat - level + 1, cell_start_lat);
                int cell_max = std::min(cell_query_lat + level - 1, cell_end_lat);
                for (int cell_lat = cell_min; cell_lat <= cell_max; cell_lat++)
                {
                    if (!is_empty_cell(cell_lat, cell_lon_left))
                    {
                        double distance = distance_from_query_to_cell(query_lat, query_lon, cell_query_lat,
                                                                      cell_query_lon, cell_lat, cell_lon_left);
                        q.push(kNN_q_member(query_lat, query_lon, cell_lat, cell_lon_left, distance));
                    }
                }
            }

            int cell_lon_right = cell_query_lon + level;
            if (cell_start_lon <= cell_lon_right && cell_lon_right <= cell_end_lon)
            {
                int cell_min = std::max(cell_query_lat - level + 1, cell_start_lat);
                int cell_max = std::min(cell_query_lat + level - 1, cell_end_lat);
                for (int cell_lat = cell_min; cell_lat <= cell_max; cell_lat++)
                {
                    if (!is_empty_cell(cell_lat, cell_lon_right))
                    {
                        double distance = distance_from_query_to_cell(query_lat, query_lon, cell_query_lat,
                                                                      cell_query_lon, cell_lat, cell_lon_right);
                        q.push(kNN_q_member(query_lat, query_lon, cell_lat, cell_lon_right, distance));
                    }
                }
            }

            level++;
            if (level_possible(cell_query_lat, cell_query_lon, level, cell_start_lat, cell_start_lon, cell_end_lat,
                               cell_end_lon))
            {
                double distance =
                    distance_from_query_to_level(query_lat, query_lon, cell_query_lat, cell_query_lon, level);
                q.push(kNN_q_member(distance, KNN_Q_MEMBER_LEVEL));
            }
        }
    }

    auto PHASE_END = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(PHASE_END - PHASE_START).count();

    if (!Waffle::get_during_regrid())
    {
        Waffle::WaffleMaker_knn_time += query_time;
        Waffle::WaffleMaker_knn_num++;
    }

    if (!Waffle::get_during_regrid())
    {
        Waffle::user_knn_num++;
        Waffle::episode_knn_num++;
    }
    else
    {
        if (role == NEW_INDEX)
        {
            Waffle::user_knn_num++;
            Waffle::episode_knn_num++;
        }
    }
    Waffle::user_knn_time += query_time;
    Waffle::episode_knn_time += query_time;

    return true;
}

bool GridIndexManager::deletion(ID_TYPE ID, int cell_lat, int cell_lon, std::vector<Lock> &current_locks,
                                bool from_regrid)
{
    auto PHASE_START = std::chrono::high_resolution_clock::now();
    bool chunk_deleted = false;

    int chunk_lat = cell_lat / nCell_chunk_lat;
    int chunk_lon = cell_lon / nCell_chunk_lon;
    int cell_lat_in_chunk = cell_lat % nCell_chunk_lat;
    int cell_lon_in_chunk = cell_lon % nCell_chunk_lon;
    int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;

    bool have = false;
    for (auto &lock : current_locks)
    {
        if (lock.get_which_index() == role && lock.get_chunk_id() == chunk_ID && lock.get_lock_type() == EXCLUSIVE_LOCK)
        {
            have = true;
        }
    }
    if (!have)
    {
        lockManager->X_lock(chunk_ID);
        current_locks.emplace_back(role, chunk_ID, EXCLUSIVE_LOCK);
    }

    int cpStart = chunk_ID * 2;
    Chunk *&chunk_start = total_chunks[cpStart];
    Chunk *&chunk_end = total_chunks[cpStart + 1];

    Chunk *target_chunk = chunk_start;
    int object_position = -1;
    int cell_ID = -1;

    while (target_chunk != nullptr)
    {
        std::pair<int, int> result = target_chunk->find_object(cell_lat_in_chunk, cell_lon_in_chunk, ID);
        cell_ID = result.first;
        object_position = result.second;
        if (object_position != -1)
        {
            break;
        }
        target_chunk = target_chunk->get_next();
    }

    assert(object_position != -1);
    const auto &object = target_chunk->get_objects()[object_position];
    const double o_lat = object.get_lat();
    const double o_lon = object.get_lon();

    Chunk *target_end = chunk_end;
    while (target_end != nullptr)
    {
        if (target_end->get_num_objects()[cell_ID] > 0)
        {
            break;
        }
        target_end = target_end->get_prev();
    }

    Object last_member = target_end->get_last_object(cell_ID);
    target_chunk->update_object(object_position, last_member);
    const bool chunk_empty = target_end->delete_last_object(cell_ID);

    if (chunk_empty)
    {
        Chunk *temp = target_end;
        Chunk *prev_chunk = target_end->get_prev();
        if (prev_chunk != nullptr)
        {
            prev_chunk->set_next(nullptr);
            chunk_end = prev_chunk;
        }
        else
        {
            chunk_start = chunk_end = nullptr;
        }

        delete temp;

        chunk_deleted = true;
    }

    auto PHASE_END = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(PHASE_END - PHASE_START).count();

    if (chunk_empty)
    {
        num_chunks--;
    }

    if (!from_regrid)
    {
        int stateX =
            std::min((int)std::floor((o_lat - getOMinLat()) / Waffle::state_cell_size_lat), (int)nCell_state_lat - 1);
        int stateY =
            std::min((int)std::floor((o_lon - getOMinLon()) / Waffle::state_cell_size_lon), (int)nCell_state_lon - 1);
        Waffle::current_state[stateX * nCell_state_lon + stateY]--;
    }

    // Updating the data structure is excluded from the execution time, which is common to the compared methods.
    ID_cell[ID] = {INTEGER_MAX, INTEGER_MAX};

    if (!from_regrid)
    { // A user query
        Waffle::user_deletion_num++;
        Waffle::user_deletion_time += query_time;

        Waffle::episode_delete_num++;
        Waffle::episode_delete_time += query_time;

        if (!Waffle::get_during_regrid())
        {
            Waffle::WaffleMaker_delete_num++;
            Waffle::WaffleMaker_delete_time += query_time;
        }
    }

    return chunk_deleted;
}

bool GridIndexManager::level_possible(int cell_query_lat, int cell_query_lon, int level, int cell_min_lat,
                                      int cell_min_lon, int cell_max_lat, int cell_max_lon) const
{

    int start_lat = cell_query_lat - level;
    int start_lon = cell_query_lon - level;
    int end_lat = cell_query_lat + level;
    int end_lon = cell_query_lon + level;

    if (start_lat >= cell_min_lat && start_lat <= cell_max_lat)
    {
        if ((start_lon >= cell_min_lon && start_lon <= cell_max_lon) ||
            (end_lon >= cell_min_lon && end_lon <= cell_max_lon))
        {
            return true;
        }
    }

    if (end_lat >= cell_min_lat && end_lat <= cell_max_lat)
    {
        if ((end_lon >= cell_min_lon && end_lon <= cell_max_lon) ||
            start_lon >= cell_min_lon && start_lon <= cell_max_lon)
        {
            return true;
        }
    }

    return false;
}

double GridIndexManager::distance_from_query_to_level(const double query_lat, const double query_lon,
                                                      const int cell_lat, const int cell_lon, const int level) const
{
    assert(0 <= cell_lat && cell_lat < nCell_space_lat && 0 <= cell_lon && cell_lon < nCell_space_lon);

    double distance = DOUBLE_MAX;

    int up_cell_lat = cell_lat - level;
    if (up_cell_lat >= 0)
    {
        distance = std::min(distance, pow(o_minLat + ((up_cell_lat + 1) * real_one_cell_lat) - query_lat, 2));
    }
    int down_cell_lat = cell_lat + level;
    if (down_cell_lat < nCell_space_lat)
    {
        distance = std::min(distance, pow(o_minLat + down_cell_lat * real_one_cell_lat - query_lat, 2));
    }
    int left_cell_lon = cell_lon - level;
    if (left_cell_lon >= 0)
    {
        distance = std::min(distance, pow(o_minLon + (left_cell_lon + 1) * real_one_cell_lon - query_lon, 2));
    }
    int right_cell_lon = cell_lon + level;
    if (right_cell_lon < nCell_space_lon)
    {
        distance = std::min(distance, pow(o_minLon + right_cell_lon * real_one_cell_lon - query_lon, 2));
    }

    return distance;
}

double GridIndexManager::distance_from_query_to_cell(double query_lat, double query_lon, int cell_query_lat,
                                                     int cell_query_lon, int cell_target_lat, int cell_target_lon) const
{
    assert((cell_query_lat != cell_target_lat) || (cell_query_lon != cell_target_lon));
    assert(0 <= cell_target_lat && cell_target_lat < nCell_space_lat);
    assert(0 <= cell_target_lon && cell_target_lon < nCell_space_lon);

    if (cell_query_lon == cell_target_lon)
    {
        if (cell_query_lat < cell_target_lat)
        {
            return pow(o_minLat + cell_target_lat * real_one_cell_lat - query_lat, 2);
        }
        else
        {
            assert(cell_query_lat != cell_target_lat);
            return pow(o_minLat + (cell_target_lat + 1) * real_one_cell_lat - query_lat, 2);
        }
    }
    else if (cell_query_lat == cell_target_lat)
    {
        if (cell_query_lon < cell_target_lon)
        {
            return pow(o_minLon + cell_target_lon * real_one_cell_lon - query_lon, 2);
        }
        else
        {
            assert(cell_query_lon != cell_target_lon);
            return pow(o_minLon + (cell_target_lon + 1) * real_one_cell_lon - query_lon, 2);
        }
    }

    if (cell_target_lat < cell_query_lat)
    {
        if (cell_target_lon < cell_query_lon)
        {
            return pow(o_minLat + (cell_target_lat + 1) * real_one_cell_lat - query_lat, 2) +
                   pow(o_minLon + (cell_target_lon + 1) * real_one_cell_lon - query_lon, 2);
        }
        else
        {
            return pow(o_minLat + (cell_target_lat + 1) * real_one_cell_lat - query_lat, 2) +
                   pow(o_minLon + cell_target_lon * real_one_cell_lon - query_lon, 2);
        }
    }
    else
    {
        if (cell_target_lon < cell_query_lon)
        {
            return pow(o_minLat + cell_target_lat * real_one_cell_lat - query_lat, 2) +
                   pow(o_minLon + (cell_target_lon + 1) * real_one_cell_lon - query_lon, 2);
        }
        else
        {
            return pow(o_minLat + cell_target_lat * real_one_cell_lat - query_lat, 2) +
                   pow(o_minLon + cell_target_lon * real_one_cell_lon - query_lon, 2);
        }
    }
}

void GridIndexManager::return_all_objects_in_cell(
    const double query_lat, const double query_lon, const int cell_lat, const int cell_lon,
    std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> &q) const
{
    assert(0 <= cell_lat && cell_lat < nCell_space_lat);
    assert(0 <= cell_lon && cell_lon < nCell_space_lon);

    const int chunk_lat = cell_lat / nCell_chunk_lat;
    const int chunk_lon = cell_lon / nCell_chunk_lon;

    int cell_lat_in_chunk = cell_lat - chunk_lat * nCell_chunk_lat;
    int cell_lon_in_chunk = cell_lon - chunk_lon * nCell_chunk_lon;

    int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;
    int cp_start = chunk_ID * 2;
    Chunk *&chunk_start = total_chunks[cp_start];
    Chunk *&chunk_end = total_chunks[cp_start + 1];

    for (Chunk *cp = chunk_start; cp != nullptr; cp = cp->get_next())
    {
        cp->return_all_objects_in_cell(query_lat, query_lon, cell_lat_in_chunk, cell_lon_in_chunk, q);
    }
}

bool GridIndexManager::is_empty_cell(const int cell_lat, const int cell_lon) const
{
    const int chunk_lat = cell_lat / nCell_chunk_lat;
    const int chunk_lon = cell_lon / nCell_chunk_lon;

    int chunk_ID = chunk_lat * num_chunks_lon + chunk_lon;
    int cp_start = chunk_ID * 2;
    Chunk *&chunk_start = total_chunks[cp_start];
    Chunk *&chunk_end = total_chunks[cp_start + 1];

    if (chunk_start == nullptr)
    {
        return true;
    }

    int cell_lat_in_chunk = cell_lat - chunk_lat * nCell_chunk_lat;
    int cell_lon_in_chunk = cell_lon - chunk_lon * nCell_chunk_lon;
    int cell_ID = cell_lat_in_chunk * nCell_chunk_lon + cell_lon_in_chunk;

    if (chunk_start->is_empty_cell(cell_ID))
    {
        return true;
    }

    return false;
}