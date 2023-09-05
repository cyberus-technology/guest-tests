{ stdenv, cmake, cbl, compiler, googlemock, logger, math, pprintpp }:
stdenv.mkDerivation {
  name = "lib-x86";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ cbl compiler logger math pprintpp ];
  checkInputs = [ googlemock ];
  src = ./.;
  doCheck = true;
}
