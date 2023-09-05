{ stdenv, cmake, cbl, config, hedron, x86 }:
stdenv.mkDerivation {
  name = "lib-slvm-info";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ cbl config hedron x86 ];
  src = ./.;
}
