let
  pkgs = import ./cbspkgs.nix;

  # All guest tests.
  tests = pkgs.cyberus.guest-tests.tests;
  testNames = builtins.attrNames tests.passthru;

  # Creates a QEMU command.
  createQemuCommand =
    { testname
    , bootMultiboot ? false
    , bootIso ? false
    , bootEfi ? false
    ,
    }:
    let
      qemu = "${pkgs.qemu}/bin/qemu-system-x86_64";
      base = "${qemu} --machine q35,accel=kvm -no-reboot -display none -cpu host -serial stdio -nodefaults";
    in
    if bootMultiboot then {
      setup = "${qemu} --version";
      main = "${base} -kernel ${tests.${testname}.elf32}";
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
    { testname }:
    {
      setup = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --version";
      main = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --memory size=256M --serial tty --console off --kernel ${tests.${testname}.elf64}";
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
          pkgs.cyberus.cidoka.apps.sotest-protocol-parser
        ];
      } ''
      set -euo pipefail

      ${vmmCommand.setup}

      # We rely on that test runs perform a proper shut down under our supported
      # VMMs.
      echo -e "$(ansi bold)Running guest test '${testname}' via ${classifier}$(ansi reset)"
      (timeout --preserve-status --signal KILL 4s ${vmmCommand.main}) |& sotest-protocol-parser --stdin --echo

      touch $out
    '';

  # Creates an attribut set with test runs for every single test run.
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

  # Script that tests if the complex structure of the "tests" attribute is
  # compliant to the promised structure in the README.
  verifyTestsAttributeStructure = pkgs.runCommandLocal "verify-tests-structure"
    {
      nativeBuildInputs = [ pkgs.file ];
    } ''
    set -euo pipefail

    file --brief --dereference ${tests}/lapic-timer.elf32 | grep -q "ELF 32"
    file --brief --dereference ${tests}/lapic-timer.elf64 | grep -q "ELF 64"
    file --brief --dereference ${tests}/lapic-timer.iso | grep -q "ISOIMAGE"
    file --brief --dereference ${tests}/lapic-timer.efi | grep -q "EFI application"

    # This structure is expected for every test. We just test one specific.
    file --brief --dereference ${tests.lapic-timer}/lapic-timer.elf32 | grep -q "ELF 32"
    file --brief --dereference ${tests.lapic-timer}/lapic-timer.elf64 | grep -q "ELF 64"
    file --brief --dereference ${tests.lapic-timer}/lapic-timer.iso | grep -q "ISOIMAGE"
    file --brief --dereference ${tests.lapic-timer}/lapic-timer.efi | grep -q "EFI application"

    # This structure is expected for every test. We just test one specific.
    file --brief --dereference ${tests.lapic-timer.elf32} | grep -q "ELF 32"
    file --brief --dereference ${tests.lapic-timer.elf64} | grep -q "ELF 64"
    file --brief --dereference ${tests.lapic-timer.iso} | grep -q "ISOIMAGE"
    file --brief --dereference ${tests.lapic-timer.efi} | grep -q "EFI application"

    touch $out
  '';
in
rec {
  inherit tests;
  inherit verifyTestsAttributeStructure;

  # Attribute set containing various configurations to run the guest tests in
  # a virtual machine.
  #
  # Structure:
  #   VMM -> Backend -> Boot variant -> Guest Tests
  #
  # Some tests might not succeed (in every configuration) as they are primarily
  # build for real hardware and our own virtualization solutions.
  testRuns = {
    qemu = {
      kvm = rec {
        default = multiboot;
        # Direct kernel boot of Multiboot1 kernel.
        multiboot = createTestRuns
          "qemu-kvm_multiboot"
          (testname: createQemuCommand {
            inherit testname;
            bootMultiboot = true;
          });
        # Legacy boot with ISO.
        iso = createTestRuns "qemu-kvm_iso"
          (testname: createQemuCommand {
            inherit testname; bootIso = true;
          });
        # UEFI environment.
        efi = createTestRuns "qemu-kvm_efi"
          (testname: createQemuCommand {
            inherit testname; bootEfi = true;
          });
      };
    };
    chv = {
      kvm = rec {
        default = xen-pvh;
        xen-pvh = createTestRuns "qemu-kvm_efi"
          (testname: createChvCommand {
            inherit testname;
          });
      };
    };
  };

  # Might be helpful for script-generated invocations of different tests.
  testNames = pkgs.runCommand "print-guest-test-names" { } ''
    echo "${builtins.concatStringsSep "\n" testNames}" > $out
  '';

  # SoTest bundle.
  sotest = pkgs.callPackage ./sotest.nix { };

}
