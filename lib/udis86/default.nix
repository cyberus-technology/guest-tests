{ stdenv, cmake, python3, runCommand }:
stdenv.mkDerivation {
  name = "lib-udis86";
  nativeBuildInputs = [ cmake python3 ];
  src = runCommand "udis86-src"
    {
      cmakeFile = ./CMakeLists.txt;
      sourceDir = ../../../contrib/udis86;
    }
    ''
      mkdir -p $out
      ln -s $cmakeFile $out/CMakeLists.txt
      ln -s $sourceDir $out/udis86
    '';
}
