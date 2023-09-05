{ stdenv, cmake, vmmu-contrib }:
stdenv.mkDerivation {
  name = "lib-vmmu";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ vmmu-contrib ];
  src = ./.;
}
