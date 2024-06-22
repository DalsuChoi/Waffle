#include "RegridManager.h"
#include "WaffleIndexManager.h"
#include "TransactionManager.h"
#include "Waffle.h"

RegridManager::RegridManager(WaffleIndexManager *&original_index, WaffleIndexManager *&new_index)
    : original_index(original_index), new_index(new_index)
{
}

void RegridManager::operator()()
{
    Waffle::WaffleMaker_wait_for_reward = false;
    Waffle::WaffleMaker_CV.notify_all();

    if (Waffle::exit_experiment)
    {
        Waffle::set_during_regrid(false);
        Waffle::set_prepare_regrid(false);
        return;
    }

    std::unique_lock<std::mutex> lock2(Waffle::WaffleMaker_Mutex2);
    std::unique_lock<std::mutex> lock(Waffle::WaffleMaker_Mutex);

    if (Waffle::stop_regrid)
    {
        Waffle::set_during_regrid(false);
        Waffle::set_prepare_regrid(false);
        Waffle::regrid_CV.notify_all();
        Waffle::stop_regrid = false;
        return;
    }

    while (Waffle::WaffleMaker_writing_new_knob_setting)
    {
        Waffle::WaffleMaker_CV.wait(lock);
        if (Waffle::exit_experiment)
        {
            Waffle::set_during_regrid(false);
            Waffle::set_prepare_regrid(false);
            Waffle::regrid_CV.notify_all();
            return;
        }
    }

    std::cout << std::endl
              << "\033[1;31m"
              << "Regrid Starts"
              << "\033[0m" << std::endl;

    std::unique_lock<std::mutex> lock_current_knob_setting(Waffle::m_current_knob_setting);
    int new_nCell_space_lat = Waffle::WaffleMaker_nCell_space_lat;
    int new_nCell_space_lon = Waffle::WaffleMaker_nCell_space_lon;
    int new_MOPC = Waffle::WaffleMaker_MOPC;
    int new_nCell_chunk_lat = Waffle::WaffleMaker_nCell_chunk_lat;
    int new_nCell_chunk_lon = Waffle::WaffleMaker_nCell_chunk_lon;
    lock_current_knob_setting.unlock();

    Waffle::WaffleMaker_writing_new_knob_setting = true;
    lock2.unlock();
    lock.unlock();

    new_index = new WaffleIndexManager(new_nCell_space_lat, new_nCell_space_lon, new_MOPC, new_nCell_chunk_lat, new_nCell_chunk_lon,
                                        NEW_INDEX, Waffle::num_total_objects);

    new_index->set_internal_parameters(original_index->get_min_lat(), original_index->get_min_lon(),
                                       original_index->get_max_lat(), original_index->get_max_lon());

    Waffle::set_during_regrid(true);
    Waffle::set_prepare_regrid(false);
    Waffle::regrid_CV.notify_all();

    auto chunks = original_index->chunks;
    int old_MOPC = original_index->get_MOPC();
    int old_nCell_chunk_lat = original_index->get_nCell_chunk_lat();
    int old_nCell_chunk_lon = original_index->get_nCell_chunk_lon();
    int old_unique_chunk_positions = original_index->get_num_chunks_lat() * original_index->get_num_chunks_lon();
    const int old_nCell_chunk = old_nCell_chunk_lat * old_nCell_chunk_lon;

    for (int old_chunk_ID = 0; old_chunk_ID < old_unique_chunk_positions; old_chunk_ID++)
    {

        int cp_start = old_chunk_ID * 2;
        bool go_to_the_next_chunk = false;

        for (int cell_ID = 0; cell_ID < old_nCell_chunk; cell_ID++)
        {
            int start = cell_ID * old_MOPC;

            while (true)
            {
                Transaction transfer(original_index, new_index);
                transfer.getXLock(ORIGINAL_INDEX, old_chunk_ID);
                Chunk *&chunk_start = chunks[cp_start];

                if (chunk_start == nullptr)
                {
                    transfer.commit();
                    go_to_the_next_chunk = true;
                    break;
                }

                const int *num_objects = chunk_start->get_num_objects();
                if (num_objects[cell_ID] == 0)
                {
                    transfer.commit();
                    break;
                }
                auto objects = chunk_start->get_objects();
                Object object = objects[start];

                IDType object_id = object.get_ID();

                Waffle::regrid_object = object_id;
                if (Waffle::client_object == Waffle::regrid_object)
                {
                    Waffle::regrid_object = INTEGER_MIN;
                    transfer.commit();
                    Waffle::CV_same_object.notify_all();
                    continue;
                }

                transfer.deletion(ORIGINAL_INDEX, object, true);
                transfer.insertion(NEW_INDEX, object, true);

                Waffle::regrid_object = INTEGER_MIN;
                transfer.commit();
                Waffle::CV_same_object.notify_all();
            }

            if (go_to_the_next_chunk)
            {
                break;
            }
        }
    }

    Waffle::set_prepare_regrid(true);

    while (Waffle::num_running_query != 0)
    {
    }

    delete original_index;
    original_index = new_index;
    original_index->set_role(ORIGINAL_INDEX);
    new_index = nullptr;

    std::unique_lock<std::mutex> lock_time(Waffle::m_time);
    Waffle::regrid_finish_time = std::chrono::high_resolution_clock::now();
    lock_time.unlock();

    std::cout << "\033[1;31m"
              << "Regrid Ends"
              << "\033[0m" << std::endl;

    Waffle::total_regrids++;
    Waffle::set_during_regrid(false);
    Waffle::set_prepare_regrid(false);
}