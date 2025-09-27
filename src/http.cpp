#include "stdafx.h"

#define CURL_STATICLIB
#include "curl/curl.h"
#include "curl/multi.h"
#include "http.h"
#include "logging.h"
#include "openlyrics_version.h" // Defines OPENLYRICS_VERSION

static void on_init()
{
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if(result != 0)
    {
        LOG_WARN("Failed to initial global libcurl state: %s", curl_easy_strerror(result));
    }
}

static void on_quit()
{
    curl_global_cleanup();
}

FB2K_ON_INIT_STAGE(on_init, init_stages::before_library_init);
FB2K_RUN_ON_QUIT(on_quit);

bool http::Result::is_success() const
{
    return completed_successfully && (response_status < 300);
}

static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    size_t content_len = size * nmemb;
    userp->append(static_cast<char*>(contents), content_len);
    return content_len;
}

http::Result http::get_http2(const std::string& url, abort_callback& abort)
{
    CURLM* multi = curl_multi_init();
    if(multi == nullptr)
    {
        Result result = {};
        result.error_message = "Failed to initialise curl-multi handle";
        return result;
    }

    CURL* curl = curl_easy_init();
    if(curl == nullptr)
    {
        curl_multi_cleanup(multi);

        Result result = {};
        result.error_message = "Failed to initialise curl-easy handle";
        return result;
    }

    char error_message[CURL_ERROR_SIZE] = {};
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_message);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "foo_openlyrics/" OPENLYRICS_VERSION);

    // Force HTTP/2
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

    Result result = {};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.response_content);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    const CURLMcode add_result = curl_multi_add_handle(multi, curl);
    if(add_result != CURLM_OK)
    {
        curl_easy_cleanup(curl);
        curl_multi_cleanup(multi);
        result.error_message = curl_multi_strerror(add_result);
        return result;
    }

    int num_running_handles = 1;
    while((num_running_handles > 0) && !abort.is_aborting())
    {
        const CURLMcode perform_result = curl_multi_perform(multi, &num_running_handles);
        if(perform_result != CURLM_OK)
        {
            LOG_WARN("Failed to perform request activity on curl multi handle: %s. Aborting request.",
                     curl_multi_strerror(perform_result));
            break;
        }

        // Wait for socket activity for up to 2ms, will return immediately if there are no handles to wait for
        const CURLMcode wait_result = curl_multi_wait(multi, nullptr, 0, 2, nullptr);
        if(wait_result != CURLM_OK)
        {
            LOG_WARN("Failed to wait for activity on curl multi handle: %s. Aborting request.",
                     curl_multi_strerror(wait_result));
            break;
        }
    }

    int msgs_remaining = 0;
    const CURLMsg* msg = curl_multi_info_read(multi, &msgs_remaining);
    if((msg != nullptr) && (msg->msg == CURLMSG_DONE))
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.response_status);

        const CURLcode curl_error = msg->data.result;
        result.completed_successfully = (curl_error == CURLE_OK);
        if(error_message[0] != '\0')
        {
            result.error_message = error_message;
        }
        else if(curl_error != CURLE_OK)
        {
            result.error_message = curl_easy_strerror(curl_error);
        }
    }
    else if(!abort.is_aborting())
    {
        if(msg == nullptr)
        {
            LOG_WARN("Received unexpected info read result from curl: Null message");
        }
        else
        {
            LOG_WARN("Received unexpected info read result from curl: Message type %d", int(msg->msg));
        }
    }

    curl_multi_remove_handle(multi, curl);
    curl_easy_cleanup(curl);
    curl_multi_cleanup(multi);
    return result;
}
