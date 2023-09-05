cmake_minimum_required(VERSION 3.10)

project(pci CXX)

find_package(cbl REQUIRED)
find_package(logger REQUIRED)

add_library(pci INTERFACE)

target_include_directories(
        pci INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(pci INTERFACE -Wall -Wextra)

target_link_libraries(pci INTERFACE cbl logger)

install(
        TARGETS pci
        EXPORT pciTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT pciTargets
        FILE pciTargets.cmake
        DESTINATION lib/cmake/pci
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES pciConfig.cmake DESTINATION lib/cmake/pci)

export(TARGETS pci FILE pciTargets.cmake)
