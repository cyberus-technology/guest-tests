{ stdenv, cmake, gtest, runCommand }:
stdenv.mkDerivation {
  name = "lib-slvm-api";
  nativeBuildInputs = [ cmake ];
  checkInputs = [ gtest ];
  # The hedron includes are shared with lib hedron, in order to do not
  # duplicate the files we need to link them properly.
  # With nix, they are outside the actual repository and arent't copied
  # correctly, so we need to make sure the headers are present.
  src = runCommand "slvm-api-src"
    {
      currentDir = ./.;
      hedronIncludes = ../hedron/include/hedron;
      cblIncludes = ../cbl/include/cbl;
      slvmInfoIncludes = ../slvm-info/include;
    } ''
    mkdir -p $out/include/slvm
    cp $currentDir/CMakeLists.txt $out
    cp $currentDir/slvm-apiConfig.cmake $out
    cp -r $currentDir/include/slvm $out/include/
    cp -r $currentDir/test $out

    copyDependencies(){
      includeName=$1
      includeDir=$2
      currentDir=$3
      mkdir -p $out/include/$includeName
      for file in $currentDir/include/$includeName/*
      do
        cp $includeDir/$(basename $file) $out/include/$includeName/
      done

    }

    copyDependencies hedron $hedronIncludes $currentDir
    copyDependencies cbl $cblIncludes $currentDir
    copyDependencies slvm-info $slvmInfoIncludes $currentDir

  '';

  doCheck = true;
}
