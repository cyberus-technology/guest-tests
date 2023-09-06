cmake_minimum_required(VERSION 3.10)

project(printf CXX)

# TODO julian: ich brauch den ganzen find_package kram net
find_package(compiler)
find_package(libc-tiny)
find_package(libcxx)

add_library(printf STATIC backend.cpp xprintf.c)

target_include_directories(
        printf PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(printf PRIVATE -Wall -Wextra)

target_link_libraries(printf PUBLIC compiler libc-tiny libcxx)

install(
        TARGETS printf
        EXPORT printfTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT printfTargets
        FILE printfTargets.cmake
        DESTINATION lib/cmake/printf
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/printf
        DESTINATION include/printf
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES printfConfig.cmake DESTINATION lib/cmake/printf)

export(TARGETS printf FILE printfTargets.cmake)
