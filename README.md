# Cyberus Guest Tests

This repository contains a set of various guest integration tests to test
virtualization stacks. Technically, a guest test is a tiny kernel that is
booting on the hardware. For example, guest tests report whether a VM host
properly emulates cpuid or the LAPIC. They report successes or failures of their
test cases by printing text to the serial device. The output follows the SoTest
protocol.

## Guidelines and other Documents

Please have a look at the [`.doc`](/doc/README.md) directory.

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

A guest test has multiple ways to transfer its output to the outer world. The
options listed below mostly can be configured via the command line.

- **Serial Console** (COM port):
  - Active as default or when specified.
  - PCI serial cards as well the builtin COM ports are supported.
- **xHCI Console**:
  - Active when specified.
  - Needs an XHCI controller with the debug capability.
  - Reference: _eXtensible Host Controller Interface for Universal Serial Bus_
               Section 7.6 _Debug Capability_
- **QEMU Debugcon**:
  - Active, when virtualized.
  - This output channel is useful to debug cmdline parsing and other logic that
    is executed before a serial or xHCI console is initialized.

## Command Line Configuration & Console Output

The following command line configurations are accepted by the test binaries:

⚠️ _Only `--flag` and `--option=value` syntax is supported._

- `--serial` or `--serial=<port: number>`:
  Enable serial console. You can explicitly specify the x86 I/O `port`, such as
  `42` or `0x3f8`. It is **recommended to use this as a flag** so that the
  program can automatically discover the port.
- `--xhci` or `--xhci=<identifier: string>`:
  Enable xHCI debug console. If no identifier is provided, a default is used.
- `--xhci-power=0|1`:
  Set the USB power cycle method (`0` = nothing, `1` = powercycle).
- `--disable-testcases=testA,testB,testC`:
  Comma-separated list of test cases that you want to disable. They will be
  skipped. For example:
  To disable `TEST_CASE(foo) {}` you can pass `--disable-testcases=foo`.

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

#### Regular (in QEMU)

- Build as shown above
- Run with recommended settings and output via serial device:
  ```console
  qemu-system-x86_64 \
    -machine q35,accel=kvm \
    -cpu host
    -kernel ./src/tests/hello-world.elf32 \
    -m 24M \
    -no-reboot \
    -serial stdio
  ```
    - Output via a `pci-serial` device: drop `-serial stdio` and add
      `-chardev stdio,id=hostserial -device pci-serial,chardev=hostserial`
    - Output via the debugcon device: add
      `-debugcon file:debugcon_console.txt` (can be combined with a serial device)

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
