/*
  Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

{
  pkgs,
  pkgs-23-11,
}:

let
  lib = pkgs.lib;

  # Nested structure with one test file in the given variant.
  #
  # We don't use callPackage here, as we do not want `override` and
  # `overrideAttrs` in the returned attrset.
  tests = import ./build.nix {
    inherit pkgs pkgs-23-11;
  };

  testNames = builtins.attrNames tests;

  # All tests in all variants combined.
  guest-tests =
    let
      variants = builtins.attrNames tests.hello-world;
      unflattenTest =
        name: variant:
        unflattenDrv {
          drv = tests.${name}.${variant};
          artifactPath = "${name}.${variant}";
        };
      unflattenDrv = import ./unflatten-drv.nix { inherit (pkgs) runCommandLocal; };
    in
    # For symlink join, we can't use flat derivations. We need unflattened ones.
    # Therefore, we map each test variant derivation to a new derivation in
    # unflattened form and then put those unflattened ones in a list.
    pkgs.symlinkJoin {
      name = "all-guest-tests";
      paths = pkgs.lib.flatten (
        map (name: [
          (map (variant: unflattenTest name variant) variants)
        ]) testNames
      );
    };

  testRuns = import ./local-test-runs.nix {
    inherit pkgs tests;
  };

  # Script that tests if the structure of the "tests" attribute is compliant to
  # the promised structure in the README.
  verifyTestsAttributeStructure =
    pkgs.runCommandLocal "verify-tests-structure"
      {
        nativeBuildInputs = [ pkgs.file ];
      }
      ''
        set -euo pipefail

        # This structure is expected for every Guest Test. We just test one specific
        # test.
        file --brief --dereference ${tests.lapic-timer.elf32} | grep -q "ELF 32"
        file --brief --dereference ${tests.lapic-timer.elf64} | grep -q "ELF 64"
        file --brief --dereference ${tests.lapic-timer.iso} | grep -q "ISOIMAGE"
        file --brief --dereference ${tests.lapic-timer.efi} | grep -q "EFI application"

        touch $out
      '';

  # Helper that creates a succeeding derivation based on a condition.
  testResultToDrv =
    name: cond:
    if (!cond) then
      abort "Failed test: ${name}"
    else
      pkgs.runCommandLocal "${name}-test-result-to-drv-" { } ''
        mkdir $out
      '';

  # Tests that the kernel command line can be overridden for the given
  # derivation. The verification happens by looking at the effective GRUB
  # config which is provided as passthru attribute.
  cmdlineCanBeOverridden =
    oldDrv:
    let
      newCmdline = "--foobar-lorem-ipsum-best-cmdline-ever";
      readBootloaderCfg = drv: builtins.readFile drv.passthru.bootloaderCfg;
      overriddenDrv = oldDrv.override ({ kernelCmdline = newCmdline; });
      hasCmdline = drv: lib.hasInfix newCmdline (readBootloaderCfg drv);
      oldDrvHasNotNewCmdline = !(hasCmdline oldDrv);
      newDrvHasNewCmdline = hasCmdline overriddenDrv;
    in
    oldDrvHasNotNewCmdline && newDrvHasNewCmdline;
in
{
  inherit
    guest-tests
    testRuns
    tests
    ;

  # Each unit test is a derivation that either fails or succeeds. The output of
  # these derivations is usually empty.
  unitTests = {
    inherit verifyTestsAttributeStructure;
    # Tests that the kernel cmdline of the ISO and EFI images can be overridden
    # so that downstream projects can change the way these tests run easily.
    kernelCmdlineOfBootItemsCanBeOverridden = pkgs.symlinkJoin {
      name = "Kernel Command Line of Boot Items can be overridden";
      paths = [
        (testResultToDrv "cmdline of iso drv can be overridden" (
          cmdlineCanBeOverridden tests.hello-world.iso
        ))
        (testResultToDrv "cmdline of efi drv can be overridden" (
          cmdlineCanBeOverridden tests.hello-world.efi
        ))
      ];
    };
  };
}
