{ stdenv, cmake }:
stdenv.mkDerivation {
  name = "lib-compiler";
  nativeBuildInputs = [ cmake ];
  src = ./.;
}
