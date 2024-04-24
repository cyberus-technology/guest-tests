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


## Hardware Requirements

This section discusses the requirements of the guests tests to run on real
hardware. If you run them virtualized, your virtual hardware platform must
fulfill the same requirements.

All guest tests are created for x86_64 machines targeting
[Intel westmere](https://en.wikipedia.org/wiki/Westmere_(microarchitecture)) and
above. <!-- Keep in sync with compiler flags! -->
Typical platform-related functionality is expected to be available as well, such
as the LAPIC, the IOAPIC, but also legacy devices such as the serial device and
the PIT.

However, for some functionality that is not "basic enough" and whose
availability is easily detectable at runtime, the guest tests check for their
presence and skip relevant test cases if the conditions are not met. Examples
for that are the availability of:

- Platform (devices, MSRs):
  - HPET
  - x2APIC mode
  - TSC deadline mode of LAPIC timer
- ISA extensions:
  - `movbe` support
  - AVX support


### Test-specific machine expectations

Some guest tests have specific expectations that are only relevant to that
single guest test.

- `cpuid` test:
  - Checks that the extended cpuid brand string prefix is one of the definitions
    in [`cpuid/main.cpp`](/src/tests/cpuid/main.cpp).
  - To extend that list or make the overall mechanism more flexible, please
    submit an issue or an MR.



## Consume the Guest Tests

Currently, you only can build the Guest Tests from source. Please refer to our
[/doc/DEVELOPER.md](/doc/DEVELOPER.md) for more details.


## Developer Documentation

Please look at [/doc/DEVELOPER.md](/doc/DEVELOPER.md). This document mainly
focuses on Cyberus employees with access to our internal network and
infrastructure.


## License

This repository is licensed under GPLv2. Files under `src/contrib` are licensed
under their corresponding license in the file header.

See [LICENSE file](./LICENSE).
