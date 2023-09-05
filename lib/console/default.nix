{ stdenv, cmake, compiler, hedron }:
stdenv.mkDerivation {
  name = "lib-console";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ compiler hedron ];
  src = ./.;
}
