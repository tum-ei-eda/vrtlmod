#include "vrtlmod/util/utility.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
unsigned checks = 0;

std::string describe(const std::string &text)
{
    const char hex[] = "0123456789abcdef";
    std::string escaped = "\"";
    for (unsigned char ch : text)
    {
        if (ch >= 32 && ch <= 126 && ch != '\\' && ch != '"')
            escaped += ch;
        else
        {
            escaped += "\\x";
            escaped += hex[ch >> 4];
            escaped += hex[ch & 15];
        }
    }
    return escaped + '"';
}

void expect_value(const std::string &text, int expected)
{
    ++checks;
    int actual;
    try
    {
        actual = util::parse_int(text);
    }
    catch (const std::exception &error)
    {
        throw std::runtime_error(describe(text) + " unexpectedly rejected: " + error.what());
    }
    if (actual != expected)
        throw std::runtime_error(describe(text) + " expected " + std::to_string(expected) + ", got " +
                                 std::to_string(actual));
}

template <class Exception>
void expect_error(const std::string &text, const char *exception_name)
{
    ++checks;
    try
    {
        util::parse_int(text);
    }
    catch (const Exception &)
    {
        return;
    }
    catch (const std::exception &error)
    {
        throw std::runtime_error(describe(text) + " expected " + exception_name + ", got: " + error.what());
    }
    throw std::runtime_error(describe(text) + " accepted instead of throwing " + exception_name);
}

void check_boundaries()
{
    const int minimum = std::numeric_limits<int>::min();
    const int maximum = std::numeric_limits<int>::max();
    const std::string min_text = std::to_string(minimum);
    const std::string max_text = std::to_string(maximum);
    expect_value(min_text, minimum);
    expect_value(max_text, maximum);
    expect_value("+" + max_text, maximum);
    expect_value("-000" + min_text.substr(1), minimum);
    expect_value("+000" + max_text, maximum);
    expect_value(std::string(256, '0') + max_text, maximum);
    expect_value("-" + std::string(256, '0') + min_text.substr(1), minimum);

    const std::string above_max = std::to_string(static_cast<long long>(maximum) + 1);
    const std::string below_min = std::to_string(static_cast<long long>(minimum) - 1);
    for (const auto &text : { above_max, "+" + above_max, below_min, std::string(256, '9'), "+" + std::string(256, '9'),
                              "-" + std::string(256, '9') })
        expect_error<std::out_of_range>(text, "out_of_range");

    // Grammar errors take precedence even when the numeric prefix overflows.
    for (const auto &text : { above_max + "x", below_min + " ", std::string(256, '9') + "x" })
        expect_error<std::invalid_argument>(text, "invalid_argument");
}

void check_malformed()
{
    for (const std::string text :
         { "",     "+",   "-",   "++1",   "--1",   "+-1", "-+1",  " 1",    "1 ",     "\t1",
           "1\t",  "\n1", "1\n", "\r1",   "1\r",   "\v1", "1\v",  "\f1",   "1\f",    "1 2",
           "1\t2", "1+2", "1-2", "1.0",   ".1",    "1e3", "0x10", "-0x10", "+0x10",  "0XFF",
           "1U",   "1L",  "1_0", "1'000", "1,000", "NaN", "inf",  "1/2",   "12junk", "123abc" })
        expect_error<std::invalid_argument>(text, "invalid_argument");

    const std::string valid = "+123";
    for (unsigned index = 0; index <= valid.size(); ++index)
    {
        auto embedded_nul = valid;
        embedded_nul.insert(index, 1, '\0');
        expect_error<std::invalid_argument>(embedded_nul, "invalid_argument");
    }
    expect_error<std::invalid_argument>(std::string(1, '\0'), "invalid_argument");
    expect_error<std::invalid_argument>(std::string("\xff") + "1", "invalid_argument");
    expect_error<std::invalid_argument>("\xd9\xa1", "invalid_argument"); // Non-ASCII digit.
}

void check_roundtrips()
{
    for (const std::string text : { "0", "+0", "-0", "00", "+000", "-000" })
        expect_value(text, 0);
    expect_value("00123", 123);
    expect_value("+00123", 123);
    expect_value("-00123", -123);

    std::uint32_t random = 0x6b8b4567U;
    for (unsigned index = 0; index < 4096; ++index)
    {
        random = random * 1664525U + 1013904223U;
        const int magnitude = random % (static_cast<std::uint64_t>(std::numeric_limits<int>::max()) + 1);
        const int value = (index & 1) ? -magnitude : magnitude;
        const std::string digits = std::to_string(magnitude);
        expect_value(std::to_string(value), value);
        expect_value((value < 0 ? "-000" : "+000") + digits, value);
        if (value >= 0)
            expect_value("+" + digits, value);
    }
}
} // namespace

int main()
{
    try
    {
        check_boundaries();
        check_malformed();
        check_roundtrips();
        std::cout << "PASS " << checks << " strict integer parsing checks\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
