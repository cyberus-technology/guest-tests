/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

# This file must be in nix/cyberus-overlay.nix by convention for the cbspkgs
# pipeline.

final: _prev:

let
  publicProject = import ./public/release.nix { pkgs = final; };
  makeSotest = { name, category }:
    import ./cyberus/sotest.nix {
      pkgs = final;
      inherit name category;
    };

in
{
  inherit (publicProject) tests;
  sotests = {
    # SoTest bundle with all tests.
    default = makeSotest {
      name = "guest-tests: all tests";
      category = "default";
    };
    # Single smoke test
    smoke = (makeSotest {
      name = "guest-tests: hello-world smoke test";
      category = "smoke";
    }).singleRuns.hello-world;
  };
}
