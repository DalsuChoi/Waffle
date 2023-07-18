#include "Waffle.h"

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        std::cerr << "1. Query file path (absolute path)" << std::endl;
        std::cerr << "2. The maximum number of objects" << std::endl;
        return EXIT_FAILURE;
    }

    std::thread interface_thread{InterfaceService()};
    interface_thread.detach();

    std::cout << "Waiting for User Preference" << std::endl;

    std::unique_lock<std::mutex> lock(Waffle::m_start_waffle);
    Waffle::CV_start_waffle.wait(lock);
    lock.unlock();

    std::string query_file_path(argv[1]);
    std::string arg2(argv[2]);
    uint64_t num_total_objects = std::stoull(arg2);

    Waffle db(query_file_path, num_total_objects);

    return EXIT_SUCCESS;
}