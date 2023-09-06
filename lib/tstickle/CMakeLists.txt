cmake_minimum_required(VERSION 3.10)

project(tstickle CXX ASM)

find_package(x86 REQUIRED)

add_library(tstickle STATIC
        entry.S
        ioapic.cpp
        irq_handler.cpp
        lapic_test_tools.cpp
)

target_include_directories(
        tstickle PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(tstickle PRIVATE -Wall -Wextra)

target_link_libraries(tstickle PUBLIC x86)

install(
        TARGETS tstickle
        EXPORT tstickleTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(
        EXPORT tstickleTargets
        FILE tstickleTargets.cmake
        DESTINATION lib/cmake/tstickle
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES tstickleConfig.cmake
        DESTINATION lib/cmake/tstickle
)

export(TARGETS tstickle FILE tstickleTargets.cmake)
