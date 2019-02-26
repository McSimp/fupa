#include "pch.h"

namespace rtech {
uint64_t(*AlignedHashFunc)(const char* data);
uint64_t(*UnalignedHashFunc)(const char* data);

void Initialize(const std::string& dllPath)
{
    // Load rtech_game.dll - we need some functions from it
    if (!SetDllDirectoryA(dllPath.c_str()))
    {
        throw std::runtime_error("Failed to set DLL search directory to Titanfall 2 directory");
    }

    HANDLE hRtechLib = LoadLibrary(L"rtech_game.dll");
    if (!hRtechLib)
    {
        throw std::runtime_error(fmt::format("Failed to load rtech_game.dll (Win32 Error = 0x{:x})", GetLastError()));
    }

    MEMORY_BASIC_INFORMATION mem;
    if (!VirtualQuery(hRtechLib, &mem, sizeof(mem)))
    {
        throw std::runtime_error(fmt::format("VirtualQuery returned NULL (Win32 Error = 0x{:x})", GetLastError()));
    }

    char* base = (char*)mem.AllocationBase;
    if (base == nullptr)
    {
        throw std::runtime_error("mem.AllocationBase was NULL");
    }

    rtech::AlignedHashFunc = reinterpret_cast<decltype(rtech::AlignedHashFunc)>(base + 0x3800);
    rtech::UnalignedHashFunc = reinterpret_cast<decltype(rtech::UnalignedHashFunc)>(base + 0x3810);

    // TODO: FreeLibrary at end of program
}

// TODO: Implement this myself
uint64_t HashData(const char* data)
{
    if ((reinterpret_cast<uint64_t>(data) & 3) != 0)
    {
        return UnalignedHashFunc(data);
    }
    else
    {
        return AlignedHashFunc(data);
    }
}
}
