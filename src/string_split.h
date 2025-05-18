#pragma once
// ============================================================================
// string_split: Easy, allocation-free string splitting
//
// Include this header in your source files and then in *one* translation unit,
// add the following definition:
//
// #define STRINGSPLIT_IMPLEMENTATION
//
// If you'd like to also run the unit tests then in that same translation unit,
// add the following definitions:
//
// #define STRINGSPLIT_TESTS_ENABLED
// #define STRINGSPLIT_TEST <TEST-DECLARATION-MACRO>
// #define STRINGSPLIT_ASSERT <TEST-ASSERT-MACRO>
// ============================================================================
#include <string_view>

struct string_split
{
    string_split(std::string_view source, std::string_view delimiter);

    std::string_view next();
    bool reached_the_end();
    bool failed();

private:
    std::string_view m_source;
    const std::string_view m_delimiter;
    bool m_failed;
};

#ifdef STRINGSPLIT_TESTS_ENABLED
#ifndef STRINGSPLIT_TEST
#error STRINGSPLIT_TEST must be defined whenever STRING_SPLIT_TESTS_ENABLED is defined
#endif // STRINGSPLIT_TEST not defined
#ifndef STRINGSPLIT_ASSERT
#error STRINGSPLIT_ASSERT must be defined whenever STRING_SPLIT_TESTS_ENABLED is defined
#endif // STRINGSPLIT_ASSERT not defined
#endif

#ifdef STRINGSPLIT_IMPLEMENTATION
string_split::string_split(std::string_view source, std::string_view delimiter)
    : m_source(source)
    , m_delimiter(delimiter)
    , m_failed(false)
{
}

std::string_view string_split::next()
{
    if(reached_the_end() || failed())
    {
        m_failed = true;
        return "";
    }

    size_t next_index = m_source.find(m_delimiter);
    if(next_index == std::string_view::npos)
    {
        const std::string_view result = m_source;
        m_source = "";
        return result;
    }

    const std::string_view result = m_source.substr(0, next_index);
    m_source = m_source.substr(next_index + m_delimiter.length());
    return result;
}

bool string_split::reached_the_end()
{
    return m_source.empty();
}

bool string_split::failed()
{
    return m_failed;
}

#ifdef STRINGSPLIT_TESTS_ENABLED
STRINGSPLIT_TEST(stringsplit_splits_with_single_character_delimiter)
{
    string_split split("qwe asd zxc", " ");
    STRINGSPLIT_ASSERT(split.next() == "qwe");
    STRINGSPLIT_ASSERT(split.next() == "asd");
    STRINGSPLIT_ASSERT(split.next() == "zxc");
    STRINGSPLIT_ASSERT(split.reached_the_end());
    STRINGSPLIT_ASSERT(!split.failed());
}

STRINGSPLIT_TEST(stringsplit_splits_with_multi_character_delimiter)
{
    string_split split("qwe~|~asd~|~zxc", "~|~");
    STRINGSPLIT_ASSERT(split.next() == "qwe");
    STRINGSPLIT_ASSERT(split.next() == "asd");
    STRINGSPLIT_ASSERT(split.next() == "zxc");
    STRINGSPLIT_ASSERT(split.reached_the_end());
    STRINGSPLIT_ASSERT(!split.failed());
}

STRINGSPLIT_TEST(stringsplit_always_returns_empty_string_after_failure)
{
    string_split split("qwe asd", " ");
    STRINGSPLIT_ASSERT(split.next() == "qwe");
    STRINGSPLIT_ASSERT(split.next() == "asd");
    STRINGSPLIT_ASSERT(!split.failed());

    STRINGSPLIT_ASSERT(split.next() == "");
    STRINGSPLIT_ASSERT(split.failed());

    STRINGSPLIT_ASSERT(split.next() == "");
    STRINGSPLIT_ASSERT(split.failed());
}

STRINGSPLIT_TEST(stringsplit_returns_the_entire_string_if_no_delimiter_found)
{
    string_split split("qwe asd", "-");
    STRINGSPLIT_ASSERT(split.next() == "qwe asd");
    STRINGSPLIT_ASSERT(split.reached_the_end());
    STRINGSPLIT_ASSERT(!split.failed());
}
#endif
#endif // STRINGSPLIT_IMPLEMENTATION
