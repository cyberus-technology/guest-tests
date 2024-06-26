/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
{ pkgs ? import ./public/nixpkgs.nix }:

pkgs.lib.mergeAttrs
  (import ./public/release.nix { inherit pkgs; })
  (if pkgs ? cyberus then
  # This is the case if we pass in a pkgs version with applied cyberus overlays.
    (import ./cyberus/release.nix { inherit pkgs; })
  else
    (import ./cyberus/release.nix { /* creates its own instance */ })
  )
