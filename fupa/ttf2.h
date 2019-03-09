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
    uint64_t Hash;
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

#pragma pack(pop)

const uint32_t kPatchAssetType = 0x68637450; // Ptch
const uint32_t kTextureAssetType = 0x72747874; // txtr
const uint32_t kShaderAssetType = 0x72646873; // shdr
const uint32_t kMaterialAssetType = 0x6C74616D; // matl
const uint32_t kDatatableAssetType = 0x6C627464; // dtbl
const uint32_t kShaderSetAssetType = 0x73646873; // shds
const uint32_t kLCDScreenEffectAssetType = 0x64636C72; // rlcd
const uint32_t kRSONFileAssetType = 0x6e6f7372; // rson
const uint32_t kUIImageAtlasType = 0x676D6975; // uimg
const uint32_t kParticleScriptType = 0x6B737072; // rpsk
const uint32_t kAnimationRecordingType = 0x72696E61; // anir
const uint32_t kUIType = 0x6975; // ui
const uint32_t kUIFontAtlasType = 0x746E6F66; // font

struct PatchMetadata
{
    uint32_t Unknown1;
    uint32_t NumFiles;
    char** PakNames;
    uint8_t* PakNumbers;
};

class PatchAsset : public BaseAsset<PatchAsset, PatchMetadata>
{
public:
    using BaseAsset<PatchAsset, PatchMetadata>::BaseAsset;
    std::map<std::string, int> BuildRPakMap();
};
