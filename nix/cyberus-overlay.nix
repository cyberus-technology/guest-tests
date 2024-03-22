final: _prev:

let
  makeSotest = { name, category }:
    import ./sotest.nix {
      pkgs = final;
      inherit name category;
    };

in
{
  # This is just an attribute set, so we don't use callPackage. Further, it is
  # important that no `override` attribute exists here, so that the test names
  # are just the attrNames of the attribute set.
  tests = import ./build.nix {
    inherit (final) stdenv
      callPackage
      catch2_3
      cyberus
      cmake
      gcc11
      nix-gitignore
      runCommand;
  };
  sotests = {
    # SoTest bundle with all tests.
    default = makeSotest {
      name = "guest-tests: all tests";
      category = "default";
    };
    # Single smoke test
    smoke = (makeSotest {
      name = "guest-tests: hello-world smoke test";
      category = "smoke";
    }).singleRuns.hello-world;
  };
}
