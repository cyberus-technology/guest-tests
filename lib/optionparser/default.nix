{ stdenv, cmake, cbl, logger, optionparser-contrib }:
stdenv.mkDerivation {
  name = "lib-optionparser";
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ cbl logger optionparser-contrib ];
  src = ./.;
}
