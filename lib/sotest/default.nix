{ stdenv, cmake, logger }:
stdenv.mkDerivation {
  name = "lib-sotest";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ logger ];
  src = ./.;
}
