cmake_minimum_required(VERSION 3.10)

project(utis86 CXX)

add_library(utis86 INTERFACE)

target_include_directories(
        utis86 INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(
        utis86 INTERFACE
)

target_link_libraries(utis86 INTERFACE utis86-contrib)

install(
        TARGETS utis86
        EXPORT utis86Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT utis86Targets
        FILE utis86Targets.cmake
        DESTINATION lib/cmake/utis86
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES utis86Config.cmake DESTINATION lib/cmake/utis86)

export(TARGETS utis86 FILE utis86Targets.cmake)
