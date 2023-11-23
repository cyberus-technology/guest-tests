# Bundles all guest tests as single sotest bundle.

{ pkgs }:

let
  libsotest = pkgs.cyberus.cbspkgs.lib.sotest;

  testList = builtins.attrValues pkgs.cyberus.guest-tests.tests.passthru;

  defaultSotestTags = [ "log:serial" "project:guest-tests" ];

  # Creates a boot item description for SoTest. Each guest test can be booted
  # on bios or uefi machines.
  toBootItemDesc = test: {
    name = "${test.meta.sotest.name}";
    tags = defaultSotestTags ++ test.meta.sotest.extraTags;
    cacheable = test.meta.sotest.cacheable;
    boot_item_timeout = 300; # 5 minutes; some machines boot very slowly
    boot_item_type =
      if test.meta.sotest.isHardwareIndependent
      then "any" else "all";
    boot_source = {
      bios = {
        # iPXE chainloads the Multiboot binary.
        exec = test.elf32 + " --serial";
        load = [ ];
      };
      uefi = {
        # iPXE chainloads the EFI binary.
        exec = test.efi + " --serial";
        load = [ ];
      };
    };
  };
in
libsotest.mkProjectBundle {
  boot_items = map
    (testName: toBootItemDesc testName)
    testList;
}
