{ stdenv, cmake, x86 }:
stdenv.mkDerivation {
  name = "lib-config";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ x86 ];
  src = ./.;
}
