#pragma once

namespace Util {
std::wstring Widen(const std::string& input);
std::filesystem::path GetRpakPath(std::filesystem::path basePath, std::string name, int pakNumber);
void ReplaceAll(std::string& source, const std::string& from, const std::string& to);
std::string HashToString(uint64_t hash);
bool EndsWith(const std::string& value, const std::string& ending);
}
