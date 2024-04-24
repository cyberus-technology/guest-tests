final: _prev:

let
  publicProject = import ../public/release.nix { pkgs = final; };
  makeSotest = { name, category }:
    import ./sotest.nix {
      pkgs = final;
      inherit name category;
    };

in
{
  inherit (publicProject) tests;
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
