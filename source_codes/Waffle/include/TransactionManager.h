#ifndef TRANSACTIONMANAGER_H
#define TRANSACTIONMANAGER_H

#include "GridIndexManager.h"
#include <iostream>
#include <mutex>
#include <vector>

class Transaction
{
  public:
    Transaction(GridIndexManager *&original_index, GridIndexManager *&new_index);

    bool insertion(int which_index, const Object &new_object, bool from_regrid = false);
    bool deletion(int which_index, const Object &new_object, bool from_regrid = false);
    bool deletion(int which_index, ID_TYPE ID, int cell_lat, int cell_lon, bool from_regrid = false);
    bool range(int which_index, double start_latitude, double start_longitude, double end_latitude,
               double end_longitude, std::vector<ID_TYPE> &result);
    bool kNN(int which_index, int k, double query_lat, double query_lon, double start_lat, double start_lon,
             double end_lat, double end_lon, std::vector<ID_TYPE> &result);
    void getXLock(int which_index, int chunk_ID);
    void release_locks();
    void commit();

  private:
    GridIndexManager *&original_index;
    GridIndexManager *&new_index;
    std::vector<Lock> current_locks;
};

class TransactionManager
{
  public:
    bool process_insertion_query(ID_TYPE object_ID, double n_lat, double n_lon, GridIndexManager *&gridIndexManager,
                                 GridIndexManager *&newGridIndexManager, bool during_regrid);
    bool process_deletion_query(ID_TYPE object_ID, GridIndexManager *&gridIndexManager,
                                GridIndexManager *&newGridIndexManager, bool during_regrid);
    bool process_range_query(double start_lon, double start_lat, double end_lon, double end_lat,
                             std::vector<ID_TYPE> &result, GridIndexManager *&gridIndexManager,
                             GridIndexManager *&newGridIndexManager, bool during_regrid);
    bool process_knn_query(int k, double start_lon, double start_lat, double end_lon, double end_lat, double center_lat,
                           double center_lon, std::vector<ID_TYPE> result, GridIndexManager *&gridIndexManager,
                           GridIndexManager *&newGridIndexManager, bool during_regrid);
};

#endif
