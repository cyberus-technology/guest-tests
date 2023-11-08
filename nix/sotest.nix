# Bundles all guest tests as single sotest bundle.

{ pkgs }:

let
  testDir = ./../src/tests;

  tomlHelpers = pkgs.callPackage ./toml-helpers.nix { inherit testDir; };

  # Creates a boot item description for SoTest. Each guest test can be booted
  # on bios or uefi machines.
  toBootItemDesc = testName: {
    name = "${testName}";
    # Will be handled as a "bifi" boot item (bios + uefi) as no further tags
    # are specified.
    tags = [ "log:serial" "project:guest-tests" ]
      ++ (tomlHelpers.specificSettings testName).extraTags or [ ]
    ;
    cacheable = tomlHelpers.checkIsCacheable testName;
    boot_item_timeout = 300; # 5 minutes; some machines boot very slowly
    boot_item_type =
      if (tomlHelpers.checkIsHardwareIndependent testName)
      then "any" else "all";
    boot_source = {
      bios = {
        # iPXE chainloads the Multiboot binary.
        exec = pkgs.cyberus.guest-tests.tests.${testName}.elf32 + " --serial";
        load = [ ];
      };
      uefi = {
        # iPXE chainloads the EFI binary.
        exec = pkgs.cyberus.guest-tests.tests.${testName}.efi + " --serial";
        load = [ ];
      };
    };
  };

  allTestNames = builtins.attrNames pkgs.cyberus.guest-tests.tests.passthru;
in
pkgs.cyberus.cbspkgs.lib.sotest.mkProjectBundle {
  boot_items = map (testname: toBootItemDesc testname) allTestNames;
}
