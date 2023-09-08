let
  pkgs = import ./cbspkgs.nix;
in
{
  inherit (pkgs.cyberus.guest-tests) tests;

  # Add additional CI relevant attributes here.
}
