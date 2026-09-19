#include "expected11/expected11.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

using expected11::expected;
using expected11::make_unexpected;
using expected11::SourceLocation;

struct NonDefaultError
{
    explicit NonDefaultError(int code) : code(code) {}
    int code;
};

struct ThrowingValue
{
    static bool throw_on_copy;
    explicit ThrowingValue(int value) : value(value) {}
    ThrowingValue(const ThrowingValue &other) : value(other.value)
    {
        if (throw_on_copy)
            throw std::runtime_error("copy failed");
    }
    int value;
};

bool ThrowingValue::throw_on_copy = false;

expected<int, std::exception> safe_divide(int a, int b)
{
    if (b == 0)
    {
        return make_unexpected(std::runtime_error("division by zero"), EXPECTED11_SOURCE_LOCATION());
    }
    return expected<int, std::exception>(a / b);
}

static expected<int, std::string> parse(const std::string &text)
{
    if (text == "42")
        return expected<int, std::string>(42);
        
    return make_unexpected(std::string("invalid"), EXPECTED11_SOURCE_LOCATION());
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

    expected<int, std::string> value(42);
    assert(value && value.has_value() && value.value() == 42);
    assert(*value == 42 && value.operator->() != 0 && value.value_or(7) == 42);

    expected<int, std::string> error = make_unexpected(std::string("bad"));
    assert(!error && error.error() == "bad" && error.value_or(7) == 7);
    assert((value == expected<int, std::string>(42)));
    assert(value == 42 && 42 == value && value != 7);
    assert(error == make_unexpected(std::string("bad")));
    assert(error != value);
    bool threw = false;
    try
    {
        error.value();
    }
    catch (const expected11::bad_expected_access<std::string> &ex)
    {
        threw = true;
        assert(ex.error() == "bad");
    }
    assert(threw);

    expected<int, std::exception> exception_error = make_unexpected(std::exception());
    assert(!exception_error && &exception_error.error() != 0);
    threw = false;
    try
    {
        exception_error.value();
    }
    catch (const expected11::bad_expected_access<std::exception> &ex)
    {
        threw = true;
        assert(&ex.error() != 0);
    }
    assert(threw);

    expected11::unexpected<std::string> located_error =
        EXPECTED11_MAKE_UNEXPECTED(std::string("located"));
    assert(located_error.error() == "located");
    assert(located_error.source_location().line() > 0);
    assert(located_error.source_location().file_name()[0] != '\0');

    expected<int, std::string> constructed(expected11::in_place, 9);
    expected<int, std::string> failed(expected11::unexpect, "nope");
    constructed = failed;
    assert(!constructed && constructed.error() == "nope");
    constructed.emplace(11);
    assert(constructed.value() == 11);
    constructed.swap(error);
    assert(!constructed && error.value() == 11);
    constructed = make_unexpected(std::string("assigned"));
    assert(!constructed && constructed.error() == "assigned" && constructed.error_or("fallback") == "assigned");

    assert(parse("42").and_then([](int n)
                                { return expected<std::string, std::string>(std::to_string(n)); })
               .value() == "42");
    assert(parse("42").transform([](int n)
                                 { return n + 1; })
               .value() == 43);
    assert(parse("x").transform_error([](std::string &message)
                                      { return message.size(); })
               .error() == 7u);

    expected<void, int> done;
    expected<void, int> not_done(make_unexpected(3));
    assert(done && !not_done && not_done.error() == 3);
    not_done.emplace();
    not_done.value();
    done.swap(not_done);
    assert(done && not_done);
    expected<void, NonDefaultError> no_error_default;
    expected<void, NonDefaultError> has_error(expected11::unexpect, 5);
    assert(no_error_default && !has_error && has_error.error().code == 5);
    assert((done == expected<void, int>()));
    expected<void, int> other_error(expected11::unexpect, 4);
    assert(not_done != other_error);

    expected<long, short> converted_source(42L);
    expected<int, long> converted_target(converted_source);
    assert(converted_target == 42);
    expected<int, long> converted_error(expected11::make_unexpected(static_cast<short>(3)));
    assert(!converted_error && converted_error.error() == 3L);
    expected<void, int> located_void = EXPECTED11_MAKE_UNEXPECTED(7);
    assert(!located_void && located_void.source_location().line() > 0);
    assert(done.and_then([]() { return expected<int, int>(9); }).value() == 9);
    assert(done.transform([]() { return 10; }).value() == 10);
    assert(done.transform([]() { }).has_value());
    expected<void, std::string> void_failure =
        EXPECTED11_MAKE_UNEXPECTED(std::string("void failure"));
    expected<int, std::string> chained_failure =
        void_failure.and_then([]() { return expected<int, std::string>(1); });
    assert(!chained_failure);
    assert(chained_failure.source_location().line() == void_failure.source_location().line());
    assert(void_failure.transform([]() { return 1; }).error() == "void failure");
    assert(void_failure.transform_error([](const std::string &message) {
        return message.size();
    }).error() == 12u);
    assert(done.or_else([](int) { return expected<void, int>(expected11::unexpect, 1); }).has_value());
    assert(!void_failure.or_else([](const std::string &message) {
        return expected<void, std::string>(expected11::unexpect, message);
    }));

    expected<ThrowingValue, int> throwing_value(expected11::in_place, 1);
    ThrowingValue replacement(2);
    ThrowingValue::throw_on_copy = true;
    threw = false;
    try
    {
        throwing_value.emplace(replacement);
    }
    catch (const std::runtime_error &)
    {
        threw = true;
    }
    ThrowingValue::throw_on_copy = false;
    assert(threw);
    assert(throwing_value.has_value());
    assert(throwing_value.value().value == 1);

    return 0;
}
