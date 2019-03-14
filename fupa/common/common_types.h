#pragma once

const uint32_t kRpakSignature = 0x6B615052; // RPak

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

struct SectionReference
{
    uint32_t Section;
    uint32_t Offset;
};

static_assert(sizeof(SectionReference) == 8, "SectionReference must be 8 bytes");

struct PatchMetadata
{
    uint32_t Unknown1;
    uint32_t NumFiles;
    char** PakNames;
    uint8_t* PakNumbers;
};

static_assert(sizeof(PatchMetadata) == 24, "PatchMetadata must be 24 bytes");

struct DatatableColumn
{
    char* Name;
    int32_t Type;
    int32_t Offset;
};

static_assert(sizeof(DatatableColumn) == 16, "DatatableColumn must be 16 bytes");

struct AtlasElement
{
    float U;
    float V;
    float Width; // In UV coordinates
    float Height; // In UV coordinates
};

struct AtlasElementPixelSize
{
    uint16_t Width;
    uint16_t Height;
};

struct AtlasUnknown
{
    uint32_t HalfHashName;
    uint16_t Unknown2;
    uint16_t NameStringOffset;
};

struct UIImageAtlasMetadata
{
    float Unknown1;
    float Unknown2;
    uint16_t FullWidth;
    uint16_t FullHeight;
    uint16_t NumElements;
    uint16_t Unknown3;
    void* pUnknown;
    AtlasElementPixelSize* PixelSizes; // array of NumElements
    void* pUnknown2;
    AtlasUnknown* UnknownEntries; // array of NumElements 8 byte entries
    char* ElementStrings; // pointer to blob of strings
    uint64_t TextureHash;
};

class PatchAsset : public BaseAsset<PatchAsset, PatchMetadata>
{
public:
    using BaseAsset<PatchAsset, PatchMetadata>::BaseAsset;
    std::map<std::string, int> BuildRPakMap();
};

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
