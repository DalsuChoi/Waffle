#include "Chunk.h"
#include "Waffle.h"
#include <cassert>

Object::Object(IDType ID, double lat, double lon)
{
    Object::ID = ID;
    Object::lat = lat;
    Object::lon = lon;
}

IDType Object::get_ID() const
{
    return ID;
}

void Object::set_ID(IDType id)
{
    ID = id;
}

double Object::get_lat() const
{
    return lat;
}

double Object::get_lon() const
{
    return lon;
}

Chunk::Chunk(int _CHUNK_SIZE_LATITUDE, int _CHUNK_SIZE_LONGITUDE, int _MAX_IN_CELL)
    : nCell_chunk_lat(_CHUNK_SIZE_LATITUDE), nCell_chunk_lon(_CHUNK_SIZE_LONGITUDE), MOPC(_MAX_IN_CELL)
{
    int nCells = nCell_chunk_lat * nCell_chunk_lon;
    num_objects = new int[nCells];
    std::fill_n(num_objects, nCells, 0);

    next = nullptr;
    prev = nullptr;

    objects = new Object[nCells * MOPC];
    num_total_objects = 0;
}

Chunk::~Chunk()
{
    delete[] num_objects;
    delete[] objects;
}

bool Chunk::insertion(int cell_lat_in_chunk, int cell_lon_in_chunk, const Object &member)
{
    int cellID = cell_lat_in_chunk * nCell_chunk_lon + cell_lon_in_chunk;
    int &_nObjects = num_objects[cellID];

    // Not enough space to insert the given object.
    if (_nObjects == MOPC)
    {
        return false;
    }

    int objectPosition = cellID * MOPC + _nObjects;
    objects[objectPosition] = member;
    _nObjects++;
    num_total_objects++;
    return true;
}

bool Chunk::update_object(int object_position, const Object &object)
{
    objects[object_position] = object;
    return true;
}

bool Chunk::delete_last_object(int cell_ID)
{
    num_objects[cell_ID]--;
    num_total_objects--;

    return (num_total_objects == 0);
}

std::pair<int, int> Chunk::find_object(int cell_lat_in_chunk, int cell_lon_in_chunk, IDType object_ID) const
{
    int cellID = cell_lat_in_chunk * nCell_chunk_lon + cell_lon_in_chunk;
    const int &_nObjects = num_objects[cellID];

    int start = cellID * MOPC;
    int end = start + _nObjects;

    for (int object_position = start; object_position < end; object_position++)
    {
        if (objects[object_position].get_ID() == object_ID)
        {
            return std::make_pair(cellID, object_position);
        }
    }
    return std::make_pair(-1, -1);
}

Object Chunk::get_last_object(int cell_ID) const
{
    return objects[cell_ID * MOPC + num_objects[cell_ID] - 1];
}

const Object *Chunk::get_objects() const
{
    return objects;
};

const int *Chunk::get_num_objects() const
{
    return num_objects;
};

Chunk *Chunk::get_next() const
{
    return next;
}

void Chunk::set_next(Chunk *next)
{
    Chunk::next = next;
}

Chunk *Chunk::get_prev() const
{
    return prev;
}

void Chunk::set_prev(Chunk *prev)
{
    Chunk::prev = prev;
}

void Chunk::return_all_objects(std::vector<int> &result) const
{
    for (int lat = 0; lat <= nCell_chunk_lat - 1; lat++)
    {
        for (int lon = 0; lon <= nCell_chunk_lon - 1; lon++)
        {
            int cell_ID = lat * nCell_chunk_lon + lon;
            int start = cell_ID * MOPC;
            int end = start + num_objects[cell_ID];

            for (int cell_member = start; cell_member < end; cell_member++)
            {
                result.push_back(objects[cell_member].get_ID());
            }
        }
    }
}

void Chunk::return_selected_objects(std::vector<int> &result, int new_cell_start_lat, int new_cell_end_lat,
                                    int new_cell_start_lon, int new_cell_end_lon, double start_lat, double start_lon,
                                    double end_lat, double end_lon) const
{
    return_all_objects(result, new_cell_start_lat + 1, new_cell_start_lon + 1, new_cell_end_lat - 1,
                       new_cell_end_lon - 1);

    int walk_lat[4] = {0, 1, 0, -1};
    int walk_lon[4] = {1, 0, -1, 0};
    int direction = 0;

    int cell_lat = new_cell_start_lat;
    int cell_lon = new_cell_start_lon;

    while (true)
    {
        int cell_ID = cell_lat * nCell_chunk_lon + cell_lon;
        int start = (cell_ID)*MOPC;
        int end = start + num_objects[cell_ID];

        for (int cell_member = start; cell_member < end; cell_member++)
        {
            double object_lat = objects[cell_member].get_lat();
            double object_lon = objects[cell_member].get_lon();
            if (object_lat >= start_lat && object_lat <= end_lat && object_lon >= start_lon && object_lon <= end_lon)
            {
                result.push_back(objects[cell_member].get_ID());
            }
        }

        int trial = 0;
        while (true)
        {
            cell_lat += walk_lat[direction];
            cell_lon += walk_lon[direction];
            trial++;

            if (cell_lat == new_cell_start_lat && cell_lon == new_cell_start_lon)
            {
                return;
            }

            if (cell_lat < new_cell_start_lat || cell_lat > new_cell_end_lat || cell_lon < new_cell_start_lon ||
                cell_lon > new_cell_end_lon)
            {
                if (trial == 2)
                {
                    return;
                }

                cell_lat -= walk_lat[direction];
                cell_lon -= walk_lon[direction];

                direction++;
                if (direction == 4)
                {
                    return;
                }
            }
            else
            {
                break;
            }
        }
    }
}

void Chunk::return_all_objects(std::vector<int> &result, int start_lat, int start_lon, int end_lat, int end_lon) const
{
    for (int lat = start_lat; lat <= end_lat; lat++)
    {
        for (int lon = start_lon; lon <= end_lon; lon++)
        {
            int cell_ID = lat * nCell_chunk_lon + lon;

            int start = cell_ID * MOPC;
            int end = start + num_objects[cell_ID];
            for (int cell_member = start; cell_member < end; cell_member++)
            {
                result.push_back(objects[cell_member].get_ID());
            }
        }
    }
}

void Chunk::return_all_objects_in_cell(
    double query_lat, double query_lon, int cell_lat_in_chunk, int cell_lon_in_chunk,
    std::priority_queue<kNN_q_member, std::vector<kNN_q_member>, compare_kNN> &q) const
{
    int cell_id = cell_lat_in_chunk * nCell_chunk_lon + cell_lon_in_chunk;
    int start = cell_id * MOPC;
    int end = start + num_objects[cell_id];
    for (int cell = start; cell < end; cell++)
    {
        q.push(kNN_q_member(query_lat, query_lon, objects[cell].get_lat(), objects[cell].get_lon(),
                            objects[cell].get_ID()));
    }
}

bool Chunk::is_empty_cell(const int cell_ID)
{
    if (num_objects[cell_ID] == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

unsigned Chunk::get_memory_usage_of_one_chunk(int nCell_chunk_lat, int nCell_chunk_lon, int MOPC)
{
    return sizeof(int) * nCell_chunk_lat * nCell_chunk_lon + (sizeof(Object)) * nCell_chunk_lat * nCell_chunk_lon * MOPC;
}
