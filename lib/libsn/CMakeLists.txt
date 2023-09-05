cmake_minimum_required(VERSION 3.10)

project(libsn CXX ASM)

add_library(libsn INTERFACE)

target_include_directories(
        libsn INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(
        TARGETS libsn
        EXPORT libsnTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(
        EXPORT libsnTargets
        FILE libsnTargets.cmake
        DESTINATION lib/cmake/libsn
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES libsnConfig.cmake
        DESTINATION lib/cmake/libsn
)

export(TARGETS libsn FILE libsnTargets.cmake)
