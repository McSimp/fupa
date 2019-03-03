#ifndef PCH_H
#define PCH_H

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <fstream>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/format.h>
#include <Windows.Foundation.h>
#include <wrl\wrappers\corewrappers.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <locale>
#include <codecvt>
#include <wincodec.h>
#include <filesystem>

#include "rtech.h"
#include "ScreenGrab.h"

#endif //PCH_H
