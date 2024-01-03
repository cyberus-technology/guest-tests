# Bundles all guest tests as single sotest bundle.

{ pkgs }:

let
  libsotest = pkgs.cyberus.cbspkgs.lib.sotest;

  tests = pkgs.cyberus.guest-tests.tests;
  testNames = builtins.attrNames tests;

  defaultSotestTags = [ "log:serial" "project:guest-tests" ];

  # Creates a boot item description for SoTest. Each guest test can be booted
  # on bios or uefi machines.
  toBootItemDesc = testName:
    let
      testAttrs = tests."${testName}";
      # The metadata is the same for each variant.
      testMeta = testAttrs.elf32.meta.testProperties;
    in
    {
      name = testName;
      tags = defaultSotestTags ++ (testMeta.sotest.extraTags or [ ]);
      cacheable = testMeta.cacheable;
      boot_item_timeout = 300; # 5 minutes; some machines boot very slowly
      boot_item_type =
        if testMeta.hardwareIndependent
        then "any" else "all";
      boot_source = {
        bios = {
          # iPXE chainloads the Multiboot binary.
          exec = testAttrs.elf32 + " " + testMeta.defaultCmdline;
          load = [ ];
        };
        uefi = {
          # iPXE chainloads the EFI binary. Here, the cmdline is embedded in the
          # GRUB standalone image.
          exec = testAttrs.efi;
          load = [ ];
        };
      };
    };
in
libsotest.mkProjectBundle {
  boot_items = map
    (testName: toBootItemDesc testName)
    testNames;
}
