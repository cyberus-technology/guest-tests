{ stdenv, cbl, cmake, config, logger, math, x86 }:
stdenv.mkDerivation {
  name = "lib-memory";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ cbl config logger math x86 ];
  src = ./.;
}
