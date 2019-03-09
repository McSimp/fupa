#pragma once

namespace Util {
std::wstring Widen(const std::string& input);
std::filesystem::path GetRpakPath(std::filesystem::path basePath, std::string name, int pakNumber);
}
