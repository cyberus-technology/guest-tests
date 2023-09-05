{ stdenv, cmake }:
stdenv.mkDerivation {
  name = "lib-math";
  nativeBuildInputs = [ cmake ];
  src = ./.;
}
