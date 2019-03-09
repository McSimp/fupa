#include "pch.h"

namespace rtech {
uint64_t(*AlignedHashFunc)(const char* data);
uint64_t(*UnalignedHashFunc)(const char* data);
uint64_t(*SetupDecompressState)(void* pState, char* compressedData, int64_t alwaysFFFFFF, int64_t totalFileSize, int64_t startVirtualOffset, int64_t headerSize);
void(*DoDecompress)(void* pState, uint64_t totalBytesReadAndAcked, uint64_t someVal);
int64_t(*ConstructPatchArray)(uint8_t* inputArray, int32_t a2, const char* a3, uint8_t* a4, uint8_t* a5);

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

    char* base = reinterpret_cast<char*>(mem.AllocationBase);
    if (base == nullptr)
    {
        throw std::runtime_error("mem.AllocationBase was NULL");
    }

    AlignedHashFunc = reinterpret_cast<decltype(AlignedHashFunc)>(base + 0x3800);
    UnalignedHashFunc = reinterpret_cast<decltype(UnalignedHashFunc)>(base + 0x3810);
    SetupDecompressState = reinterpret_cast<decltype(SetupDecompressState)>(base + 0x4b80);
    DoDecompress = reinterpret_cast<decltype(DoDecompress)>(base + 0x4ea0);
    ConstructPatchArray = reinterpret_cast<decltype(ConstructPatchArray)>(base + 0x56c0);

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
