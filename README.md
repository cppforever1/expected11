# expected11

`expected11` is a small, header-only C++11 implementation of the core C++23 `std::expected` API. It stores either a value or an error without exceptions for normal control flow and has no third-party dependencies.

## Requirements

- C++11 compiler
- CMake 3.10 or newer (for the test project)

## Usage

```cpp
#include <expected11/expected11.hpp>

expected<int, std::exception> safe_divide(int a, int b)
{
    if (b == 0)
    {
        return make_unexpected(std::runtime_error("division by zero"), EXPECTED11_SOURCE_LOCATION());
    }
    return expected<int, std::exception>(a / b);
}

int main() 
{
    // test div function
    expected<int, std::exception> result = safe_divide(10, 2);
    assert(result && result.value() == 5);
    expected<int, std::exception> div_by_zero = safe_divide(10, 0);
    assert(!div_by_zero);
    assert(div_by_zero.error().what() != 0);
    auto location = div_by_zero.source_location();
    assert(location.line() > 0);
    assert(location.file_name()[0] != '\0');
    assert(location.function_name()[0] != '\0');
    expected<int, std::exception> copied_error = div_by_zero;
    assert(copied_error.source_location().line() == location.line());
    expected<int, std::exception> assigned_error(0);
    assigned_error = div_by_zero;
    assert(assigned_error.source_location().line() == location.line());

    return 0;
}
```

The public surface includes `expected<T, E>`, `expected<void, E>`, `unexpected<E>`, `make_unexpected`, `bad_expected_access`, `in_place`, `unexpect`, `and_then`, `transform`, `transform_error`, `or_else`, `swap`, `value_or`, `error_or`, and equality comparisons.

Compatible value and error types can be converted:

```cpp
expected11::expected<long, short> source(42L);
expected11::expected<int, long> converted(source);
```

`unexpected<E>::source_location()` exposes optional source metadata. Use `EXPECTED11_MAKE_UNEXPECTED(error)` when the error should capture the call site; plain `make_unexpected(error)` remains available without location capture.

```cpp
#include <iostream>

expected11::unexpected<std::string> unexpected_error =
  EXPECTED11_MAKE_UNEXPECTED(std::string("not found"));

expected11::SourceLocation location = unexpected_error.source_location();
std::cout << location.file_name() << ':' << location.line() << '\n';
```

When the `unexpected` value is converted to an `expected`, the location is preserved:

```cpp
expected11::expected<int, std::string> result = unexpected_error;
location = result.source_location();
```

## SourceLocation

C++11 has no built-in source location type. `SourceLocation` provides file, function, and line fields, and `EXPECTED11_SOURCE_LOCATION()` captures the call site:

```cpp
expected11::SourceLocation location = EXPECTED11_SOURCE_LOCATION();
```

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The test target is compiled with `-std=c++11`, warnings enabled, and warnings treated as errors.

## Exception safety

Union lifetime state is tracked explicitly. Replacement values and errors are constructed before the current state is changed, so a throwing replacement constructor preserves the original value or error. The final commit can still inherit exceptions from move and swap operations supplied by `T` and `E`.
