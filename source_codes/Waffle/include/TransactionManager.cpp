#include "TransactionManager.h"
#include "RegridManager.h"
#include "Waffle.h"
#include <cassert>
#include <thread>

Transaction::Transaction(WaffleIndexManager *&original_index, WaffleIndexManager *&new_index)
    : original_index(original_index), new_index(new_index)
{
}

bool Transaction::insertion(int which_index, const Object &new_object, bool from_regrid)
{
    bool success;
    if (which_index == ORIGINAL_INDEX)
    {
        success = original_index->insertion(new_object, current_locks, from_regrid);
    }
    else
    {
        assert(which_index == NEW_INDEX);
        success = new_index->insertion(new_object, current_locks, from_regrid);
        assert(success);
    }

    return success;
}

bool Transaction::deletion(int which_index, const Object &new_object, bool from_regrid)
{
    switch (which_index)
    {
    case ORIGINAL_INDEX: {
        return original_index->deletion(new_object, current_locks, from_regrid);
    }
    case NEW_INDEX: {
        return new_index->deletion(new_object, current_locks, from_regrid);
    }
    default:
        return false;
    }
}

bool Transaction::range(int which_index, double start_latitude, double start_longitude, double end_latitude,
                        double end_longitude, std::vector<IDType> &result)
{
    switch (which_index)
    {
    case ORIGINAL_INDEX: {
        return original_index->range(start_latitude, start_longitude, end_latitude, end_longitude, result,
                                     current_locks);
    }
    case NEW_INDEX: {
        return new_index->range(start_latitude, start_longitude, end_latitude, end_longitude, result, current_locks);
    }
    default:
        return false;
    }
}

void Transaction::release_locks()
{
    for (const auto &lock : current_locks)
    {
        switch (lock.get_which_index())
        {
        case ORIGINAL_INDEX: {
            original_index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
            break;
        }
        case NEW_INDEX: {
            if (new_index != nullptr)
            {
                new_index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
            }
            break;
        }
        }
    }
}

void Transaction::commit()
{
    release_locks();
}

bool Transaction::deletion(int which_index, IDType ID, int cell_lat, int cell_lon, bool from_regrid)
{
    switch (which_index)
    {
    case ORIGINAL_INDEX: {
        return original_index->deletion(ID, cell_lat, cell_lon, current_locks, from_regrid);
    }
    case NEW_INDEX: {
        return new_index->deletion(ID, cell_lat, cell_lon, current_locks, from_regrid);
    }
    default:
        return false;
    }
}

bool Transaction::kNN(const int which_index, const int k, const double query_lat, const double query_lon,
                      const double start_lat, const double start_lon, const double end_lat, const double end_lon,
                      std::vector<IDType> &result)
{
    switch (which_index)
    {
    case ORIGINAL_INDEX: {
        return original_index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result,
                                   current_locks);
    }
    case NEW_INDEX: {
        return new_index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    }
    default:
        return false;
    }
}

void Transaction::getXLock(const int which_index, const int chunk_ID)
{
    switch (which_index)
    {
    case ORIGINAL_INDEX: {
        original_index->get_lock_manager()->X_lock(chunk_ID); // wait until getting the exclusive lock.
        current_locks.emplace_back(ORIGINAL_INDEX, chunk_ID, EXCLUSIVE_LOCK);
        break;
    }
    case NEW_INDEX: {
        new_index->get_lock_manager()->X_lock(chunk_ID); // wait until getting the exclusive lock.
        current_locks.emplace_back(NEW_INDEX, chunk_ID, EXCLUSIVE_LOCK);
        break;
    }
    default:
        return;
    }
}

bool TransactionManager::process_insertion_query(const IDType object_ID, const double n_lat, const double n_lon,
                                                 WaffleIndexManager *&original_index, WaffleIndexManager *&new_index,
                                                 const bool during_regrid)
{
    if (original_index->chunks == nullptr)
    {
        original_index->set_internal_parameters(n_lat, n_lon, n_lat, n_lon);
    }

    if (original_index->ID_cell[object_ID].first != INTEGER_MAX)
    {
        const int old_cell_lat = original_index->ID_cell[object_ID].first;
        const int old_cell_lon = original_index->ID_cell[object_ID].second;

        Object new_object(object_ID, n_lat, n_lon);
        if (!during_regrid)
        {
            Transaction tx(original_index, new_index);
            tx.deletion(ORIGINAL_INDEX, object_ID, old_cell_lat, old_cell_lon, false);
            tx.insertion(ORIGINAL_INDEX, new_object, false);
            tx.commit();
            return true;
        }
        else
        {
            Transaction tx(original_index, new_index);
            tx.deletion(ORIGINAL_INDEX, object_ID, old_cell_lat, old_cell_lon, false);
            tx.insertion(NEW_INDEX, new_object, false);
            tx.commit();
            return true;
        }
    }
    else
    {
        if (!during_regrid)
        {
            Waffle::current_object_num++;
            Transaction tx(original_index, new_index);
            tx.insertion(ORIGINAL_INDEX, Object(object_ID, n_lat, n_lon), false);
            tx.commit();
            return true;
        }
        else
        {
            if (new_index->ID_cell[object_ID].first != INTEGER_MAX)
            {
                int old_cell_lat = new_index->ID_cell[object_ID].first;
                int old_cell_lon = new_index->ID_cell[object_ID].second;

                Transaction tx(original_index, new_index);
                tx.deletion(NEW_INDEX, object_ID, old_cell_lat, old_cell_lon, false);
                tx.insertion(NEW_INDEX, Object(object_ID, n_lat, n_lon), false);
                tx.commit();
                return true;
            }
            else
            {
                Waffle::current_object_num++;
                Transaction tx(original_index, new_index);
                tx.insertion(NEW_INDEX, Object(object_ID, n_lat, n_lon), false);
                tx.commit();
                return true;
            }
        }
    }

    return true;
}

bool TransactionManager::process_deletion_query(const IDType object_ID, WaffleIndexManager *&original_index,
                                                WaffleIndexManager *&new_index, const bool during_regrid)
{
    Waffle::current_object_num--;
    if (original_index->ID_cell[object_ID].first != INTEGER_MAX)
    {
        int cell_lat = original_index->ID_cell[object_ID].first;
        int cell_lon = original_index->ID_cell[object_ID].second;

        Transaction tx(original_index, new_index);
        tx.deletion(ORIGINAL_INDEX, object_ID, cell_lat, cell_lon, false);
        tx.commit();
    }
    else
    {
        if (during_regrid)
        {
            if (new_index->ID_cell[object_ID].first != INTEGER_MAX)
            {
                int cell_lat = new_index->ID_cell[object_ID].first;
                int cell_lon = new_index->ID_cell[object_ID].second;

                Transaction tx(original_index, new_index);
                tx.deletion(NEW_INDEX, object_ID, cell_lat, cell_lon, false);
                tx.commit();
            }
        }
    }

    return true;
}

bool TransactionManager::process_range_query(const double start_lon, const double start_lat, const double end_lon,
                                             const double end_lat, std::vector<IDType> &result,
                                             WaffleIndexManager *&original_index,
                                             WaffleIndexManager *&new_index, const bool during_regrid)
{
    if (!during_regrid)
    {
        Transaction tx(original_index, new_index);
        tx.range(ORIGINAL_INDEX, start_lat, start_lon, end_lat, end_lon, result);
        tx.commit();
    }
    else
    {
        Transaction tx(original_index, new_index);
        tx.range(ORIGINAL_INDEX, start_lat, start_lon, end_lat, end_lon, result);
        tx.range(NEW_INDEX, start_lat, start_lon, end_lat, end_lon, result);
        tx.commit();
    }

    return true;
}

bool TransactionManager::process_knn_query(const int k, const double start_lon, const double start_lat,
                                           const double end_lon, const double end_lat, const double center_lat,
                                           const double center_lon, std::vector<IDType> result,
                                           WaffleIndexManager *&original_index, WaffleIndexManager *&new_index,
                                           const bool during_regrid)
{
    if (!during_regrid)
    {
        Transaction tx(original_index, new_index);

        tx.kNN(ORIGINAL_INDEX, k, center_lat, center_lon, start_lat, start_lon, end_lat, end_lon, result);
        tx.commit();
    }
    else
    {
        Transaction tx(original_index, new_index);
        tx.kNN(ORIGINAL_INDEX, k, center_lat, center_lon, start_lat, start_lon, end_lat, end_lon, result);
        tx.kNN(NEW_INDEX, k, center_lat, center_lon, start_lat, start_lon, end_lat, end_lon, result);
        tx.commit();
    }

    return true;
}