#pragma once

namespace mtfind
{

class Application final
{
public:
    static Application& instance() noexcept
    {
        static Application app;
        return app;
    }

public:
    // comparator for equalness for the pattern
    auto pattern_comparator() noexcept { return [](auto c, auto p){ return '?' == p || c == p; }; }

    // define a validator for the pattern
    auto pattern_validator() noexcept { return [](auto c){ return 0 <= c && c <= 0x7E && '\n' != c && '\r' != c || '?' == c; }; }

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
    )help""\n";
    }

private:
    Application() = default;
};

} // namespace mtfind
