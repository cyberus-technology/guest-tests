{ stdenv, cmake, writeScript, gtest }:
let
  cmakeLists = writeScript "CMakeLists.txt" ''
    cmake_minimum_required(VERSION 3.10)
    project(googlemock CXX)

    find_package(GTest REQUIRED)

    add_library(gmock_lib INTERFACE)
    target_include_directories(gmock_lib INTERFACE ${gtest.dev}/include)
    target_link_directories(gmock_lib INTERFACE ${gtest}/lib)
    target_link_libraries(gmock_lib INTERFACE gmock GTest::GTest)

    add_library(gmock_main INTERFACE)
    target_include_directories(gmock_main INTERFACE ${gtest.dev}/include)
    target_link_directories(gmock_main INTERFACE ${gtest}/lib)
    target_link_libraries(gmock_main INTERFACE gmock_main gmock_lib GTest::Main)

    set(GTEST_LIBRARIES gtest)
    set(GTEST_MAIN_LIBRARIES gtest_main)
    set(GMOCK_LIBRARIES gmock_lib gmock_main)
    set(GTEST_BOTH_LIBRARIES ''${GTEST_LIBRARIES} ''${GTEST_MAIN_LIBRARIES})

    install(
      TARGETS gmock_lib gmock_main
      EXPORT googlemockTargets
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      RUNTIME DESTINATION lib
      INCLUDES DESTINATION include
    )

    install(DIRECTORY ${gtest.dev}/include DESTINATION include)

    install(
      EXPORT googlemockTargets
      FILE googlemockTargets.cmake
      DESTINATION lib/cmake/googlemock
    )

    install(FILES googlemockConfig.cmake DESTINATION lib/cmake/googlemock)

    export(TARGETS gmock_lib gmock_main FILE googlemockTargets.cmake)
  '';
in
stdenv.mkDerivation {
  name = "googlemock";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ gtest ];
  src = ./.;
  doCheck = true;

  patchPhase = ''
    cp ${cmakeLists} ./CMakeLists.txt
  '';
}
