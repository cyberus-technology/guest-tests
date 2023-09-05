{ stdenv, cmake, first-fit-heap-contrib }:
stdenv.mkDerivation {
  name = "lib-first-fit-heap";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ first-fit-heap-contrib ];
  src = ./.;
}
