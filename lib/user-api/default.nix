{ stdenv, cmake, hedron }:
stdenv.mkDerivation {
  name = "lib-user-api";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ hedron ];
  src = ./.;
}
