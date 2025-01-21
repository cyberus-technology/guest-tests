{ pkgs, tests }:

let

  testNames = builtins.attrNames tests;

  # Creates a QEMU command.
  createQemuCommand =
    {
      testname,
      bootMultiboot ? false,
      bootIso ? false,
      bootEfi ? false,
      cmdline,
    }:
    let
      qemu = "${pkgs.qemu}/bin/qemu-system-x86_64";
      base = "${qemu} --machine q35,accel=kvm -no-reboot -display none -cpu host -serial stdio -nodefaults";
    in
    if bootMultiboot then
      {
        setup = "${qemu} --version";
        main = "${base} -kernel ${tests.${testname}.elf32} -append '${cmdline}'";
      }
    else if bootIso then
      {
        setup = "${qemu} --version";
        main = "${base} -cdrom ${tests.${testname}.iso}";
      }
    else if bootEfi then
      {
        setup = ''
          mkdir -p uefi/EFI/BOOT
          install -m 0644 ${tests.${testname}.efi} uefi/EFI/BOOT/BOOTX64.EFI

          ${qemu} --version
        '';
        main = "${base} -bios ${pkgs.OVMF.fd}/FV/OVMF.fd -drive format=raw,file=fat:rw:./uefi";
      }
    else
      abort "You must specify one boot variant!";

  # Creates a Cloud Hypervisor command.
  createChvCommand =
    {
      testname,
      cmdline,
    }:
    {
      setup = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --version";
      main = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --memory='size=256M' --serial=tty --console=off --kernel=${tests.${testname}.elf64} --cmdline='${cmdline}'";
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
        ];
      }
      ''
        set -euo pipefail

        ${vmmCommand.setup}

        # We have some long-running tests. For now, we configure the timeouts
        # here. If this becomes unwieldy, we need to move it to the test
        # properties.
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
        echo "Executing \"${vmmCommand.main}\""
        timeout --preserve-status --signal KILL "$timeout" ${vmmCommand.main}

        touch $out
      '';

  # Creates an attribute set with test runs for every single test.
  createTestRuns =
    # Classifier of the test for better log output.
    classifier:
    # Creates the VMM invocation for the right test.
    vmmCommandForTest:
    builtins.foldl' (
      acc: testname:
      {
        "${testname}" = createTestRun testname classifier (vmmCommandForTest testname);
      }
      // acc
    ) { } testNames;

  getDefaultCmdline = testname: tests."${testname}".elf32.meta.testProperties.defaultCmdline;
in
{
  qemu = {
    kvm = rec {
      default = multiboot;
      # Direct kernel boot of Multiboot1 kernel.
      multiboot = createTestRuns "qemu-kvm_multiboot" (
        testname:
        createQemuCommand {
          inherit testname;
          bootMultiboot = true;
          cmdline = getDefaultCmdline testname;
        }
      );
      # Legacy boot with ISO.
      iso = createTestRuns "qemu-kvm_iso" (
        testname:
        createQemuCommand {
          inherit testname;
          bootIso = true;
          cmdline = getDefaultCmdline testname;
        }
      );
      # Booting the hybrid ISO in an UEFI environment.
      # In NixOS 23.11, OVMF has the Compatibility Support Module (CSM), to
      # also boot ISOs without any EFI file in it. However, starting with
      # NixOS 24.05, this is a real EFI boot from an ISO file.
      # See: https://github.com/NixOS/nixpkgs/commit/4631f2e1ed2b66d099948665209409f2e8fc37ec
      iso-uefi = createTestRuns "qemu-kvm_iso-uefi" (
        testname:
        createQemuCommand {
          inherit testname;
          bootIso = true;
          bootEfi = true;
          cmdline = getDefaultCmdline testname;
        }
      );
      # UEFI environment.
      efi = createTestRuns "qemu-kvm_efi" (
        testname:
        createQemuCommand {
          inherit testname;
          bootEfi = true;
          cmdline = getDefaultCmdline testname;
        }
      );
    };
  };
  chv = {
    kvm = rec {
      default = xen-pvh;
      xen-pvh = createTestRuns "chv-kvm_xen-pvh" (
        testname:
        createChvCommand {
          inherit testname;
          cmdline = getDefaultCmdline testname;
        }
      );
    };
  };
}
