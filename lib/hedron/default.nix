{ stdenv, acpi, cbl, cmake, compiler, config, gtest, logger, math, memory, pprintpp, x86 }:
stdenv.mkDerivation {
  name = "lib-hedron";
  nativeBuildInputs = [ cmake ];
  buildInputs = [ acpi cbl compiler config logger math memory pprintpp x86 ];

  checkInputs = [ gtest ];
  src = ./.;

  doCheck = true;
}
