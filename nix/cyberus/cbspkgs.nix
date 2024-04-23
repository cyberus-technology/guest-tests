/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

let
  sources = import ../sources.nix;
  cbspkgs = import sources.cbspkgs { };
  vanillaPkgs = import cbspkgs.nixpkgs { };
in
import cbspkgs.nixpkgs {
  system = "x86_64-linux";

  overlays = cbspkgs.overrideOverlaySources {
    guest-tests = vanillaPkgs.nix-gitignore.gitignoreSource [ ] ./../..;
  };
}
