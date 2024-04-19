{ pkgs ? import ./cbspkgs.nix }:

let
  lib = pkgs.lib;
  # We don't use callPackage here, as we do not want `override` and
  # `overrideAttrs`. Otherwise, `builtins.attrNames` doesn't only return
  # all test names.
  tests = import ./build.nix {
    inherit pkgs;
  };
  sotest = pkgs.cyberus.guest-tests.sotests.default;
  testNames = builtins.attrNames tests;

  # Creates a QEMU command.
  createQemuCommand =
    { testname
    , bootMultiboot ? false
    , bootIso ? false
    , bootEfi ? false
    , cmdline
    }:
    let
      qemu = "${pkgs.qemu}/bin/qemu-system-x86_64";
      baseCmd = "${qemu} --machine q35,accel=kvm -no-reboot -display none -cpu host -serial stdio -nodefaults";
      bootEfiFile = bootEfi && !bootIso;
    in
    assert
    (bootMultiboot -> !(bootIso || bootEfi));
    {
      # Setup script lines.
      setup = builtins.concatStringsSep "\n"
        (
          [ "${qemu} --version" ] ++
          lib.optional bootEfiFile ''
            mkdir -p uefi/EFI/BOOT
            install -m 0644 ${tests.${testname}.efi} uefi/EFI/BOOT/BOOTX64.EFI
          ''
        );
      # VMM command (oneline).
      main = "${baseCmd}"
        + (lib.optionalString bootMultiboot " -kernel ${tests.${testname}.elf32} -append ${cmdline}")
        + (lib.optionalString bootIso " -cdrom ${tests.${testname}.iso}")
        + (lib.optionalString bootEfi " -bios ${pkgs.OVMF.fd}/FV/OVMF.fd")
        + (lib.optionalString bootEfiFile " -drive format=raw,file=fat:rw:./uefi")
      ;
    }
  ;

  # Creates a Cloud Hypervisor command.
  createChvCommand =
    { testname
    , cmdline
    }:
    {
      setup = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --version";
      main = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --memory size=256M --serial tty --console off --kernel ${tests.${testname}.elf64} --cmdline '${cmdline}'";
    };


  # Creates a single test run of a test.
  createTestRun =
    # Test name.
    testname:
    # Classifier of the test for better log output.
    classifier:
    # VMM invocation.
    vmmCommand:
    pkgs.runCommand "${testname}-guest-test-${classifier}"
      {
        nativeBuildInputs = [
          pkgs.ansi
          pkgs.cyberus.sotest.apps.sotest-protocol-parser
        ];
      } ''
      set -euo pipefail

      ${vmmCommand.setup}

      # sotest-protocol-parser has no timeout handling and we also have some
      # long-running tests. For now, we configure the timeouts here. If this becomes
      # unwieldy, we need to move it to the test properties.
      case '${testname}' in
        timing)
          timeout=10m
          ;;
        *)
          timeout=4s
          ;;
      esac

      # We rely on that test runs perform a proper shut down under our supported
      # VMMs.
      echo -e "$(ansi bold)Running guest test '${testname}' via ${classifier} with $timeout timeout$(ansi reset)"
      (timeout --preserve-status --signal KILL "$timeout" ${vmmCommand.main}) |& sotest-protocol-parser --stdin --echo

      touch $out
    '';

  # Creates an attribute set with test runs for every single test.
  createTestRuns =
    # Classifier of the test for better log output.
    classifier:
    # Creates the VMM invocation for the right test.
    vmmCommandForTest:
    builtins.foldl'
      (acc: testname: {
        "${testname}" = createTestRun testname classifier (vmmCommandForTest testname);
      } // acc)
      { }
      testNames;

  # Script that tests if the structure of the "tests" attribute is compliant to
  # the promised structure in the README.
  verifyTestsAttributeStructure = pkgs.runCommandLocal "verify-tests-structure"
    {
      nativeBuildInputs = [ pkgs.file ];
    } ''
    set -euo pipefail

    # This structure is expected for every test. We just test one specific.
    file --brief --dereference ${tests.lapic-timer.elf32} | grep -q "ELF 32"
    file --brief --dereference ${tests.lapic-timer.elf64} | grep -q "ELF 64"
    file --brief --dereference ${tests.lapic-timer.iso} | grep -q "ISOIMAGE"
    file --brief --dereference ${tests.lapic-timer.efi} | grep -q "EFI application"

    touch $out
  '';

  # Helper that creates a succeeding derivation based on a condition.
  testResultToDrv = name: cond:
    if (!cond) then abort "Failed test: ${name}" else
    pkgs.runCommandLocal "${name}-test-result-to-drv-" { } ''
      mkdir $out
    '';

  # Tests that the kernel command line can be overridden for the given
  # derivation.
  cmdlineCanBeOverridden = oldDrv:
    let
      newCmdline = "--foobar-lorem-ipsum-best-cmdline-ever";
      readBootloaderCfg = drv: builtins.readFile drv.passthru.bootloaderCfg;
      overriddenDrv = oldDrv.override ({ kernelCmdline = newCmdline; });
      hasCmdline = drv: lib.hasInfix newCmdline (readBootloaderCfg drv);
      oldDrvHasNotNewCmdline = !(hasCmdline oldDrv);
      newDrvHasNewCmdline = hasCmdline overriddenDrv;
    in
    oldDrvHasNotNewCmdline && newDrvHasNewCmdline;

  pre-commit-check = import ./pre-commit-check.nix { inherit pkgs; };
in
{
  inherit pre-commit-check;
  inherit tests sotest;

  # Attribute set containing various configurations to run the guest tests in
  # a virtual machine.
  #
  # Structure:
  #   VMM -> Backend -> Boot variant -> Guest Tests
  #
  # Some tests might not succeed (in every configuration) as they are primarily
  # build for real hardware and our own virtualization solutions.
  testRuns =
    let
      getDefaultCmdline = testname: tests."${testname}".elf32.meta.testProperties.defaultCmdline;
    in
    {
      qemu = {
        kvm = rec {
          default = multiboot;
          # Direct kernel boot of Multiboot1 kernel.
          multiboot = createTestRuns
            "qemu-kvm_multiboot"
            (testname: createQemuCommand {
              inherit testname;
              bootMultiboot = true;
              cmdline = getDefaultCmdline testname;
            });
          # Legacy boot with ISO.
          iso = createTestRuns "qemu-kvm_iso"
            (testname: createQemuCommand {
              inherit testname;
              bootIso = true;
              cmdline = getDefaultCmdline testname;
            });
          # Booting the hybrid ISO in an UEFI environment.
          # In NixOS 23.11, OVMF has the Compatibility Support Module (CSM), to
          # also boot ISOs without any EFI file in it. However, starting with
          # NixOS 24.05, this is a real EFI boot from an ISO file.
          # See: https://github.com/NixOS/nixpkgs/commit/4631f2e1ed2b66d099948665209409f2e8fc37ec
          iso-uefi = createTestRuns "qemu-kvm_iso-uefi"
            (testname: createQemuCommand {
              inherit testname;
              bootIso = true;
              bootEfi = true;
              cmdline = getDefaultCmdline testname;
            });
          # UEFI environment.
          efi = createTestRuns "qemu-kvm_efi"
            (testname: createQemuCommand {
              inherit testname;
              bootEfi = true;
              cmdline = getDefaultCmdline testname;
            });
        };
      };
      chv = {
        kvm = rec {
          default = xen-pvh;
          xen-pvh = createTestRuns "chv-kvm_xen-pvh"
            (testname: createChvCommand {
              inherit testname;
              cmdline = getDefaultCmdline testname;
            });
        };
      };
    };

  # Each unit test is a derivation that either fails or succeeds. The output of
  # these derivations is usually empty.
  unitTests = {
    inherit verifyTestsAttributeStructure;
    # Tests that the kernel cmdline of the ISO and EFI images can be overridden
    # so that downstream projects can change the way these tests run easily.
    kernelCmdlineOfBootItemsCanBeOverridden = pkgs.symlinkJoin {
      name = "Kernel Command Line of Boot Items can be Overridden";
      paths = [
        (testResultToDrv "cmdline of iso drv can be overridden" (cmdlineCanBeOverridden tests.hello-world.iso))
        (testResultToDrv "cmdline of efi drv can be overridden" (cmdlineCanBeOverridden tests.hello-world.efi))
      ];
    };
  };
}
