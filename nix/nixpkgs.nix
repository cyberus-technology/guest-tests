/*
  Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

let
  sources = import ./sources.nix;
in
import sources.nixpkgs { }
