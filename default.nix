/*
  Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

# Alternate entry (legacy entry) into the Nix project. Initializes the
# dependencies from the flake.lock file.

let
  lock = builtins.fromJSON (builtins.readFile ./flake.lock);
  fetchInput =
    inputName:
    let
      nodeName = lock.nodes.root.inputs.${inputName};
      node = lock.nodes.${nodeName}.locked;
      url = "https://github.com/${node.owner}/${node.repo}/archive/${node.rev}.tar.gz";
    in
    fetchTarball {
      inherit url;
      sha256 = node.narHash;
    };
  pkgsSrc = fetchInput "nixpkgs";
  pkgs-23-11Src = fetchInput "nixpkgs-23-11";
  pkgs = import pkgsSrc { system = "x86_64-linux"; };
  pkgs-23-11 = import pkgs-23-11Src { system = "x86_64-linux"; };
in
import ./nix/release.nix {
  inherit pkgs pkgs-23-11;
}
