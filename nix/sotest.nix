# Bundles all guest tests as single sotest bundle.

{ pkgs, name, category }:

let
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
      timeout = 300; # 5 minutes; some machines boot very slowly
      type =
        if testMeta.hardwareIndependent
        then "any" else "all";
      bootSource = {
        bios = {
          # iPXE chainloads the Multiboot binary.
          exec = {
            file = toString testAttrs.elf32;
            params = [ testMeta.defaultCmdline ];
          };
        };
        uefi = {
          # iPXE chainloads the EFI binary. Here, the cmdline is embedded in the
          # GRUB standalone image.
          exec = {
            file = toString testAttrs.efi;
            params = [ ];
          };
          load = [ ];
        };
      };
      extraFiles = [ ];
    };
in
pkgs.cyberus.linux-engineering.mkBareSotestBundle {
  sotest = {
    run = {
      inherit name category;
      # If tests receive interrupts or exceptions unexpectedly,
      # they might print the following message and just hang.
      # To prevent long timeouts and retries, we specify this
      # as a panic pattern to immediately cause a defined test
      # failure.
      panicPatterns = [ "NO INTERRUPT HANDLER DEFINED" ];

      localTags = [
        {
          "machine_name" = "amd_m75q_gen2";
          "local_tags" = [ "log:serial" "project:guest-tests" ];
        }

        {
          "machine_name" = "amd_t16_gen2";
          "local_tags" = [ "log:serial" "project:guest-tests" ];
        }
      ];
    };
    bootItems = builtins.listToAttrs (map
      (testName: {
        name = testName;
        value = toBootItemDesc testName;
      })
      testNames);
  };
}
