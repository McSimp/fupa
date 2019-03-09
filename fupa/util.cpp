#include "pch.h"

namespace Util {

// Taken from https://stackoverflow.com/a/18374698
std::wstring Widen(const std::string& input)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.from_bytes(input);
}

std::filesystem::path GetRpakPath(std::filesystem::path basePath, std::string name, int pakNumber)
{
    if (pakNumber == 0)
    {
        return basePath / (name + ".rpak");
    }
    else
    {
        return basePath / fmt::format("{}({:02d}).rpak", name, pakNumber);
    }
}

void ReplaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

}
