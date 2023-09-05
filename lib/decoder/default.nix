{ stdenv, cmake, cbl, compiler, first-fit-heap, logger, math, udis86, x86 }:
stdenv.mkDerivation {
  name = "lib-decoder";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ cbl compiler first-fit-heap logger math udis86 x86 ];
  src = ./.;
}
