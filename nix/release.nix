{ pkgs ? import ./cbspkgs.nix }:

let
  # All guest tests.
  tests = pkgs.cyberus.guest-tests.tests;
  sotest = pkgs.cyberus.guest-tests.sotests.default;
  testNames = builtins.attrNames tests;

  # trace = pkgs.cyberus.cbspkgs.lib.tracing.prettyVal;

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
      base = "${qemu} --machine q35,accel=kvm -no-reboot -display none -cpu host -serial stdio -nodefaults";
    in
    if bootMultiboot then {
      setup = "${qemu} --version";
      main = "${base} -kernel ${tests.${testname}.elf32} -append '${cmdline}'";
    }
    else if bootIso then {
      setup = "${qemu} --version";
      main = "${base} -cdrom ${tests.${testname}.iso}";
    }
    else if bootEfi then {
      setup = ''
        mkdir -p uefi/EFI/BOOT
        install -m 0644 ${tests.${testname}.efi} uefi/EFI/BOOT/BOOTX64.EFI

        ${qemu} --version
      '';
      main = "${base} -bios ${pkgs.OVMF.fd}/FV/OVMF.fd -drive format=raw,file=fat:rw:./uefi";
    }
    else abort "You must specify one boot variant!"
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
  # derivation. The verification happens by looking at the effective GRUB
  # config which is provided as passthru attribute.
  cmdlineCanBeOverridden = oldDrv:
    let
      newCmdline = "--foobar-lorem-ipsum-best-cmdline-ever";
      readGrubCfg = drv: builtins.readFile drv.passthru.grubCfg;
      overriddenDrv = oldDrv.override ({ kernelCmdline = newCmdline; });
      hasCmdline = drv: pkgs.lib.hasInfix newCmdline (readGrubCfg drv);
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
