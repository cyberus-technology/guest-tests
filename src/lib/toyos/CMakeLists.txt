include(code-quality)

if(BUILD_TESTING)
  include(build-host.cmake)
endif()

include(build-nostd.cmake)
