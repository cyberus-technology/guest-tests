cmake_minimum_required(VERSION 3.10)

project(udis86-contrib)

set(udis86-itab-path ${CMAKE_CURRENT_BINARY_DIR}/libudis86)
set(udis86-directory ${CMAKE_CURRENT_SOURCE_DIR}/udis86)

add_custom_command(
        OUTPUT ${udis86-itab-path}/itab.h ${udis86-itab-path}/itab.c
        MAIN_DEPENDENCY ${udis86-directory}/docs/x86/optable.xml
        COMMAND ${CMAKE_COMMAND} ARGS -E make_directory ${udis86-itab-path}
        COMMAND python3 ARGS -B ${udis86-directory}/scripts/ud_itab.py
        ${udis86-directory}/docs/x86/optable.xml ${udis86-itab-path}
)

add_library(
        udis86-contrib STATIC
        ${udis86-directory}/libudis86/decode.c
        ${udis86-directory}/libudis86/syn-att.c
        ${udis86-directory}/libudis86/syn.c
        ${udis86-directory}/libudis86/syn-intel.c
        ${udis86-directory}/libudis86/udis86.c
        ${udis86-itab-path}/itab.c
)

target_compile_features(udis86-contrib PUBLIC cxx_std_17)
target_compile_definitions(udis86-contrib PUBLIC __UD_STANDALONE__)
target_compile_options(
        udis86-contrib
        PUBLIC -iquote ${udis86-itab-path}
        PRIVATE -Wno-implicit-fallthrough
)

target_include_directories(
        udis86-contrib
        PUBLIC $<BUILD_INTERFACE:${udis86-directory}/libudis86/>
        $<BUILD_INTERFACE:${udis86-directory}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>
        $<BUILD_INTERFACE:${udis86-itab-path}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
        udis86-contrib
        PUBLIC libc-tiny libcxx
)

add_custom_target(
        udis86-itab-header-generated DEPENDS ${udis86-itab-path}/itab.c
        ${udis86-itab-path}/itab.h
)

add_dependencies(udis86-contrib udis86-itab-header-generated)

install(
        TARGETS udis86-contrib
        EXPORT udis86-contribConfig
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES
        DESTINATION include
)

install(
        EXPORT udis86-contribConfig
        FILE udis86-contribConfig.cmake
        DESTINATION lib/cmake/udis86
)

install(FILES udis86/udis86.h DESTINATION include/)
install(
        FILES udis86/libudis86/decode.h udis86/libudis86/extern.h
        udis86/libudis86/syn.h udis86/libudis86/types.h udis86/libudis86/udint.h
        ${udis86-itab-path}/itab.h DESTINATION include/libudis86
)

export(TARGETS udis86-contrib FILE udis86-contribConfig.cmake)
