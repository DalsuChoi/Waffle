project(interface)

set(INTERFACE_HEADER_FILES
        InterfaceResource.h
        InterfaceService.h
        InterfaceServiceSettings.h)

set(INTERFACE_SOURCE_FILES
        InterfaceResource.cpp
        InterfaceService.cpp
        InterfaceServiceSettings.cpp)

add_library(interface STATIC ${INTERFACE_SOURCE_FILES} ${INTERFACE_HEADER_FILES})

find_package(Torch REQUIRED)
target_link_libraries(interface PUBLIC "${TORCH_LIBRARIES}")
target_link_libraries(interface PUBLIC restbed)