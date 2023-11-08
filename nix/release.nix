let
  pkgs = import ./cbspkgs.nix;

  allTests = pkgs.cyberus.guest-tests.tests;
  allTestNames = builtins.attrNames allTests.passthru;

  # Creates a QEMU command.
  createQemuCommand =
    { testname
    , bootMultiboot ? false
    , bootIso ? false
    , bootEfi ? false
    ,
    }:
    let
      base = "${pkgs.qemu}/bin/qemu-system-x86_64 --machine q35,accel=kvm -no-reboot -display none -cpu host -serial stdio -nodefaults";
    in
    if bootMultiboot then {
      setup = "";
      main = "${base} -kernel ${allTests.${testname}.elf32}";
    }
    else if bootIso then {
      setup = "";
      main = "${base} -cdrom ${allTests.${testname}.iso}";
    }
    else if bootEfi then {
      setup = ''
        mkdir -p uefi/EFI/BOOT
        install -m 0644 ${allTests.${testname}.efi} uefi/EFI/BOOT/BOOTX64.EFI
      '';
      main = "${base} -bios ${pkgs.OVMF.fd}/FV/OVMF.fd -drive format=raw,file=fat:rw:./uefi";
    }
    else abort "You must specify one boot variant!"
  ;

  # Creates a Cloud Hypervisor command.
  createChvCommand =
    { testname }:
    {
      setup = "";
      main = "${pkgs.cloud-hypervisor}/bin/cloud-hypervisor --memory size=256M --serial tty --console off --kernel ${allTests.${testname}.elf64}";
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

      echo -e "$(ansi bold)Running guest test '${testname}' via ${classifier}$(ansi reset)"
      (timeout --preserve-status -k 4s 3s ${vmmCommand.main}) |& sotest-protocol-parser --stdin --echo

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
      allTestNames;
in
rec {
  inherit (pkgs.cyberus.guest-tests) tests;

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
    echo "${builtins.concatStringsSep "\n" allTestNames}" > $out
  '';

}
