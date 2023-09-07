{ sources ? import ./sources.nix
, pkgs ? import sources.nixpkgs {}
}:

{
  tests = pkgs.callPackage ./tests.nix { gitignoreNixSrc = sources."gitignore.nix"; };
}
