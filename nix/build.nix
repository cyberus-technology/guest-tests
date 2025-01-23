/*
  Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

# Builds the CMake project and makes each test accessible in multiple binary
# variants (ELF32, ELF64, ISO, ELF) using the following attribute structure:
# `tests.<testname>.<variant>`.

{ pkgs, pkgs-23-11 }:

let
  lib = pkgs.lib;

  testNames = [
    "cpuid"
    "emulator-syscall"
    "exceptions"
    "fpu"
    "hello-world"
    "lapic-modes"
    "lapic-priority"
    "lapic-timer"
    "msr"
    "pagefaults"
    "pit-timer"
    "sgx"
    "sgx-launch-control"
    "timing"
    "tsc"
    "tinivisor"
    "vmx"
  ];

  cmakeProj = pkgs.callPackage ./build-cmake-project.nix { inherit pkgs-23-11; };

  # Extracts a single binary variant of a test.
  # The result is a direct symlink to the boot item.
  extractBinaryFromCmakeBuild =
    testName: suffix:
    pkgs.runCommandLocal "cmake-build-variant-${testName}-${suffix}"
      {
        passthru = {
          inherit cmakeProj;
        };
      }
      ''
        ln -s ${cmakeProj}/${testName}.${suffix} $out
      '';

  # Returns the default command line for each test.
  getDefaultCmdline =
    testName:
    let
      dict = {
        hello-world = "--disable-testcases=test_case_is_skipped_by_cmdline";
      };
    in
    dict.${testName} or "";

  # Returns the properties of a test as attribute set.
  getTestProperties =
    let
      helper = import ./properties-toml-helper.nix;
    in
    testName:
    # Properties from properties.toml.
    {
      cacheable = helper.isCacheable testName;
      hardwareIndependent = helper.isHardwareIndependent testName;
      sotest = helper.getSotestMeta testName;
    }
    # Merge with additional properties from other sources.
    // {
      defaultCmdline = getDefaultCmdline testName;
    };

  createIsoImage = lib.makeOverridable (pkgs.callPackage ./create-iso-image.nix { });

  # Creates an attribute set that holds all binary variants of a test.
  toVariantsAttrs =
    testName:
    let
      base =
        let
          elf32 = extractBinaryFromCmakeBuild testName "elf32";
          elf64 = extractBinaryFromCmakeBuild testName "elf64";
        in
        {
          inherit elf32 elf64;
          iso = createIsoImage {
            kernel = elf64;
            kernelCmdline = getDefaultCmdline testName;
          };
          efi = pkgs.callPackage ./create-efi-image.nix {
            name = "guest-test-${testName}-efi";
            kernel = elf64;
            kernelCmdline = getDefaultCmdline testName;
          };
        };
    in
    # Add meta data to each variant.
    builtins.mapAttrs (
      _name: variant:
      let
        testProperties = getTestProperties testName;
        meta = (variant.meta or { }) // {
          inherit testProperties;
        };
      in
      variant // { inherit meta; }
    ) base;

  # List of (name, value) pairs.
  testList = map (name: {
    inherit name;
    value = toVariantsAttrs name;
  }) testNames;
in
builtins.listToAttrs testList
