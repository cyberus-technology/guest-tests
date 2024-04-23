# Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# gtbase: Guest Tests Base Library
#
# Header-only library with definitions that we need in our custom nostd-friendly
# libcxx and our libtoyos. These definitions do not clash with the header files
# of a typical standard libc, so that we can use these definitions in hosted
# unit tests.

add_library(gtbase INTERFACE)

target_include_directories(
  gtbase INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS gtbase
  EXPORT gtbaseTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )
