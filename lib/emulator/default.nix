{ stdenv, cmake, decoder }:
stdenv.mkDerivation {
  name = "lib-emulator";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ decoder ];
  src = ./.;
}
