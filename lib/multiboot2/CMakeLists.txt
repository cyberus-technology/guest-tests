cmake_minimum_required(VERSION 3.10)

project(multiboot2 CXX)

add_library(multiboot2 INTERFACE)

target_include_directories(
        multiboot2 INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(multiboot2 INTERFACE -Wall -Wextra)

install(
        TARGETS multiboot2
        EXPORT multiboot2Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT multiboot2Targets
        FILE multiboot2Targets.cmake
        DESTINATION lib/cmake/multiboot2
)

install(FILES
        include/multiboot2/multiboot2.hpp
        include/multiboot2/multiboot2_builder.hpp
        DESTINATION include/multiboot2)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES multiboot2Config.cmake DESTINATION lib/cmake/multiboot2)

export(TARGETS multiboot2 FILE multiboot2Targets.cmake)
