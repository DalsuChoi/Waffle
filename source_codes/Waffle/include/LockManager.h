#ifndef LOCKMANAGER_H
#define LOCKMANAGER_H

#include <atomic>
#include <mutex>
#include <queue>

class LockManager
{
  public:
    LockManager(int num_chunks_lat, int num_chunks_lon);
    ~LockManager();

    void S_lock(int chunk_ID);
    void X_lock(int chunk_ID);
    void unlock(int chunk_ID, bool lock_type);

  private:
    int *locks;
    std::mutex *m;

    bool has_X_lock(int chunk_ID) const;
    bool has_S_locks(int chunk_ID) const;
};

class Lock
{
  public:
    Lock(int which_index, int chunk_ID, bool lock_type);

    int get_which_index() const;
    int get_chunk_id() const;
    bool get_lock_type() const;

  private:
    int which_index;
    int chunk_ID;
    bool lock_type;
};

#endif