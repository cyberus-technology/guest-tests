# Guest Tests

Every subfolder is a small test kernel that tests a specific property
of our system.

Guest test execution can be configured using `sotest.toml` files. Available
settings are:
- `hedron_args`: Hedron cmdline to use instead of default (`serial novga iommu`)
- `supernova_args`: VMM cmdline to use instead of default (`--debugport`)
- `boot_on`: A list of [ `native`, `supernova` ] to specify if tests should run
  natively, on SuperNOVA, or both (default is both)
- `hardware_independent`: Test has no variability on different hardware (default is false)
- `extraTags`: A list of tags to add for this test
- `cacheable`: Test results should be cached when no test input changed

See `nix/tests/toml-helpers.nix` and `nix/tests/guest-mini.nix` for how it's being used.
