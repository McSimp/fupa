#pragma once

#include <string>

namespace rtech {

void Initialize(const std::string& dllPath);
uint64_t HashData(const char* data);

}
