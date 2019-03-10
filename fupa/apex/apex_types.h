#pragma once
#ifdef APEX

const uint16_t kExpectedVersion = 8;

#pragma pack(push, 1)
struct OuterHeader
{
    uint32_t Signature;
    uint16_t Version;
    uint16_t Flags;
    char Unknown1[16];
    uint64_t CompressedSize;
    char Unknown2[16];
    uint64_t DecompressedSize;
    char Unknown3[16];
    uint16_t StarpakPathBlockSize;
    uint16_t SizeOfUnknownThingAfterStarpakBlock;
    uint16_t NumSlotDescriptors;
    uint16_t NumSections;
    uint16_t NumRPakLinks;
    uint16_t Unknown4;
    uint32_t NumRelocations;
    uint32_t NumAssets;
    uint32_t NumExtraHeader8Bytes;
    uint32_t NumExtraHeader4Bytes1;
    uint32_t NumExtraHeader4Bytes2;
    uint32_t NumExtraHeader1Bytes;
    uint32_t Unknown5;
    uint64_t Unknown6;
    uint64_t Unknown7;
};

static_assert(sizeof(OuterHeader) == 0x80, "Outer header must be 0x80 bytes");

struct AssetDefinition
{
    uint64_t Hash;
    uint32_t Unknown3;
    uint32_t Unknown4;
    SectionReference MetadataRef;
    SectionReference DataRef;
    char Unknown5[16];
    uint16_t NumRequiredSections; // Number of sections required to have been loaded before asset can be processed
    uint16_t Unknown6;
    uint32_t Unknown7;
    uint32_t UnknownExtra8ByteIndex;
    uint32_t UnknownExtra8ByteStartIndex;
    uint32_t UnknownExtra8ByteNumEntries;
    uint32_t MetadataSize;
    uint32_t MetadataAlignment; // Not 100% sure on this
    uint32_t Type;
};

static_assert(sizeof(AssetDefinition) == 80, "AssetDefinition must be 80 bytes");

struct DatatableMetadata
{
    int32_t ColumnCount;
    int32_t RowCount;
    DatatableColumn* Columns;
    char* RowData;
    uint64_t Unknown1;
    uint32_t RowSize;
    uint32_t Unknown2;
};

static_assert(sizeof(DatatableMetadata) == 40, "DatatableMetadata must be 40 bytes");

struct TextureMetadata
{
    uint64_t Unknown1;
    char* Name;
    uint16_t Width; // This is the max - might not actually contain this in the rpak
    uint16_t Height;
    uint16_t Unknown2_ShouldBeZero;
    uint16_t Format;
    uint32_t DataSize;
    uint8_t Unknown3;
    uint8_t SomeOtherSkippedMips; // I suspect this might be to do with the 'opt' starpaks - they are probably for the full game install, but you can still play when you've just got the normal starpaks. So this might be the number of mips in the opt starpak?
    uint16_t Unknown4;
    uint8_t TwoIfUsageDefaultOtherwiseImmutable;
    uint8_t MipLevels;
    uint8_t SkippedMips; // Number of mip levels that aren't present (starting from largest size)
};

#pragma pack(pop)
#endif
