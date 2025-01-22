# Developer Documentation

_This documentation focuses on our internal engineering pipeline and not on
any public read-only mirror._

## General Remarks

Please note that our Nix tooling is split into `nix/public` and
`nix/cyberus/`. The latter only works if you are onboarded to our internal
infrastructure. However, the public functionality offers build support but
without additional convenience.

## Code Style

We use `pre-commit` via [`pre-commit-hooks.nix`](https://github.com/cachix/pre-commit-hooks.nix)
to run various style checks in the CI. You can run all checks by running
`$ nix-shell --run "pre-commit run --all-files"`. If you enter the Nix shell,
the hooks are automatically installed and a `.pre-commit-config.yaml` is
generated.


## Regular Build (Guest Tests + Unit Tests)

Build all Guest Tests:

```shell
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=./install
ninja
```

Build all Guest Tests and run unit tests:

```shell
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=./install -DBUILD_TESTING=On
ninja
ninja test
```

You can also use `make` instead of `ninja`.


## Nix Build (Guest Tests + Unit Tests)

We provide a Nix flake entry (`nix build`) but also a traditional `nix-build`
entry via `default.nix`.

To simply get all guest tests, run:

```shell
nix build .#guest-tests
```

This also builds and runs the unit tests of the CMake project. To get a specific
test, you have to use the traditional way, as these are not exported by the
flake:

```shell
nix-build -A tests.<name>.{elf32|elf64|iso|efi}
```

This also builds and runs the unit tests of the CMake project. `./result` is a
symlink to the corresponding boot item.

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

When consumed with Nix, each test variant carries metadata that you can use to
create test runs. The metadata is the same for all variants of a test.
It is accessible via `nix-build -A tests.<name>.<variant>`. Please find more
info in [Reuse Guest Tests (with Nix)](/doc/nix-reuse-guest-tests/nix-reuse-guest-tests.md).

<!--
It is important that we don't export these settings as meta.sotest = {} as
otherwise, the cbspkgs pipeline might think that these are sotest runs, which
they are not.
-->

```nix
{
  meta = {
    # These properties give hints about the behaviour of a test when it runs on
    # real hardware. If you run a test in a virtual machine, i.e., on virtual
    # hardware, you might chose different values depending on your project.
    #
    # We use them in our (CI) test infrastructure to schedule test runs on real
    # hardware.
    testProperties = {
      # Test results can be cached safely when no test inputs have changed.
      #
      # This is usually false if the test contains timing-specific behavior
      # or benchmarks or if we suspect a hidden flakiness, which may be
      # revealed over time.
      cacheable = <bool>;
      # The default cmdline of that test.
      defaultCmdline = <string>;
      # The test behaves equally on different machines/hardware.
      #
      # This is false, if the same test produces different results on different
      # hardware, but these differences still result in a successful test
      # execution, and it is important that different runs on different machines
      # all report success. Usually, this is the case if a test uses cpuid or
      # timers, which naturally differs between various platforms of the same
      # overall platform family.
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


## Unit Tests and Integration Tests

### libtoyos Unit Tests

We have unit tests for libtoyos, the base of our Guest Tests. Please head to the
build section above to see how to build and run them.


### Guest Tests

#### What we Test

Our guest tests are developed for real hardware and our own virtualization
solutions. But, here we only test that they run successfully on real hardware.
We run the following mandatory steps in CI:

- `hello-world` test boots successfully:
    - via Multiboot (in QEMU/kvm)
    - as ISO image (in QEMU/kvm)
    - as EFI image (in QEMU/kvm)
    - via XEN PVH (in Cloud Hypervisor/kvm)
- all tests succeed on real hardware (in SoTest)

Our own virtualization solutions are supposed to schedule their own guest test
runs as part of their CI infrastructure. We don't test any VMMs here as
mandatory steps of the pipeline, except for the `hello-world` test.

There are also optional and manual CI steps, that are allowed to fail. They run
each test in existing stock VMMs. This helps us to easily detect where we are
and also to fix upstream bugs, if we ever want to. These runs are purely
informative.


#### Running Guest Tests

##### Regular (in QEMU)

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


##### Nix

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
available as passthru attributes ([reference](http://cyberus.doc.vpn.cyberus-technology.de/engineering/linux-engineering/nixos-modules/sotest.html#sotestpassthruattributes)).
