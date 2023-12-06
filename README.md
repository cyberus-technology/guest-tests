# Cyberus Guest Tests

This repository contains a set of various guest integration tests to test
virtualization stacks. Technically, a guest test is a tiny kernel that is
booting on the hardware. For example, guest tests report whether a VM host
properly emulates cpuid or the LAPIC. They report successes or failures of their
test cases by printing text to the serial device. The output follows the SoTest
protocol.

## Guidelines and other Documents

Please have a look at the [`.doc`](/doc/README.md) directory.

## Serial Output

The tests prefer to use a PCI serial device, if one is found. Otherwise, they
fall back to the legacy way of finding the serial port by looking into the BDA.
If this doesn't work either, they fall back to the default `0x3f8` port.

## Binary Format, Supported Boot Flows, Limitations, and Restrictions

Each guest test is a statically linked ELF binary at 8M. They are not
relocatable in physical or virtual memory.

Each guest test is bootable via

- [Multiboot1](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html),
- [Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html),
  and
- [Xen PVH](https://xenbits.xen.org/docs/unstable/misc/pvh.html).

The CMake-build builds the tests as `.elf32` and `.elf64` (same content but
different ELF header for Multiboot1 bootloaders). Additionally, the Nix build
allows ready-to-boot `.efi` and `.iso` variants (see below for more
information) for boot in legacy as well as UEFI environments.
<!--
TODO add that .iso can also be used in EFI boot, once that is implemented
-->

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

To build a test, run:

```shell
nix-build -A tests.<name>.{elf32|elf64|iso|efi}
```

`./result` is a symlink to the boot item.

The `iso` and `efi` attributes are high-level variants including a bootloader
chainloading the test via Multiboot with a corresponding `cmdline`. It is
possible to override the cmdline of those attributes using `override`. For
example:

```shell
nix-build -E '(import ./nix/release.nix).tests.hello-world.{iso|efi}.override({kernelCmdline = "foobar";})'
```

The effective cmdline can also be verified by looking at the final GRUB config:

```shell
cat $(nix-build -E '((import ./nix/release.nix).tests.hello-world.{iso|efi}.override({kernelCmdline = "foobar";})).grubCfg')
```

## Test Metadata (Nix)

When consumed with Nix, each test carries metadata that you can use to create
test runs:

```nix
{
  meta = {
    testProperties = {
      # Test results should be cached when no test input changed.
      #
      # This is usually false if the test contains timing-specific behavior
      # or benchmarks.
      cacheable = <bool>;
      # Test has no expected variability on different hardware.
      #
      # This is usually false if the test contains timing-specific behavior.
      hardwareIndependent = <bool>;
      # SoTest-specific metadata.
      sotest = {
        # Additional machine tags required for bare-metal execution.
        #
        # This might be interesting for VMMs too, as this can tell about
        # desired hardware or firmware functionality.
        extraTags = <string>[];
      };
    };
  };
}
```
<!--
It is important that we don't export these settings as meta.sotest = {} as
otherwise, the cbspkgs pipeline might think that these are sotest runs, which
they are not.
-->

It is accessible via `nix-build -A tests.<name>.<variant>`. Please find more
info in
[Reuse Guest Tests (with Nix)](/doc/nix-reuse-guest-tests/nix-reuse-guest-tests.md).

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
