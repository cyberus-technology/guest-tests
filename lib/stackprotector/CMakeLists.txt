cmake_minimum_required(VERSION 3.10)

project(stackprotector CXX)


add_library(stackprotector STATIC src/stackprotector.c
)

target_include_directories(
        stackprotector PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(stackprotector PUBLIC -Wall -Wextra -Wno-stack-protector)

install(
        TARGETS stackprotector
        EXPORT stackprotectorTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT stackprotectorTargets
        FILE stackprotectorTargets.cmake
        DESTINATION lib/cmake/stackprotector
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES stackprotectorConfig.cmake
        DESTINATION lib/cmake/stackprotector
)

export(TARGETS stackprotector FILE stackprotectorTargets.cmake)
