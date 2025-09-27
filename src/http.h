#pragma once
#include <string>

namespace foobar2000_io
{
    class abort_callback;
}

namespace http
{
    struct Result
    {
        bool completed_successfully;
        long response_status;
        std::string response_content;
        std::string error_message;

        bool is_success() const;
    };

    Result get_http2(const std::string& url, foobar2000_io::abort_callback& abort);
}
