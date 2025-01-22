/*
  Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

# Style and quality checks using pre-commit as centralized component

{ pkgs, pre-commit-hooks }:

let
  lib = pkgs.lib;
in
pre-commit-hooks.run {
  src = pkgs.nix-gitignore.gitignoreSource [ ] ../.;

  tools = pkgs;

  hooks = {
    clang-format = {
      enable = true;
      # Without mkForce this is appended and not replaced.
      types_or = lib.mkForce [
        "c"
        "c++"
      ];
      excludes = [
        "lib/libc-tiny"
        "lib/libcxx"
        "src/contrib"
      ];
    };

    cmake-format = {
      enable = true;
    };

    deadnix = {
      enable = true;
      excludes = [
        "nix/sources.nix"
      ];
      settings.noLambdaPatternNames = true;
    };

    nixfmt-rfc-style = {
      enable = true;
      excludes = [
        "nix/sources.nix"
      ];
    };

    typos = {
      enable = true;
      # TODO Remove this, depending on the outcome of
      # https://github.com/cachix/pre-commit-hooks.nix/pull/405
      pass_filenames = false;
      settings.configPath = ".typos.toml";
    };
  };
}
