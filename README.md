# Cyberus Guest Tests

This repository contains a set of various guest integration tests to test
virtualization stacks. For example, the report whether a VM host properly
emulates cpuid or the LAPIC. Each test binary is bootable via
[Multiboot1](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html),
[Multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html),
and [Xen PVH](https://xenbits.xen.org/docs/unstable/misc/pvh.html). They are
build as ELF, ELF32 (same content but different ELF header for Multiboot1),
and bootable ISO.

The tests report success or failures by printing text. The output follows the
[SoTest protocol](https://docs.sotest.io/user/protocol/).

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
#### All Tests
```shell
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=./install && ninja -j 1
```

### Nix
#### All Tests
- `nix-build -A tests.all`
#### Individual Tests:
- `nix-build -A tests.<name>` (such as `cpuid`)
