self: super:

let
  tests = super.callPackage ./release.nix {
    pkgs = super;
  };
in
tests
