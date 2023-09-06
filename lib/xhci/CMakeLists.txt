cmake_minimum_required(VERSION 3.10)

project(xhci CXX)

add_library(xhci STATIC
        console_base.cpp
        debug_device.cpp
        debug_device_transfers.cpp
        device.cpp
)

find_package(console REQUIRED)
find_package(memory REQUIRED)
find_package(printf REQUIRED)

target_include_directories(
        xhci PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(xhci PRIVATE -Wall -Wextra)

target_link_libraries(xhci PUBLIC console memory printf)

install(
        TARGETS xhci
        EXPORT xhciTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT xhciTargets
        FILE xhciTargets.cmake
        DESTINATION lib/cmake/xhci
)

install(
        FILES include/acpi.hpp
        include/baremetal_device_driver_adapter.hpp
        include/boot_cmdline.hpp
        include/irq_guard.hpp
        include/mm.hpp
        include/page_table_base.hpp
        include/paging_directory_entry_base.hpp
        include/pd.hpp
        include/pdpt.hpp
        include/pml4.hpp
        include/pt.hpp
        DESTINATION include
)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES xhciConfig.cmake DESTINATION lib/cmake/xhci)

export(TARGETS xhci FILE xhciTargets.cmake)
