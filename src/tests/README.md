# Guest Tests

Every subfolder is a small test kernel that tests a specific property
of our system.

Guest test execution can be configured using `sotest.toml` files. Available
settings are:
- `cacheable`: Test results should be cached when no test input changed
- `extraTags`: A list of required SoTest machine tags to add for this test
- `hardware_independent`: Test has no variability on different hardware (default is false)
