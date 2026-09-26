#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

std::string normalize_url(const std::string &url);
std::string fetch_url(const std::string &url);
std::vector<std::string> extract_urls_from_html(const std::string &current_url, const std::string &html_page);

#endif
