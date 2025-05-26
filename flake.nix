{
  description = "Cyberus Guest Tests";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-parts.inputs.nixpkgs-lib.follows = "nixpkgs";
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";
    nixpkgs-23-11.url = "github:nixos/nixpkgs/nixos-23.11";
    pre-commit-hooks.url = "github:cachix/git-hooks.nix";
    pre-commit-hooks.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    { self, flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      flake = {
        # non per-system flake attributes
      };
      # No artificially limit here. If it builds, it builds.
      systems = inputs.nixpkgs.lib.systems.flakeExposed;
      perSystem =
        {
          pkgs,
          lib,
          system,
          ...
        }:
        let
          project = import ./nix/release.nix {
            inherit pkgs;
            pkgs-23-11 = import inputs.nixpkgs-23-11 { inherit system; };
          };
        in
        {
          checks = {
            pre-commit = import ./nix/pre-commit-check.nix {
              pre-commit-hooks = inputs.pre-commit-hooks.lib.${system};
              inherit pkgs;
            };
          };
          devShells = {
            # Shell for this repository.
            default = pkgs.mkShell {
              inputsFrom = builtins.attrValues self.packages.${system};
              packages =
                with pkgs;
                [
                  clang-tools # format and tidy
                  ninja
                ]
                ++ self.checks.${system}.pre-commit.enabledPackages;
              shellHook = ''
                ${self.checks.${system}.pre-commit.shellHook}
              '';
            };
          };
          formatter = pkgs.nixfmt-rfc-style;
          packages =
            let
              # Flattens a nested attribute set recursively, by constructing a
              # new flat attribute key from the whole path.
              flattenAttrsByName =
                attrSet: prefix: sep:
                lib.concatMapAttrs (
                  key: value:
                  let
                    fullKey = "${prefix}${sep}${key}";
                  in
                  if lib.isAttrs value && !lib.isDerivation value then
                    flattenAttrsByName value fullKey sep
                  else
                    { ${fullKey} = value; }
                ) attrSet;
              flattenedTestRuns = flattenAttrsByName project.testRuns "testRun" "_";
            in
            {
              inherit (project) guest-tests;
              default = project.guest-tests;
            }
            // flattenedTestRuns;
        };
    };
}
