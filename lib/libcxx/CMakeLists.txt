cmake_minimum_required(VERSION 3.10)

project(libcxx CXX ASM)

find_package(libsn REQUIRED)
find_package(compiler REQUIRED)
find_package(string_backend REQUIRED)

add_library(libcxx STATIC
        src/c_string.cpp
        src/exception.cpp
        src/locale.cpp
        src/memory.cpp
        src/setjmp.S
        src/stdexcept.cpp
        src/string.cpp
        src/vector.cpp
)

target_include_directories(
        libcxx PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(libcxx PRIVATE -Wall -Wextra)

target_link_libraries(libcxx PUBLIC compiler libsn string_backend)

install(
        TARGETS libcxx
        EXPORT libcxxTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(
        EXPORT libcxxTargets
        FILE libcxxTargets.cmake
        DESTINATION lib/cmake/libcxx
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES libcxxConfig.cmake
        DESTINATION lib/cmake/libcxx
)

export(TARGETS libcxx FILE libcxxTargets.cmake)
