{ stdenv, cmake, gtest }:
stdenv.mkDerivation {
  name = "lib-acpi";
  nativeBuildInputs = [ cmake ];
  checkInputs = [ gtest ];
  src = ./.;

  doCheck = true;
}
