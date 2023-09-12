let
  pkgs = import ./cbspkgs.nix;

  allTests = pkgs.cyberus.guest-tests.tests;

  # Runs a single guest test in QEMU with KVM as backend. It boots the guest
  # test as Multiboot1 kernel and as bootable ISO. The test succeeds if the
  # sotest record parser can succesfully parse the output.
  runIntegrationTestKvmQemu = name: pkgs.runCommand "run-${name}-integration-test-kvm-qemu"
    {
      nativeBuildInputs = [ pkgs.qemu pkgs.cyberus.cidoka.apps.sotest-protocol-parser ];
    } ''
    QEMU_BASE="qemu-system-x86_64 --enable-kvm -m 32 -no-reboot -display none -cpu host -serial stdio"

    echo "Running guest test '${name}' as Multiboot1 kernel"
    (timeout --preserve-status -k 4s 3s $QEMU_BASE \
      -kernel ${toString allTests.${name}.elf32} \
      -append "--serial") |& sotest-protocol-parser --stdin --echo

    echo "Running guest test '${name}' as bootable iso"
    (timeout --preserve-status -k 4s 3s $QEMU_BASE \
      -cdrom ${toString allTests.${name}.iso}) |& sotest-protocol-parser --stdin --echo

    touch $out
  '';

  # Runs a single guest test in QEMU with KVM as backend. It boots the guest
  # test as Xen PVH direct boot kernel. The test succeeds if the sotest record
  # parser can succesfully parse the output.
  runIntegrationTestKvmChv = name: pkgs.runCommand "run-${name}-integration-test-kvm-cloud-hypervisor"
    {
      nativeBuildInputs = [ pkgs.cloud-hypervisor ];
    } ''
    CHV_BASE="cloud-hypervisor --memory size=32M --serial tty --console off"

    echo "Running guest test '${name}' via Xen PVH direct boot"
    (timeout --preserve-status -k 4s 3s $CHV_BASE \
      --kernel ${toString allTests.${name}.elf64} \
      --cmdline "--serial") |& sotest-protocol-parser --stdin --echo

    touch $out
  '';
in
{
  inherit (pkgs.cyberus.guest-tests) tests;

  runTests = {
    kvm = {
      qemu = runIntegrationTestKvmQemu "hello-world";
      chv = runIntegrationTestKvmQemu "hello-world";
    };
  };

}
