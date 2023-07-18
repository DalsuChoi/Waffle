#include "LockManager.h"
#include "Parameters.h"
#include <cassert>
#include <iostream>

LockManager::LockManager(int num_chunks_lat, int num_chunks_lon)
{
    int num_chunks = num_chunks_lat * num_chunks_lon;
    locks = new int[num_chunks];
    m = new std::mutex[num_chunks];
    std::fill_n(locks, num_chunks, 0);
}

void LockManager::S_lock(int chunk_ID)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m[chunk_ID]);
        if (!has_X_lock(chunk_ID))
        {
            assert(locks[chunk_ID] != -1);
            locks[chunk_ID]++; // Grant
            return;
        }
    }
}

void LockManager::X_lock(int chunk_ID)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m[chunk_ID]);
        if (!has_S_locks(chunk_ID) && !has_X_lock(chunk_ID))
        {
            assert(locks[chunk_ID] == 0);
            locks[chunk_ID] = -1; // Grant
            return;
        }
    }
}

bool LockManager::has_X_lock(int chunk_ID) const
{
    return (locks[chunk_ID] == -1);
}

bool LockManager::has_S_locks(int chunk_ID) const
{
    return (locks[chunk_ID] >= 1);
}

void LockManager::unlock(int chunk_ID, bool lock_type)
{
    std::unique_lock<std::mutex> lock(m[chunk_ID]);
    if (lock_type == SHARED_LOCK)
    {
        assert(locks[chunk_ID] > 0);
        locks[chunk_ID]--;
    }
    else if (lock_type == EXCLUSIVE_LOCK)
    {
        assert(locks[chunk_ID] == -1);
        locks[chunk_ID] = 0;
    }
}

LockManager::~LockManager()
{
    delete[] locks;
    delete[] m;
}

Lock::Lock(int which_index, int chunk_ID, bool lock_type)
    : which_index(which_index), chunk_ID(chunk_ID), lock_type(lock_type)
{
}

int Lock::get_which_index() const
{
    return which_index;
}

int Lock::get_chunk_id() const
{
    return chunk_ID;
}

bool Lock::get_lock_type() const
{
    return lock_type;
}