{ stdenv, cmake, emulator, hedron, logger, math, vmmu, x86 }:
stdenv.mkDerivation {
  name = "lib-vcpu";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ hedron logger math vmmu x86 ];
  buildInputs = [ emulator ];
  src = ./.;
}
