{ stdenv, cmake, cbl, googlemock, math }:
stdenv.mkDerivation {
  name = "lib-elf";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ cbl math ];

  checkInputs = [ googlemock ];
  src = ./.;

  doCheck = true;
}
