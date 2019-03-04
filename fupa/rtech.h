#pragma once

#include <string>

namespace rtech {

void Initialize(const std::string& dllPath);
uint64_t HashData(const char* data);
extern uint64_t(*SetupDecompressState)(void* pState, char* compressedData, int64_t alwaysFFFFFF, int64_t totalFileSize, int64_t startVirtualOffset, int64_t headerSize);
extern void(*DoDecompress)(void* pState, uint64_t totalBytesReadAndAcked, uint64_t someVal);
extern int64_t(*ConstructPatchArray)(uint8_t* inputArray, int32_t a2, const char* a3, uint8_t* a4, uint8_t* a5);

extern uint32_t* FirstDword;
extern uint32_t* SecondDword;
extern uint32_t* ThirdDword;

}
