/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

# Helpers to access properties of a test from it's properties.toml.

let
  testsDir = ../src/tests;

  getTestProperties = testName:
    let
      path = "${testsDir}/${testName}/properties.toml";
      content = builtins.readFile path;
    in
    builtins.fromTOML content;
in
{
  # We don't want a default value here. Property is too important.
  isCacheable = testName: (getTestProperties testName).cacheable;
  # We don't want a default value here. Property is too important.
  isHardwareIndependent = testName: (getTestProperties testName).hardwareIndependent;
  # Default is fine.
  getSotestMeta = testName: (getTestProperties testName).sotest or { };
}
