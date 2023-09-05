cmake_minimum_required(VERSION 3.10)

project(multiboot CXX)

add_library(multiboot INTERFACE)

target_include_directories(
        multiboot INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(multiboot INTERFACE -Wall -Wextra)

install(
        TARGETS multiboot
        EXPORT multibootTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT multibootTargets
        FILE multibootTargets.cmake
        DESTINATION lib/cmake/multiboot
)

install(FILES include/multiboot/multiboot.hpp DESTINATION include/multiboot/)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES multibootConfig.cmake DESTINATION lib/cmake/multiboot)

export(TARGETS multiboot FILE multibootTargets.cmake)
