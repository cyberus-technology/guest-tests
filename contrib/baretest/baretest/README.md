Baretest Test Library
=====================

A C++ testing library with minimal dependencies.

Dependencies
------------

 * `std::vector`
 * C++11 `enum class`
 * `size_t` needs to be defined
 * `setjmp()` and `longjmp()` need to be implemented

Building
--------

Baretest is designed to provide configurable output. This way the library easily
integrates with any existing test automation framework. The easiest way to do
that is to provide a small glue library that defines the following functions:

``` C++
namespace baretest
{
    void success(const char *name);
    void failure(const char *name);
    void skip();
    void hello(size_t test_count);
    void goodbye();
} // namespace baretest
```

Baretest uses the [tup](http://gittup.org/tup) build system. In order to build
the project all you have to do is run `tup` in the baretest root folder.

Writing Tests
-------------

Baretest strives to enable writing tests with as little boilerplate as possible.
As simple test could look like this:

``` C++
#include <baretest/baretest.hpp>
#include <cstdint>

TEST_CASE(expect)
{
    baretest::expectation<uint64_t> expected(7);
    uint64_t                        actual = 5 + 2;
    BARETEST_VERIFY(expected == actual);
    BARETEST_VERIFY(expected != actual + 1);
    BARETEST_VERIFY(expected <= actual + 1);
    BARETEST_VERIFY(expected >= actual - 1);
    BARETEST_VERIFY(expected < actual + 1);
    BARETEST_VERIFY(expected > actual - 1);
}

BARETEST_RUN;
```

The general structure of any test suite written with baretest is to first
include the baretest header using `#include <baretest/baretest.hpp>` followed by
the test cases. Finally test cases are run with the `BARETEST_RUN` macro.

A test case can be created with the `TEST_CASE` macro. The `TEST_CASE` macro
takes the test name as its sole argument. A test case name must be unique across
the test suite and follow the same rules as any function name.

The test case itself can be written like any normal function, with the exception
that a test case should not have any `return` statements.

Assertions & Expectations
-------------------------

Baretest provides only the basic boolean assertion that does nothing if its
arguments resolves to `true` and fails the test if its argument resolves to false.
The assertion is provided by the `BARETEST_ASSERT` macro.

Most other test frameworks provide helper assertions, like `ASSERT_EQUAL` to
test whether two arguments are equal, different, one is smaller than the other,
etc. Baretest goes a different way and uses an `expectation` object for this
purpose.

In order to specify an expectation you create the object which entails the
expected value (and its type) with a normal ctor call like this:
`baretest::expect<uint64_t> expected(42);`. Expectations can be verified with a
call to the `BARETEST_VERIFY` macro. In order to verify that two values are
equal one would call `BARETEST_VERIFY(expected == actual);`. This works as long
as `operator==` is defined for the type saved in the expectation and the type
of `actual`. The same can be done with `!=`, `<`, `>` etc.

If any call to `BARETEST_ASSERT` or `BARETEST_VERIFY` fails, the execution of
the test case is immediately aborted and the test case is marked as failure.


Test Suite Setup
----------------

If there is any setup code necessary in order to execute tests, just define the
`void preamble()` function. If it exists, this function is run *once*, before
the first test.

