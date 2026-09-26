#include "utils.h"
#include <curl/curl.h>
#include <gumbo.h>
#include <iostream>
#include <mutex>

std::string normalize_url(const std::string &url)
{
    if (url.find("http://") == 0 || url.find("https://") == 0)
        return url;
    return "https://" + url;
}

static std::once_flag curl_init_flag;

static void curl_ensure_init()
{
    std::call_once(curl_init_flag, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *user_data)
{
    user_data->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}

std::string fetch_url(const std::string &url)
{
    curl_ensure_init();
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; MyCrawler/1.0)");
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {

            //std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
        }

        curl_easy_cleanup(curl);
    }

    return response;
}

static void extract_urls_from_node(GumboNode *node, std::vector<std::string> &urls)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return;

    if (node->v.element.tag == GUMBO_TAG_A)
    {
        GumboAttribute *href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href && href->value)
        {
            urls.emplace_back(href->value);
        }
    }

    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        extract_urls_from_node(static_cast<GumboNode *>(children->data[i]), urls);
    }
}

std::vector<std::string> extract_urls_from_html(const std::string &current_url, const std::string &html_page)
{
    if (html_page.empty())
        return {};

    GumboOutput *output = gumbo_parse(html_page.c_str());
    if (!output || !output->root)
        return {};

    std::vector<std::string> raw_urls;
    extract_urls_from_node(output->root, raw_urls);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    std::string domain = current_url;
    size_t scheme_end = domain.find("://");
    if (scheme_end != std::string::npos)
    {
        size_t path_start = domain.find('/', scheme_end + 3);
        if (path_start != std::string::npos)
            domain = domain.substr(0, path_start);
    }

    std::vector<std::string> full_urls;
    for (auto &url : raw_urls)
    {
        if (url.empty() || url[0] == '#' || url.find("javascript:") == 0 || url.find("mailto:") == 0)
            continue;

        if (url.find("http://") == 0 || url.find("https://") == 0)
        {
            full_urls.push_back(url);
        }
        else if (url[0] == '/')
        {
            full_urls.push_back(domain + url);
        }
        else
        {
            std::string directory = current_url;
            size_t last_slash = directory.find_last_of('/');
            if (last_slash != std::string::npos && last_slash > scheme_end + 2)
                directory = directory.substr(0, last_slash + 1);
            else if (directory.back() != '/')
                directory += '/';

            full_urls.push_back(directory + url);
        }
    }
    return full_urls;
}
