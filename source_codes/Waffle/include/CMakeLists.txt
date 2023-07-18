project(include)

set(HEADER_FILES
        Chunk.h
        Waffle.h
        Parameters.h
        LockManager.h
        RegridManager.h
        GridIndexManager.h
        utils.h
        TransactionManager.h
        json.hpp)

set(SOURCE_FILES
        Chunk.cpp
        Waffle.cpp
        LockManager.cpp
        RegridManager.cpp
        GridIndexManager.cpp
        TransactionManager.cpp)

add_library(include STATIC ${SOURCE_FILES} ${HEADER_FILES})

find_package(Torch REQUIRED)
target_link_libraries(include PUBLIC "${TORCH_LIBRARIES}")

include_directories(WaffleMaker)
add_subdirectory(WaffleMaker)
target_link_libraries(include PUBLIC WaffleMaker)

include_directories(interface)
add_subdirectory(interface)
target_link_libraries(include PUBLIC interface)