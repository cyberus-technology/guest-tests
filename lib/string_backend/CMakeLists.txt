cmake_minimum_required(VERSION 3.10)

project(string_backend CXX)

find_package(compiler REQUIRED)
find_package(libsn REQUIRED)

add_library(string_backend STATIC
        string.cpp
        strtox.cpp
)

target_include_directories(
        string_backend PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(string_backend PRIVATE -Wall -Wextra)

target_link_libraries(string_backend PUBLIC compiler libsn)

install(
        TARGETS string_backend
        EXPORT string_backendTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

# install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)
install(FILES include/string_backend DESTINATION include)

install(
        EXPORT string_backendTargets
        FILE string_backendTargets.cmake
        DESTINATION lib/cmake/string_backend
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES string_backendConfig.cmake
        DESTINATION lib/cmake/string_backend
)

export(TARGETS string_backend FILE string_backendTargets.cmake)
