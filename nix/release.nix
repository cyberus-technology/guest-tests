/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
{ pkgs ? import ./public/nixpkgs.nix }:

import ./public/release.nix { inherit pkgs; }
