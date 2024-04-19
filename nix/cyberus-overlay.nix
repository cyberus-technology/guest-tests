final: _prev:

let
  project = import ./release.nix { pkgs = final; };
  makeSotest = { name, category }:
    import ./sotest.nix {
      pkgs = final;
      inherit name category;
    };

in
{
  inherit (project) tests;
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
