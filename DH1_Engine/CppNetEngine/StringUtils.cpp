#include "pch.h"
#include "StringUtils.h"

std::string cpp_net_engine::ToString(const std::wstring& wStr)
{
    if (wStr.empty())
    {
        return {};
    }

    std::string result;
    result.reserve(wStr.length() * 3);
	utf8::utf16to8(wStr.begin(), wStr.end(), std::back_inserter(result));
    result.shrink_to_fit();
	return result;
}

std::wstring cpp_net_engine::ToWstring(const std::string& utf8Str)
{
    if (utf8Str.empty())
    {
        return {};
    }
    std::wstring result;
    result.reserve(utf8Str.length());
    utf8::utf8to16(utf8Str.begin(), utf8Str.end(), std::back_inserter(result));
    result.shrink_to_fit();
    return result;
}