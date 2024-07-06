#include "WaffleIndexManager.h"
#include "WaffleMaker/WaffleMaker.h"
#include "Waffle.h"
#include <gtest/gtest.h>

#include <iostream>
#include <random>

class UnitTestWaffleIndexManager : public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    WaffleIndexManager* initialize_index()
    {
        const int nCell_space_lat = 10;
        const int nCell_space_lon = 20;
        const int MOPC = 2;
        const int nCell_chunk_lat = 2;
        const int nCell_chunk_lon = 2;
        const uint64_t num_total_objects = 100;

        WaffleIndexManager *index = new WaffleIndexManager(nCell_space_lat, nCell_space_lon,
                                                MOPC, nCell_chunk_lat, nCell_chunk_lon, ORIGINAL_INDEX, num_total_objects);

        const double min_lat = -10;
        const double min_lon = -100;
        const double max_lat = 10;
        const double max_lon = 100;

        index->set_internal_parameters(min_lat, min_lon, max_lat, max_lon);

        WaffleMaker::state.initialize(10, 10);
        WaffleMaker::state.calculate_state_cell_size(min_lon, min_lat, max_lon, max_lat);

        return index;
    }

    void get_correct_results_for_range_query(double start_lat, double start_lon, double end_lat, double end_lon, const std::vector<Object>& objects, std::unordered_set<IDType>& result)
    {
        for (const auto& object: objects) {
            if (start_lat <= object.get_lat() && object.get_lat() <= end_lat && start_lon <= object.get_lon() && object.get_lon() <= end_lon) {
                result.insert(object.get_ID());
            }
        }
    }

    void get_correct_results_for_knn_query(double query_lat, double query_lon, double start_lat, double start_lon, double end_lat, double end_lon, const std::vector<Object>& objects, std::vector<IDType>& result)
    {
        std::vector<std::pair<double, IDType>> distance_ID;
        for (const auto& object: objects) {
            if (start_lat <= object.get_lat() && object.get_lat() <= end_lat && start_lon <= object.get_lon() && object.get_lon() <= end_lon) {
                double distance = pow(object.get_lat() - query_lat, 2) + pow(object.get_lon() - query_lon, 2);
                distance_ID.emplace_back(distance, object.get_ID());
            }
        }
        std::sort(distance_ID.begin(), distance_ID.end());
        for (const auto& pair: distance_ID) {
            result.push_back(pair.second);
        }
    }
};

TEST_F(UnitTestWaffleIndexManager, WaffleIndexManager)
{
    const int nCell_space_lat = 10;
    const int nCell_space_lon = 20;
    const int MOPC = 2;
    const int nCell_chunk_lat = 2;
    const int nCell_chunk_lon = 2;
    const uint64_t num_total_objects = 100;

    WaffleIndexManager *index = new WaffleIndexManager(nCell_space_lat, nCell_space_lon,
                                            MOPC, nCell_chunk_lat, nCell_chunk_lon, ORIGINAL_INDEX, num_total_objects);

    EXPECT_EQ(index->nCell_space_lat, nCell_space_lat);
    EXPECT_EQ(index->nCell_space_lon, nCell_space_lon);
    EXPECT_EQ(index->MOPC, MOPC);
    EXPECT_EQ(index->nCell_chunk_lat, nCell_chunk_lat);
    EXPECT_EQ(index->nCell_chunk_lon, nCell_chunk_lon);
    EXPECT_EQ(index->role, ORIGINAL_INDEX);
    for (int i = 0; i < num_total_objects; i++) {
        const auto& coordinate = index->get_cell_coordinate(i);
        EXPECT_EQ(coordinate.first, INTEGER_MAX);
        EXPECT_EQ(coordinate.second, INTEGER_MAX);
    }
    EXPECT_EQ(index->chunks, nullptr);
    EXPECT_EQ(index->lock_manager, nullptr);
    EXPECT_EQ(index->num_chunks, 0);
    EXPECT_EQ(index->memory_one_chunk, 208);

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, setInternalParameters)
{
    const int nCell_space_lat = 10;
    const int nCell_space_lon = 20;
    const int MOPC = 2;
    const int nCell_chunk_lat = 2;
    const int nCell_chunk_lon = 2;
    const uint64_t num_total_objects = 100;

    WaffleIndexManager *index = new WaffleIndexManager(nCell_space_lat, nCell_space_lon,
                                            MOPC, nCell_chunk_lat, nCell_chunk_lon, ORIGINAL_INDEX, num_total_objects);

    const double min_lat = -10;
    const double min_lon = -100;
    const double max_lat = 10;
    const double max_lon = 100;

    index->set_internal_parameters(min_lat, min_lon, max_lat, max_lon);

    EXPECT_EQ(index->get_min_lat(), min_lat);
    EXPECT_EQ(index->get_min_lon(), min_lon);
    EXPECT_EQ(index->get_max_lat(), max_lat);
    EXPECT_EQ(index->get_max_lon(), max_lon);
    EXPECT_EQ(index->cell_size_lat, 2);
    EXPECT_EQ(index->cell_size_lon, 10);
    EXPECT_EQ(index->num_chunks_lat, 5);
    EXPECT_EQ(index->num_chunks_lon, 10);

    const int num_chunks = index->num_chunks_lat * index->num_chunks_lon;
    EXPECT_EQ(num_chunks, 50);
    for (int i = 0; i < num_chunks * 2; i++) {
        EXPECT_EQ(index->chunks[i], nullptr);
    }

    EXPECT_NE(index->lock_manager, nullptr);

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, insertion)
{
    WaffleIndexManager* index = initialize_index();
    const IDType id = 10;
    const Object object(id, -7.2436, 49.3024);
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    Waffle::user_insertion_num = 0;
    Waffle::user_insertion_time = 0;
    Waffle::episode_insert_num = 0;
    Waffle::episode_insert_time = 0;
    Waffle::WaffleMaker_insert_num = 0;
    Waffle::WaffleMaker_insert_time = 0;

    EXPECT_EQ(index->insertion(object, current_locks, from_regrid), true);

    EXPECT_EQ(Waffle::user_insertion_num, 1);
    EXPECT_GT(Waffle::user_insertion_time, 0);
    EXPECT_EQ(Waffle::episode_insert_num, 1);
    EXPECT_GT(Waffle::episode_insert_time, 0);
    EXPECT_EQ(Waffle::WaffleMaker_insert_num, 1);
    EXPECT_GT(Waffle::WaffleMaker_insert_time, 0);

    for (IDType i = 0; i < index->get_num_total_objects(); i++) {
        if (i == id) {
            EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(1, 14));
        } else {
            EXPECT_EQ(index->get_cell_coordinate(i), std::make_pair(INTEGER_MAX, INTEGER_MAX));
        }
    }

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, insertionIntoTheSameCoordinate)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;
    uint64_t num_total_objects = index->get_num_total_objects();

    const double object_lat = -9.9999;
    const double object_lon = 99.9999;
    for (IDType id = 0; id < num_total_objects; id++) {
        const Object object(id, object_lat, object_lon);
        EXPECT_EQ(index->insertion(object, current_locks, from_regrid), true);
        EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(0, 19));
    }

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, insertionHasLockAlready)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    current_locks.emplace_back(ORIGINAL_INDEX, 40, EXCLUSIVE_LOCK);
    bool from_regrid = false;

    const IDType id = 0;
    const Object object(id, 9.9999, -99.9999);
    EXPECT_EQ(current_locks.size(), 1);
    EXPECT_EQ(index->insertion(object, current_locks, from_regrid), true);
    EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(9, 0));
    EXPECT_EQ(current_locks.size(), 1);
}

TEST_F(UnitTestWaffleIndexManager, deletionFromSingleObject)
{
    WaffleIndexManager* index = initialize_index();
    const IDType id = 10;
    const Object object(id, 4.3291, -53.9098);
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    EXPECT_EQ(index->insertion(object, current_locks, from_regrid), true);
    EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(7, 4));

    Waffle::user_deletion_num = 0;
    Waffle::user_deletion_time = 0;
    Waffle::episode_delete_num = 0;
    Waffle::episode_delete_time = 0;
    Waffle::WaffleMaker_delete_num = 0;
    Waffle::WaffleMaker_delete_time = 0;

    EXPECT_EQ(index->deletion(object, current_locks, from_regrid), true);
    EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(INTEGER_MAX, INTEGER_MAX));

    EXPECT_EQ(Waffle::user_deletion_num, 1);
    EXPECT_GT(Waffle::user_deletion_time, 0);
    EXPECT_EQ(Waffle::episode_delete_num, 1);
    EXPECT_GT(Waffle::episode_delete_time, 0);
    EXPECT_EQ(Waffle::WaffleMaker_delete_num, 1);
    EXPECT_GT(Waffle::WaffleMaker_delete_time, 0);

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, deletionFromMultipleObjects)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const double object_lat = -9.9999;
    const double object_lon = 99.9999;
    for (IDType id = 0; id < 9; id++) {
        const Object object(id, object_lat, object_lon);
        EXPECT_EQ(index->insertion(object, current_locks, from_regrid), true);
        EXPECT_EQ(index->get_cell_coordinate(id), std::make_pair(0, 19));
    }

    Chunk* chunk_start1 = index->chunks[18];
    Chunk* chunk_end1 = index->chunks[19];
    Chunk* chunk_pointer = chunk_start1;
    int num_chunks = 0;
    while (chunk_pointer) {
        num_chunks++;
        chunk_pointer = chunk_pointer->get_next();
    }
    EXPECT_EQ(num_chunks, 5);

    const Object object(4, object_lat, object_lon);
    EXPECT_EQ(index->deletion(object, current_locks, from_regrid), true);
    EXPECT_EQ(index->get_cell_coordinate(4), std::make_pair(INTEGER_MAX, INTEGER_MAX));

    Chunk* chunk_start2 = index->chunks[18];
    Chunk* chunk_end2 = index->chunks[19];
    chunk_pointer = chunk_start2;
    num_chunks = 0;
    while (chunk_pointer) {
        num_chunks++;
        chunk_pointer = chunk_pointer->get_next();
    }
    EXPECT_NE(chunk_end1, chunk_end2);
    EXPECT_EQ(num_chunks, 4);

    delete index;
}

TEST_F(UnitTestWaffleIndexManager, rangeSimple)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 9.18334, 0.164023);
    const Object object1(1, -2.92095, -33.7923);
    const Object object2(2, 8.62274, 74.9315);
    const Object object3(3, -3.37835, -67.6786);
    const Object object4(4, -9.54006, 35.313);
    const Object object5(5, -6.62081, 99.8664);
    const Object object6(6, -1.75624, 33.5918);
    const Object object7(7, 4.81693, 78.8457);
    const Object object8(8, 6.41207, 84.1115);
    const Object object9(9, 3.75099, 79.1291);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double start_lat = -3.1;
    double start_lon = -30.5;
    double end_lat = 10.4;
    double end_lon = 40.9;
    std::vector<IDType> result;
    index->range(start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::unordered_set<IDType> correct_result;
    get_correct_results_for_range_query(start_lat, start_lon, end_lat, end_lon, objects, correct_result);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(correct_result.size(), 2);
    for (IDType id: result) {
        EXPECT_TRUE(correct_result.find(id) != correct_result.end());
        correct_result.erase(id);
    }
    EXPECT_TRUE(correct_result.empty());
}

TEST_F(UnitTestWaffleIndexManager, rangeLarge)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, -1.1279, 70.3254);
    const Object object1(1, -9, -39.9192);
    const Object object2(2, 3.5846, -19.6434);
    const Object object3(3, -2.6431, -80.3265);
    const Object object4(4, 9.3929, 99.9999);
    const Object object5(5, -6.3234, 36.7575);
    const Object object6(6, -1.2317, 59.3213);
    const Object object7(7, 8.8231, -12.8876);
    const Object object8(8, -4.4231, 0.2245);
    const Object object9(9, 0.8929, -3.7744);
    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    std::vector<IDType> result;
    double small_number = 0.0001;
    double start_lat = index->get_min_lat();
    double start_lon = index->get_min_lon();
    double end_lat = index->get_max_lat() - small_number;
    double end_lon = index->get_max_lon() - small_number;
    index->range(start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::unordered_set<IDType> correct_result;
    get_correct_results_for_range_query(start_lat, start_lon, end_lat, end_lon, objects, correct_result);

    EXPECT_EQ(result.size(), 10);
    EXPECT_EQ(correct_result.size(), 10);
    for (IDType id: result) {
        EXPECT_TRUE(correct_result.find(id) != correct_result.end());
        correct_result.erase(id);
    }
    EXPECT_TRUE(correct_result.empty());
}

TEST_F(UnitTestWaffleIndexManager, rangeSmall)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, -10, -100);
    const Object object1(1, -10, -100);
    const Object object2(2, -10, -100);
    const Object object3(3, -10, -100);
    const Object object4(4, -10, -100);
    const Object object5(5, -10, -100);
    const Object object6(6, -10, -100);
    const Object object7(7, -10, -100);
    const Object object8(8, -10, -100);
    const Object object9(9, -10, -100);
    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    std::vector<IDType> result;
    double start_lat = -10;
    double start_lon = -100;
    double end_lat = -10;
    double end_lon = -100;
    index->range(start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::unordered_set<IDType> correct_result;
    get_correct_results_for_range_query(start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), 10);
    EXPECT_EQ(correct_result.size(), 10);
    for (IDType id: result) {
        EXPECT_TRUE(correct_result.find(id) != correct_result.end());
        correct_result.erase(id);
    }
    EXPECT_TRUE(correct_result.empty());
}

TEST_F(UnitTestWaffleIndexManager, kNNSimple)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 9.18334, 0.164023);
    const Object object1(1, -2.92095, -33.7923);
    const Object object2(2, 8.62274, 74.9315);
    const Object object3(3, -3.37835, -67.6786);
    const Object object4(4, -9.54006, 35.313);
    const Object object5(5, -6.62081, 99.8664);
    const Object object6(6, -1.75624, 33.5918);
    const Object object7(7, 4.81693, 78.8457);
    const Object object8(8, 6.41207, 84.1115);
    const Object object9(9, 3.75099, 79.1291);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = 0.1928;
    double query_lon = -0.4837;
    double start_lat = -5.2;
    double start_lon = -50.6;
    double end_lat = 5.9;
    double end_lon = 60.8;
    const int k = 2;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNLargeRangeWithLargeK)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 9.18334, 0.164023);
    const Object object1(1, -2.92095, -33.7923);
    const Object object2(2, 8.62274, 74.9315);
    const Object object3(3, -3.37835, -67.6786);
    const Object object4(4, -9.54006, 35.313);
    const Object object5(5, -6.62081, 99.8664);
    const Object object6(6, -1.75624, 33.5918);
    const Object object7(7, 4.81693, 78.8457);
    const Object object8(8, 6.41207, 84.1115);
    const Object object9(9, 3.75099, 79.1291);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = 0.1928;
    double query_lon = -0.4837;
    double small_number = 0.0001;
    double start_lat = index->get_min_lat();
    double start_lon = index->get_min_lon();
    double end_lat = index->get_max_lat() - small_number;
    double end_lon = index->get_max_lon() - small_number;
    const int k = objects.size();
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNSmallRange)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const double lat = 2.4321;
    const double lon = -3.4321;
    const Object object0(0, lat, lon);
    const Object object1(1, lat, lon);
    const Object object2(2, lat, lon);
    const Object object3(3, lat, lon);
    const Object object4(4, lat, lon);
    const Object object5(5, lat, lon);
    const Object object6(6, lat, lon);
    const Object object7(7, lat, lon);
    const Object object8(8, lat, lon);
    const Object object9(9, lat, lon);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    const int k = objects.size();
    std::vector<IDType> result;
    index->kNN(k, lat, lon, lat, lon, lat, lon, result, current_locks);
    std::unordered_set<IDType> correct_result;
    for (IDType i = 0; i < k; i++) {
        correct_result.insert(i);
    }
    EXPECT_EQ(result.size(), k);
    for (IDType id: result) {
        EXPECT_TRUE(correct_result.find(id) != correct_result.end());
        correct_result.erase(id);
    }
    EXPECT_TRUE(correct_result.empty());
}

TEST_F(UnitTestWaffleIndexManager, kNNFourvertices)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    double start_lat = -5.2;
    double start_lon = -50.6;
    double end_lat = 5.9;
    double end_lon = 60.8;

    const Object object0(0, start_lat, start_lon);
    const Object object1(1, start_lat, end_lon);
    const Object object2(2, end_lat, start_lon);
    const Object object3(3, end_lat, end_lon);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = 0.1928;
    double query_lon = -0.4837;
    const int k = 4;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNLeftTopCorner)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, -0.13398, 49.0739);
    const Object object1(1, -8.83681, 85.1543);
    const Object object2(2, 5.4185, 41.7643);
    const Object object3(3, 6.83886, 87.1059);
    const Object object4(4, -9.50833, 70.9576);
    const Object object5(5, 4.25251, -58.0442);
    const Object object6(6, 6.9404, -95.69);
    const Object object7(7, -1.72584, 46.576);
    const Object object8(8, 2.71726, -39.8284);
    const Object object9(9, 2.48006, -92.3379);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = -10;
    double query_lon = -100;
    double start_lat = -10;
    double start_lon = -100;
    double end_lat = 0.2857;
    double end_lon = 97.3824;
    const int k = 4;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNLeftBottomCorner)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 3.95433, 99.8246);
    const Object object1(1, 4.50406, 24.3903);
    const Object object2(2, -5.14192, -16.0343);
    const Object object3(3, -7.69324, -44.1184);
    const Object object4(4, 8.76054, -50.5276);
    const Object object5(5, -3.03763, -12.0437);
    const Object object6(6, -0.778451, -75.2157);
    const Object object7(7, -8.80534, -88.038);
    const Object object8(8, -2.46973, -48.4387);
    const Object object9(9, -5.66381, 52.671);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = 9.9999;
    double query_lon = -100;
    double start_lat = 3.2711;
    double start_lon = query_lon;
    double end_lat = query_lat;
    double end_lon = 97.3824;
    const int k = 2;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNRightTopCorner)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 9.63364, 32.6032);
    const Object object1(1, -3.84268, -6.42317);
    const Object object2(2, -6.11172, -23.5326);
    const Object object3(3, -3.0532, 59.8203);
    const Object object4(4, -3.1623, -24.1414);
    const Object object5(5, -6.56256, 57.8996);
    const Object object6(6, 2.48939, -56.6729);
    const Object object7(7, 9.66064, 91.6168);
    const Object object8(8, 3.14506, -59.8189);
    const Object object9(9, 7.87394, -97.2019);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = -10;
    double query_lon = 99.9999;
    double start_lat = -9.3284;
    double start_lon = -98.7901;
    double end_lat = 7.3322;
    double end_lon = 99.9999;
    const int k = 7;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}

TEST_F(UnitTestWaffleIndexManager, kNNRightBottomCorner)
{
    WaffleIndexManager* index = initialize_index();
    std::vector<Lock> current_locks;
    bool from_regrid = false;

    const Object object0(0, 0.682534, -43.9585);
    const Object object1(1, -4.83971, -84.8823);
    const Object object2(2, -9.30486, 86.628);
    const Object object3(3, 3.85, 78.3418);
    const Object object4(4, -4.98277, -76.2705);
    const Object object5(5, -9.24602, 77.8793);
    const Object object6(6, 1.1237, 75.9341);
    const Object object7(7, 0.59469, -40.1215);
    const Object object8(8, -5.85975, -86.1963);
    const Object object9(9, -7.61724, 76.3559);

    std::vector<Object> objects;
    objects.push_back(object0);
    objects.push_back(object1);
    objects.push_back(object2);
    objects.push_back(object3);
    objects.push_back(object4);
    objects.push_back(object5);
    objects.push_back(object6);
    objects.push_back(object7);
    objects.push_back(object8);
    objects.push_back(object9);

    EXPECT_EQ(index->insertion(object0, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object1, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object2, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object3, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object4, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object5, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object6, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object7, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object8, current_locks, from_regrid), true);
    EXPECT_EQ(index->insertion(object9, current_locks, from_regrid), true);
    for (Lock lock: current_locks) {
        index->get_lock_manager()->unlock(lock.get_chunk_id(), lock.get_lock_type());
    }
    current_locks.clear();

    double query_lat = 9.9999;
    double query_lon = 99.9999;
    double start_lat = -3.1129;
    double start_lon = -100;
    double end_lat = 9.9999;
    double end_lon = 99.9999;
    const int k = 4;
    std::vector<IDType> result;
    index->kNN(k, query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, result, current_locks);
    std::vector<IDType> correct_result;
    get_correct_results_for_knn_query(query_lat, query_lon, start_lat, start_lon, end_lat, end_lon, objects, correct_result);
    EXPECT_EQ(result.size(), k);
    EXPECT_EQ(correct_result.size(), k);
    for (int i = 0; i < k; i++) {
        EXPECT_EQ(result[i], correct_result[i]);
    }
}
