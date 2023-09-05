{ stdenv, cmake, compiler, pprintpp }:
stdenv.mkDerivation {
  name = "lib-logger";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ compiler pprintpp ];
  src = ./.;
}
