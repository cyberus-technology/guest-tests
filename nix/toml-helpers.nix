{ testDir }:
rec {
  /* Return a test's sotest settings.
     Parses custom settings from 'sotest.toml' in the test directory and returns them as an attribute set.
     The attribute set is empty if no 'sotest.toml' file exists.
  */
  specificSettings = testName:
    let
      cfgFileName = testDir + "/${testName}/properties.toml";
      cfgContents = if builtins.pathExists cfgFileName then builtins.readFile cfgFileName else "";
    in
    builtins.fromTOML cfgContents;

  /* Return whether a test is hardware independent.
  */
  checkIsHardwareIndependent = testName: (specificSettings testName).hardware_independent or false;

  /* Return whether a test is cacheable.
  */
  checkIsCacheable = testName: (specificSettings testName).cacheable or false;
}
