{ stdenv, cmake, runCommand }:
stdenv.mkDerivation {
  name = "lib-pprintpp";
  nativeBuildInputs = [ cmake ];
  src = runCommand "pprintpp-src"
    {
      cmakeFile = ./CMakeLists.txt;
      cmakeConfigFile = ./pprintppConfig.cmake;
      includeDir = ../../../contrib/pprintpp/include;
    }
    ''
      mkdir -p $out
      ln -s $cmakeFile $out/CMakeLists.txt
      ln -s $cmakeConfigFile $out/pprintppConfig.cmake
      ln -s $includeDir $out/include
    '';
}
