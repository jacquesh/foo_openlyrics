#include "stdafx.h"

#pragma warning(push, 0)
#include <comdef.h>
#pragma warning(pop)

#include "logging.h"
#include "mvtf/mvtf.h"
#include "win32_util.h"

size_t wide_to_narrow_string(unsigned int codepage, std::wstring_view wide, std::vector<char>& out_buffer)
{
    if(wide.empty())
    {
        return 0;
    }
    assert(wide.length() <= INT_MAX);

    // Ignore byte-order marks at the beginning of our UTF-16 text
    const wchar_t little_endian_bom = 0xFFFE;
    const wchar_t big_endian_bom = 0xFEFF;
    const wchar_t byte_order_mark = wide[0];
    size_t start_index = 0;
    if((byte_order_mark == little_endian_bom) || (byte_order_mark == big_endian_bom))
    {
        start_index = 1;
    }

    int bytes_required = WideCharToMultiByte(codepage,
                                             WC_ERR_INVALID_CHARS,
                                             &wide[start_index],
                                             int(wide.length() - start_index),
                                             nullptr,
                                             0,
                                             nullptr,
                                             nullptr);
    if(bytes_required <= 0)
    {
        return 0;
    }

    assert(bytes_required > 0);
    out_buffer.resize((size_t)bytes_required);
    int bytes_written = WideCharToMultiByte(codepage,
                                            WC_ERR_INVALID_CHARS,
                                            &wide[start_index],
                                            int(wide.length() - start_index),
                                            out_buffer.data(),
                                            bytes_required,
                                            nullptr,
                                            nullptr);
    assert(bytes_written == bytes_required);
    return (size_t)bytes_written;
}

size_t narrow_to_wide_string(unsigned int codepage, std::string_view narrow, std::vector<wchar_t>& out_buffer)
{
    assert(narrow.length() <= INT_MAX);
    int chars_required = MultiByteToWideChar(codepage,
                                             MB_ERR_INVALID_CHARS,
                                             narrow.data(),
                                             int(narrow.length()),
                                             nullptr,
                                             0);
    if(chars_required <= 0)
    {
        return 0;
    }

    assert(chars_required > 0);
    out_buffer.resize((size_t)chars_required);
    int chars_written = MultiByteToWideChar(codepage,
                                            MB_ERR_INVALID_CHARS,
                                            narrow.data(),
                                            int(narrow.length()),
                                            out_buffer.data(),
                                            chars_required);
    assert(chars_written == chars_required);
    return (size_t)chars_written;
}

std::tstring to_tstring(std::string_view string)
{
#ifdef UNICODE
    std::vector<wchar_t> tmp_wide;
    const size_t chars_written = narrow_to_wide_string(CP_UTF8, string, tmp_wide);
    const std::wstring result = std::wstring(tmp_wide.data(), chars_written);
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return string;
#endif // UNICODE
}

std::tstring to_tstring(const std::string& string)
{
    return to_tstring(std::string_view { string });
}

std::tstring to_tstring(const pfc::string8& string)
{
    return to_tstring(std::string_view { string.c_str(), string.length() });
}

std::string from_tstring(std::tstring_view string)
{
#ifdef UNICODE
    std::vector<char> tmp_narrow;
    const size_t bytes_written = wide_to_narrow_string(CP_UTF8, string, tmp_narrow);
    const std::string result = std::string(tmp_narrow.data(), bytes_written);
    return result;
#else // UNICODE
    static_assert(sizeof(TCHAR) == sizeof(char), "UNICODE is defined but TCHAR is not a char");
    return std::string(buffer, buffer_len);
#endif // UNICODE
}

std::string from_tstring(const std::tstring& string)
{
    return from_tstring(std::tstring_view(string));
}

std::tstring normalise_utf8(std::tstring_view input)
{
    if(input.empty())
    {
        return std::tstring();
    }

    // NOTE: fb2k docs specify that tags are UTF-8 encoded
    if(input.length() > INT_MAX)
    {
        LOG_WARN("Input string for UTF-8 normalisation is too long, skipping...");
        return std::tstring(input.data(), input.length());
    }

    int required_bytes = NormalizeString(NormalizationKD, input.data(), (int)input.length(), nullptr, 0);
    if(required_bytes <= 0)
    {
        LOG_WARN("Estimated number of bytes required for normalised string is negative, skipping normalisation...");
        return std::tstring(input.data(), input.length());
    }

    assert(required_bytes > 0);
    const int buffer_size = required_bytes + 1;
    TCHAR* buffer = new TCHAR[(size_t)buffer_size];
    int normalised_bytes = NormalizeString(NormalizationKD, input.data(), (int)input.length(), buffer, buffer_size);
    if(normalised_bytes <= 0)
    {
        LOG_WARN("Failed to normalise UTF-8 string with error %u. Skipping normalisation...", GetLastError());
        return std::tstring(input.data(), input.length());
    }

    assert(normalised_bytes > 0);
    std::tstring result(buffer, (size_t)normalised_bytes);
    delete[] buffer;

    return result;
}

bool is_char_whitespace(TCHAR c)
{
    // U+00A0 and U+202F are non-breaking spaces.
    // U+180E was classified as a space when microsoft first defined isspace, but was later removed from the standard.
    return (_istspace(c) > 0) && (c != L'\u00A0') && (c != L'\u202F') && (c != L'\u180E');
}

size_t find_first_whitespace(const std::tstring_view str, size_t pos)
{
    // match behavior of std::string_view::find_first_of
    if(pos >= str.length() || str.empty()) return std::tstring_view::npos;

    const auto it = std::find_if(std::next(str.begin(), pos), str.end(), is_char_whitespace);

    if(it == str.end()) return std::tstring_view::npos;

    return it - str.begin();
}

size_t find_first_nonwhitespace(const std::tstring_view str, size_t pos)
{
    // match behavior of std::string_view::find_first_not_of
    if(pos >= str.length() || str.empty()) return std::tstring_view::npos;

    const auto it = std::find_if_not(std::next(str.begin(), pos), str.end(), is_char_whitespace);

    if(it == str.end()) return std::tstring_view::npos;

    return it - str.begin();
}

size_t find_last_whitespace(const std::tstring_view str, size_t pos)
{
    if(str.empty()) return std::tstring_view::npos;

    size_t offset = 0;
    if(pos != std::tstring_view::npos && pos < str.length()) offset = str.length() - pos - 1;

    const auto it = std::find_if(std::next(str.rbegin(), offset), str.rend(), is_char_whitespace);
    if(it == str.rend()) return std::tstring_view::npos;

    return str.rend() - it - 1;
}

size_t find_last_nonwhitespace(const std::tstring_view str, size_t pos)
{
    if(str.empty()) return std::tstring_view::npos;

    size_t offset = 0;
    if(pos != std::tstring_view::npos && pos < str.length()) offset = str.length() - pos - 1;

    const auto it = std::find_if_not(std::next(str.rbegin(), offset), str.rend(), is_char_whitespace);
    if(it == str.rend()) return std::tstring_view::npos;

    return str.rend() - it - 1;
}

bool hr_success(HRESULT result, const char* filename, int line_number)
{
    const bool success = (result == S_OK);
    if(!success)
    {
        _com_error err(result);
        std::string err_msg = from_tstring(std::tstring_view(err.ErrorMessage()));
        LOG_WARN("HRESULT indicated failure @ %s:%d: 0x%x %s",
                 filename,
                 line_number,
                 uint32_t(result),
                 err_msg.c_str());
    }
    return success;
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(win32_string_wide_to_narrow_string_handles_bomless_ascii)
{
    std::wstring_view input = L"test string!\nwith a newline :O";
    std::vector<char> output_buffer;
    const size_t output_bytes = wide_to_narrow_string(CP_UTF8, input, output_buffer);
    ASSERT(output_bytes == 30);

    const std::string output = std::string(output_buffer.data(), output_bytes);
    ASSERT(output == "test string!\nwith a newline :O");
}

MVTF_TEST(win32_string_wide_to_narrow_string_handles_ascii_with_a_bom)
{
    std::wstring_view input = L"\xFFFEtest string!\nwith a newline :O";
    std::vector<char> output_buffer;
    const size_t output_bytes = wide_to_narrow_string(CP_UTF8, input, output_buffer);
    ASSERT(output_bytes == 30);

    const std::string output = std::string(output_buffer.data(), output_bytes);
    ASSERT(output == "test string!\nwith a newline :O");
}

MVTF_TEST(win32_string_narrow_to_wide_handles_ascii)
{
    std::string_view input = "test string!\nwith a newline :O";
    std::vector<wchar_t> output_buffer;
    const size_t output_chars = narrow_to_wide_string(CP_UTF8, input, output_buffer);
    ASSERT(output_chars == 30);

    const std::wstring output = std::wstring(output_buffer.data(), output_chars);
    ASSERT(output == L"test string!\nwith a newline :O");
}

MVTF_TEST(win32_is_char_whitespace_true_for_breaking_whitespace)
{
    ASSERT(is_char_whitespace(L'\t'));
    ASSERT(is_char_whitespace(L'\n'));
    ASSERT(is_char_whitespace(L'\v'));
    ASSERT(is_char_whitespace(L'\f'));
    ASSERT(is_char_whitespace(L'\r'));
    ASSERT(is_char_whitespace(L' '));

    ASSERT(is_char_whitespace(L'\u0085')); // Next line
    ASSERT(is_char_whitespace(L'\u1680')); // Ogham space mark

    ASSERT(is_char_whitespace(L'\u2000')); // En quad
    ASSERT(is_char_whitespace(L'\u2001')); // Em quad
    ASSERT(is_char_whitespace(L'\u2002')); // En space
    ASSERT(is_char_whitespace(L'\u2003')); // Em space
    ASSERT(is_char_whitespace(L'\u2004')); // Three-per-em space
    ASSERT(is_char_whitespace(L'\u2005')); // Four-per-em space
    ASSERT(is_char_whitespace(L'\u2006')); // Six-per-em space
    ASSERT(is_char_whitespace(L'\u2007')); // Figure space
    ASSERT(is_char_whitespace(L'\u2008')); // Punctuation space
    ASSERT(is_char_whitespace(L'\u2009')); // Thin space
    ASSERT(is_char_whitespace(L'\u200A')); // Hair space

    ASSERT(is_char_whitespace(L'\u2028')); // Line separator
    ASSERT(is_char_whitespace(L'\u2029')); // Paragraph separator
    ASSERT(is_char_whitespace(L'\u205F')); // Medium mathematical space
    ASSERT(is_char_whitespace(L'\u3000')); // Ideographic space

    ASSERT(!is_char_whitespace(L'\u00A0')); // Non-breaking space
    ASSERT(!is_char_whitespace(L'\u202F')); // Narrow non-breaking space
    ASSERT(!is_char_whitespace(L'\u180E')); // Mongolian vowel separator
    ASSERT(!is_char_whitespace(L'A'));
    ASSERT(!is_char_whitespace(L'1'));
    ASSERT(!is_char_whitespace(L'-'));
}

MVTF_TEST(win32_find_first_whitespace_gives_correct_indices)
{
    std::tstring_view input = _T("Test string.\u3000Second sentence.");
    ASSERT(find_first_whitespace(input) == 4);
    ASSERT(find_first_whitespace(input, 4) == 4);
    ASSERT(find_first_whitespace(input, 5) == 12);
    ASSERT(find_first_whitespace(input, 23) == std::tstring_view::npos);
    ASSERT(find_first_whitespace(input, 100) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_first_whitespace_empty_string)
{
    ASSERT(find_first_whitespace(_T("")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_first_whitespace_no_whitespace)
{
    ASSERT(find_last_whitespace(_T("abcdef")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_first_nonwhitespace_gives_correct_indices)
{
    std::tstring_view input = _T("   \u3000Test    string.    ");
    ASSERT(find_first_nonwhitespace(input) == 4);
    ASSERT(find_first_nonwhitespace(input, 4) == 4);
    ASSERT(find_first_nonwhitespace(input, 10) == 12);
    ASSERT(find_first_nonwhitespace(input, 20) == std::tstring_view::npos);
    ASSERT(find_first_nonwhitespace(input, 100) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_first_nonwhitespace_empty_string)
{
    ASSERT(find_first_nonwhitespace(_T("")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_first_nonwhitespace_no_nonwhitespace)
{
    ASSERT(find_last_nonwhitespace(_T("   ")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_last_whitespace_gives_correct_indices)
{
    std::tstring_view input = _T("Test string.\u3000Second sentence.");
    ASSERT(find_last_whitespace(input) == 19);
    ASSERT(find_last_whitespace(input, 19) == 19);
    ASSERT(find_last_whitespace(input, 15) == 12);
    ASSERT(find_last_whitespace(input, 2) == std::tstring_view::npos);
    ASSERT(find_last_whitespace(input, 100) == 19);
}

MVTF_TEST(win32_find_last_whitespace_empty_string)
{
    ASSERT(find_last_whitespace(_T("")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_last_whitespace_no_whitespace)
{
    ASSERT(find_last_whitespace(_T("abcdef")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_last_nonwhitespace_gives_correct_indices)
{
    std::tstring_view input = _T("   \u3000Test    string.    ");
    ASSERT(find_last_nonwhitespace(input) == 18);
    ASSERT(find_last_nonwhitespace(input, 18) == 18);
    ASSERT(find_last_nonwhitespace(input, 10) == 7);
    ASSERT(find_last_nonwhitespace(input, 3) == std::tstring_view::npos);
    ASSERT(find_last_nonwhitespace(input, 100) == 18);
}

MVTF_TEST(win32_find_last_nonwhitespace_empty_string)
{
    ASSERT(find_last_nonwhitespace(_T("")) == std::tstring_view::npos);
}

MVTF_TEST(win32_find_last_nonwhitespace_no_nonwhitespace)
{
    ASSERT(find_last_nonwhitespace(_T("   ")) == std::tstring_view::npos);
}
#endif
