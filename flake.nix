{
  description = "Cyberus Guest Tests";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-parts.inputs.nixpkgs-lib.follows = "nixpkgs";
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
    nixpkgs-23-11.url = "github:nixos/nixpkgs/nixos-23.11";
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
        { pkgs, system, ... }:
        let
          project = import ./nix/release.nix {
            inherit pkgs;
            pkgs-23-11 = import inputs.nixpkgs-23-11 { inherit system; };
          };
        in
        {
          devShells = {
            # Shell for this repository.
            default = pkgs.mkShell {
              inputsFrom = builtins.attrValues self.packages.${system};
              packages = with pkgs; [
                ninja
              ];
            };
          };
          formatter = pkgs.nixfmt-rfc-style;
          packages =
            let
              project = import ./default.nix {
                inherit pkgs;
                pkgs-23-11 = import inputs.nixpkgs-23-11 { inherit system; };
              };
            in
            {
              inherit (project) guest-tests;
              default = project.guest-tests;
            };
        };
    };
}
