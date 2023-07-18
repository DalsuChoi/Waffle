project(WaffleMaker)

set(WAFFLEMAKER_HEADER_FILES
        WaffleMaker.h
        model.h
        utils.h)

set(WAFFLEMAKER_SOURCE_FILES
        WaffleMaker.cpp
        model.cpp
        utils.cpp)

add_library(WaffleMaker STATIC ${WAFFLEMAKER_SOURCE_FILES} ${WAFFLEMAKER_HEADER_FILES})

find_package(Torch REQUIRED)
target_link_libraries(WaffleMaker PUBLIC "${TORCH_LIBRARIES}")

find_package(TBB REQUIRED)
target_link_libraries(WaffleMaker PUBLIC TBB::tbb)