#pragma once

#pragma pack(push, 1)
struct SectionReference
{
    uint32_t Section;
    uint32_t Offset;
};

struct OuterHeader
{
    uint32_t Signature;
    uint16_t Version;
    uint16_t Flags;
    char Unknown1[16];
    uint64_t CompressedSize;
    uint64_t Unknown2;
    uint64_t DecompressedSize;
    uint64_t Unknown3;
    uint16_t StarpakPathBlockSize;
    uint16_t NumSlotDescriptors;
    uint16_t NumSections;
    uint16_t NumRPakLinks;
    uint32_t NumRelocations;
    uint32_t NumAssets;
    uint32_t Header72;
    uint32_t Header76;
    uint32_t Header80;
    uint32_t Header84;
};

static_assert(sizeof(OuterHeader) == 0x58, "Outer header must be 0x58 bytes");

/*
// Apex
struct OuterHeader
{
    char Unknown1[24];
    uint64_t CompressedSize;
    char Unknown2[16];
    uint64_t DecompressedSize;
    char Unknown3[72];
};

static_assert(sizeof(OuterHeader) == 0x80, "Outer header must be 0x80 bytes");
*/

struct LinkedRPakSize
{
    uint64_t SizeOnDisk;
    uint64_t DecompressedSize;
};

static_assert(sizeof(LinkedRPakSize) == 16, "LinkedRPakSize must be 16 bytes");

struct SlotDescriptor
{
    uint32_t Slot;
    uint32_t Alignment;
    uint64_t Size;
};

static_assert(sizeof(SlotDescriptor) == 16, "SlotDescriptor must be 16 bytes");

struct SectionDescriptor
{
    uint32_t SlotDescIndex;
    uint32_t Alignment;
    uint32_t Size;
};

static_assert(sizeof(SectionDescriptor) == 12, "SectionDescriptor must be 12 bytes");

struct AssetDefinition
{
    uint64_t ID;
    uint32_t Unknown3;
    uint32_t Unknown4;
    SectionReference MetadataRef;
    SectionReference DataRef;
    uint64_t Unknown5;
    uint16_t NumRequiredSections; // Number of sections required to have been loaded before asset can be processed
    uint16_t Unknown6;
    uint32_t Unknown7;
    uint32_t UnknownExtra8ByteIndex;
    uint32_t UnknownExtra8ByteStartIndex;
    uint32_t UnknownExtra8ByteNumEntries;
    uint32_t DataSize;
    uint32_t DataAlignment; // Not 100% sure on this
    uint32_t Type;
};

static_assert(sizeof(AssetDefinition) == 72, "AssetDefinition must be 72 bytes");

struct MaterialGlue
{
    char Unknown[24];
    char* Name;
};

struct ShdrMetadata
{
    char* Name;
};

struct CompressionInfo
{
    uint8_t BytesPerBlock;
    uint8_t BlockSize;
};

struct TextureMetadata
{
    uint64_t Unknown1;
    char* Name;
    uint16_t Width; // This is the max - might not actually contain this in the rpak
    uint16_t Height;
    uint16_t Unknown2_ShouldBeZero;
    uint16_t Format;
    uint32_t DataSize;
    uint32_t Unknown3;
    uint8_t Unknown4;
    uint8_t MipLevels;
    uint8_t SkippedMips; // Number of mip levels that aren't present (starting from largest size)
};

struct DatatableColumn
{
    char* Name;
    int32_t Type;
    int32_t Offset;
};

struct DatatableMetadata
{
    int32_t ColumnCount;
    int32_t RowCount;
    DatatableColumn* Columns;
    char* RowData;
    uint32_t RowSize;
};

struct PatchMetadata
{
    uint32_t Unknown1;
    uint32_t NumFiles;
    char** PakNames;
    uint8_t* PakNumbers;
};

#pragma pack(pop)
