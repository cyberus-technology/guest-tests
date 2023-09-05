{ stdenv, cmake, googlemock, logger, math }:
stdenv.mkDerivation {
  name = "lib-cbl";
  nativeBuildInputs = [ cmake ];
  # To allow transitive library includes for interface libraries
  # with nix we have to propagate the buildInputs to dependent libraries too.
  propagatedBuildInputs = [ logger math ];

  checkInputs = [ googlemock ];
  src = ./.;

  doCheck = true;
}
