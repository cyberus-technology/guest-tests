/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/util/string.hpp>

std::vector<std::string> util::string::split(const std::string& source, char delimiter)
{
    if (source.empty()) {
        // return empty vector
        return {};
    }

    std::vector<std::string> items{};
    auto string{ source };
    size_t pos{ 0 };
    while ((pos = string.find(delimiter)) != std::string::npos) {
        auto item{ string.substr(0, pos) };
        items.push_back(item);
        string.erase(0, pos + 1 /* +1: delimiter */);
    }
    // push what's left
    items.push_back(string);
    return items;
}
