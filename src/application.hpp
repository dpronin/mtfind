#pragma once

#include <iostream>

namespace mtfind
{

///
/// @brief          Class that is used for signleton of application's main parameters and requisites
///
class Application final
{
public:
    static Application &instance() noexcept
    {
        static Application app;
        return app;
    }

public:
    ///
    /// @brief
    /// This class describes a pattern validator that is dedicated to verifying
    /// a pattern provided to the application through the parameters
    ///
    class PatternValidator
    {
    public:
        template <typename T>
        bool operator()(T c) const noexcept
        {
            has_masked_symbols_ = has_masked_symbols_ || '?' == c;
            return 0 <= c && c <= 0x7E && '\n' != c && '\r' != c || '?' == c;
        }

        bool has_masked_symbols() const noexcept { return has_masked_symbols_; }

        void reset() noexcept { has_masked_symbols_ = false; }

    private:
        mutable bool has_masked_symbols_ = false;
    };

    ///
    /// @brief      The comparator for equalness for the pattern
    ///
    /// @return     The comparator
    ///
    auto masked_pattern_comparator() const noexcept
    {
        return [](auto c, auto p) { return '?' == p || c == p; };
    }

    ///
    /// @brief      Gets the validator for application's pattern provided through the arguments
    ///
    /// @return     Validator of a pattern
    ///
    PatternValidator pattern_validator() const noexcept { return {}; }

    ///
    /// @brief      Prints a help page.
    ///
    void help() noexcept
    {
        std::cout << R"help(
usage: mtfind INPUT MASK

    INPUT - an input file to process or stdin if '-' is specified
    MASK  - a pattern to seek words matching it

    A pattern should meet the following format (the rule is represented in EBNF):
        MASK = ASCII 7-bit symbol | ?, { ASCII 7-bit symbol | ? }

        ASCII 7-bit symbol - is a certain symbol from the ASCII symbols table encoded from 0 up to 127 including
        ?                  - matches any ASCII 7-bit symbol

examples:
    > mtfind input.txt "?ad"
        Will find words "bad", "mad", "sad", " ad", ";ad", etc. Whitespace symbols and separators also meet a pattern '?'

    > mtfind input.txt "??"
        Will split an input file into pairs of symbols

    > mtfild input.txt "hello"
        Will find words "hello" in input.txt

    > mtfild input.txt "wor:d"
        Will find words "wor:d" in input.txt. Colon symbol is as normal as letters and digits to search for matching

    > cat input.txt | mtfind - "wor:d"
        Will do the same as the previous example except that stdin is used, that is tied to stdout of 'cat' by pipelining
    )help"
                     "\n";
    }

private:
    Application() = default;
};

} // namespace mtfind
