# Cyberus Guest Tests

This repository contains a set of various guest integration tests to test
virtualization stacks. Technically, a guest test is a tiny kernel that is
booting on the hardware. For example, guest tests report whether a VM host
properly emulates cpuid or the LAPIC. They report successes or failures of their
test cases by printing text to the serial device. The output follows the SoTest
protocol.

## Serial Output

The tests prefer to use a PCI serial device, if one is found. Otherwise, they
fall back to the legacy way of finding the serial port by looking into the BDA.
If this doesn't work either, they fall back to the default `0x3f8` port.

## Supported Boot Flows & Limitations

Each test binary is bootable via/as
[Multiboot1](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html),
[Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html),
[EFI](https://uefi.org/specs/UEFI/2.10/) and
[Xen PVH](https://xenbits.xen.org/docs/unstable/misc/pvh.html). (When consumed
from Nix), they are built as ELF64, ELF32 (same content but different ELF header
for Multiboot1 bootloaders), EFI, and bootable ISO files. The ISO and the EFI
files use a standalone GRUB where the guest-test is embedded. Each guest test is
a statically linked binary at 8M. They are not relocatable in physical or
virtual memory.

## Supported Text Output Channels

- Serial Console (COM port)
- XHCI Console

## Command Line Configuration

The following command line configurations are accepted by the test binaries:
- `--serial [<port>]` Enable serial console, with an optional port <port> in hex without prefix (`0x`).
- `--xhci [<number>]` Enable xHCI debug console, with optional custom serial number.
- `--xhci_power 0|1` Set the USB power cycle method (0=nothing, 1=powercycle).

## Build

### Regular

- build all tests:
```shell
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=./install
ninja
```

### Nix

- All tests with all binary variants: \
  `nix-build -A tests`
  - `./result` is a directory with all corresponding boot items
- Specific test with all binary variants: \
  `nix-build -A tests.<name>` (such as `lapic-timer`)
  - `./result` is a directory with all corresponding boot items
- Specific test with specific binary variants: \
  `nix-build -A tests.<name>.{elf32|elf64|iso|efi}`
  - `./result` is a symlink to the boot item

## Testing

### What we Test

Our guest tests are developed for real hardware and our own virtualization
solutions. Hence, here we only test that they run successfully on real hardware.
We run the following mandatory steps in CI:

- `hello-world` test boots succeeds
  - via Multiboot (in QEMU/kvm)
  - as ISO image (in QEMU/kvm)
  - as EFI image (in QEMU/kvm)
  - via XEN PVH (in Cloud Hypervisor/kvm)
- all tests succeed on real hardware (in SoTest)

Our own virtualization solutions are supposed to schedule their own guest tests
to run as part of their CI infrastructure. We don't test any VMMs here, except
for the `hello-world` test.

There are also optional and manual CI steps, that are allowed to fail. They run
each test in existing stock VMMs. This helps us to easily detect where we are
and also to fix upstream bugs, if we ever want to. These runs are purely
informative.

### Running Guest Tests

#### Regular

- (build as shown above)
- `qemu-system-x86_64 -machine q35,accel=kvm -kernel
  ./src/tests/hello-world.elf32 -m 24M -no-reboot --cpu host -chardev
  stdio,id=hostserial -device pci-serial,chardev=hostserial`

#### Nix

The `release.nix` file exports a rich `testRuns` attribute that runs tests
in virtual machines with various VMMs and different boot strategies. Example
invocations are:

```
# For sotest-protocol-parser:
$ export NIXPKGS_ALLOW_UNFREE=1
$ nix-build nix/release.nix -A testRuns.qemu.kvm.default.hello-world
$ nix-build nix/release.nix -A testRuns.qemu.kvm.multiboot.hello-world
$ nix-build nix/release.nix -A testRuns.chv.kvm.xen-pvh.hello-world
```

The SoTest bundle can be obtained by running

`$ nix-build nix/release.nix -A sotest`.

Note that you can modify the sotest run by using one of the various modifiers
available as passthru attributes. See [cbspkgs for reference](https://gitlab.vpn.cyberus-technology.de/infrastructure/cbspkgs/-/blob/4d38d483167d09fbe95f99886010776e736e250b/lib/README.md).
