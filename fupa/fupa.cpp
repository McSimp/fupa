#include "pch.h"

#pragma comment (lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device> dev;
ComPtr<ID3D11DeviceContext> devCon;

class IFileReader
{
public:
    virtual void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes = 0) = 0;
};

class PreprocessedFileReader : public IFileReader
{
public:
    PreprocessedFileReader(std::string filename) :
        m_file(filename, std::ios::in | std::ios::binary)
    {
        if (!m_file.is_open())
        {
            throw std::runtime_error("Failed to open file");
        }
    }

    void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes) override
    {
        m_file.seekg(skipBytes, std::ios::cur);
        m_file.read(buffer, bytesToRead);
        spdlog::trace("Offset after reading 0x{:x} bytes (skipping 0x{:x}): 0x{:x}", bytesToRead, skipBytes, m_file.tellg());
        if (m_file.fail())
        {
            throw std::runtime_error("File read failed");
        }
    }

private:
    std::ifstream m_file;
};

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
    char Unknown1[32];
    uint64_t DecompressedSize;
    uint64_t Unknown2;
    uint16_t StarpakPathBlockSize;
    uint16_t NumSlotDescriptors;
    uint16_t NumSections;
    uint16_t Header62;
    uint32_t NumRelocations;
    uint32_t NumAssets;
    uint32_t Header72;
    uint32_t Header76;
    uint32_t Header80;
    uint32_t Header84;
};

static_assert(sizeof(OuterHeader) == 0x58, "Outer header must be 0x58 bytes");

struct Header62Elem
{
    unsigned char Unknown[18];
};

static_assert(sizeof(Header62Elem) == 18, "Header62Elem must be 18 bytes");

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

struct BaseAssetMetadata
{
    uint64_t Unknown;
    char* Name;
};

struct MaterialGlue
{
    char Unknown[24];
    char* Name;
};

struct ShdrMetadata
{
    char* Name;
};

DXGI_FORMAT TEXTURE_FORMATS[] = {
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D16_UNORM
};

struct CompressionInfo
{
    uint8_t BytesPerBlock;
    uint8_t BlockSize;
};

CompressionInfo COMPRESSION_INFO[] = {
    { 8, 4 },
    { 8, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 8, 4 },
    { 8, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 12, 1 },
    { 12, 1 },
    { 12, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 2, 1 },
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

const char* DATATABLE_TYPES[] = {
    "bool",
    "int",
    "float",
    "vector",
    "string",
    "asset",
    "asset_noprecache"
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

#pragma pack(pop)

void ReplaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

const uint32_t kNumSlots = 4;

class PakFile
{
public:
    PakFile(std::string name, std::unique_ptr<IFileReader> reader) :
        m_reader(std::move(reader)),
        m_name(std::move(name))
    {

    }

    void Initialize()
    {
        // Read the outer header
        spdlog::debug("Initializing {}", m_name);
        spdlog::debug("Reading outer header...");
        m_reader->ReadData(reinterpret_cast<char*>(&m_outerHeader), sizeof(m_outerHeader));

        if (m_outerHeader.Signature != 0x6B615052)
        {
            throw std::runtime_error("Signature of file does not match");
        }

        if (m_outerHeader.Version != 7)
        {
            throw std::runtime_error(fmt::format("Version {} does not match expected version {}", m_outerHeader.Version, 7));
        }

        // Print out the header values
        spdlog::debug("====== Outer Header ======");
        spdlog::debug("Flags: 0x{:x}", m_outerHeader.Flags);
        spdlog::debug("DecompressedSize: 0x{:x}", m_outerHeader.DecompressedSize);
        spdlog::debug("StarpakPathBlockSize: 0x{:x}", m_outerHeader.StarpakPathBlockSize);
        spdlog::debug("NumSlotDescriptors: {}", m_outerHeader.NumSlotDescriptors);
        spdlog::debug("NumSections: {}", m_outerHeader.NumSections);
        spdlog::debug("Header62: {}", m_outerHeader.Header62);
        spdlog::debug("NumRelocations: {}", m_outerHeader.NumRelocations);
        spdlog::debug("NumAssets: {}", m_outerHeader.NumAssets);
        spdlog::debug("Header72: {}", m_outerHeader.Header72);
        spdlog::debug("Header76: {}", m_outerHeader.Header76);
        spdlog::debug("Header80: {}", m_outerHeader.Header80);
        spdlog::debug("Header84: {}", m_outerHeader.Header84);

        // Read the data before starpak paths if applicable
        if (m_outerHeader.Header62 != 0)
        {
            m_reader->ReadData(reinterpret_cast<char*>(&m_unknownBlockSize), sizeof(m_unknownBlockSize));
            m_reader->ReadData(reinterpret_cast<char*>(&m_beforeStarpakSecond), sizeof(m_beforeStarpakSecond));

            m_header62Elems = std::make_unique<Header62Elem[]>(m_outerHeader.Header62);
            m_reader->ReadData(reinterpret_cast<char*>(m_header62Elems.get()), sizeof(Header62Elem) * m_outerHeader.Header62);

            spdlog::debug("====== Pre Starpak Paths ======");
            spdlog::debug("Unknown Block Size: 0x{:x}", m_unknownBlockSize);
            spdlog::debug("Second: 0x{:x}", m_beforeStarpakSecond);
        }

        // Read starpak paths
        if (m_outerHeader.StarpakPathBlockSize == 0)
        {
            throw std::runtime_error("StarpakPathBlockSize was 0");
        }

        spdlog::debug("====== Starpak Paths ======");
        spdlog::debug("Size: 0x{:x}", m_outerHeader.StarpakPathBlockSize);

        std::unique_ptr<char[]> starpakBlock = std::make_unique<char[]>(m_outerHeader.StarpakPathBlockSize);
        m_reader->ReadData(starpakBlock.get(), m_outerHeader.StarpakPathBlockSize);

        size_t offset = 0;
        while (offset < m_outerHeader.StarpakPathBlockSize)
        {
            const char* path = starpakBlock.get() + offset;
            size_t pathLen = strlen(path);
            if (pathLen > 0)
            {
                m_starpakPaths.emplace_back(path, pathLen);
                spdlog::debug("{}", m_starpakPaths.back());
                if (m_starpakPaths.back().find("_hotswap.starpak") != std::string::npos)
                {
                    throw std::runtime_error("Unexpected hotswap starpak present in file");
                }
                offset += pathLen + 1;
            }
            else
            {
                break;
            }
        }

        if (m_outerHeader.NumSlotDescriptors == 0)
        {
            throw std::runtime_error("NumSlotDescriptors was 0");
        }

        // Read slot descriptors and allocate memory
        spdlog::debug("====== Slot Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);
        
        m_slotDescriptors = std::make_unique<SlotDescriptor[]>(m_outerHeader.NumSlotDescriptors);
        m_reader->ReadData(reinterpret_cast<char*>(m_slotDescriptors.get()), sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);

        // Calculate sizes, offsets, and alignments
        uint64_t slotSizes[kNumSlots] = {};
        uint64_t slotDescOffsets[20] = {};
        uint32_t slotAlignments[kNumSlots] = {};
        for (uint16_t i = 0; i < m_outerHeader.NumSlotDescriptors; i++)
        {
            SlotDescriptor& slotDesc = m_slotDescriptors[i];
            uint32_t slotNum = slotDesc.Slot & 3; // TODO: This will need to be updated if total slots changes
            slotAlignments[slotNum] = std::max(slotAlignments[slotNum], slotDesc.Alignment);

            uint64_t offset = (slotSizes[slotNum] + slotDesc.Alignment - 1) & ~static_cast<uint64_t>(slotDesc.Alignment - 1);
            slotDescOffsets[i] = offset;

            slotSizes[slotNum] = offset + slotDesc.Size;

            spdlog::debug("{}: SlotNum: {}, Alignment: 0x{:x}, Size: 0x{:x}, Offset: 0x{:x}", i, slotNum, slotDesc.Alignment, slotDesc.Size, offset);
        }

        spdlog::debug("====== Slot Totals ======");
        for (uint32_t i = 0; i < kNumSlots; i++)
        {
            spdlog::debug("{}: Size: 0x{:x}, Alignment: 0x{:x}", i, slotSizes[i], slotAlignments[i]);
        }

        // Allocate memory for slot data
        for (uint32_t i = 0; i < kNumSlots; i++)
        {
            if (slotSizes[i] > 0)
            {
                m_slotData[i] = static_cast<char*>(_aligned_malloc(slotSizes[i], slotAlignments[i]));
            }
        }

        // Read section information and make array of pointers to each of them
        spdlog::debug("====== Section Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SectionDescriptor) * m_outerHeader.NumSections);

        m_sectionDescriptors = std::make_unique<SectionDescriptor[]>(m_outerHeader.NumSections);
        m_reader->ReadData(reinterpret_cast<char*>(m_sectionDescriptors.get()), sizeof(SectionDescriptor) * m_outerHeader.NumSections);

        m_sectionPointers = std::make_unique<char*[]>(m_outerHeader.NumSections);

        for (uint16_t i = 0; i < m_outerHeader.NumSections; i++)
        {
            SectionDescriptor& sectDesc = m_sectionDescriptors[i];
            uint64_t offset = (slotDescOffsets[sectDesc.SlotDescIndex] + sectDesc.Alignment - 1) & ~static_cast<uint64_t>(sectDesc.Alignment - 1);
            m_sectionPointers[i] = m_slotData[m_slotDescriptors[sectDesc.SlotDescIndex].Slot & 3] + offset;
            slotDescOffsets[sectDesc.SlotDescIndex] = offset + sectDesc.Size;
            spdlog::debug("{}: SlotDescIdx: {}, Alignment: 0x{:x}, Size: 0x{:x}, Offset: 0x{:x}", i, sectDesc.SlotDescIndex, sectDesc.Alignment, sectDesc.Size, offset);
        }

        // Read relocation information
        spdlog::debug("====== Relocation Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SectionReference) * m_outerHeader.NumRelocations);

        m_relocationDescriptors = std::make_unique<SectionReference[]>(m_outerHeader.NumRelocations);
        m_reader->ReadData(reinterpret_cast<char*>(m_relocationDescriptors.get()), sizeof(SectionReference) * m_outerHeader.NumRelocations);

        for (uint32_t i = 0; i < m_outerHeader.NumRelocations; i++)
        {
            SectionReference& reloc = m_relocationDescriptors[i];
            spdlog::trace("{}: Section: {}, Offset: 0x{:x}", i, reloc.Section, reloc.Offset);
        }

        // Read asset definitions
        spdlog::debug("====== Asset Definitions ======");
        spdlog::debug("Size: 0x{:x}", sizeof(AssetDefinition) * m_outerHeader.NumAssets);

        m_assetDefinitions = std::make_unique<AssetDefinition[]>(m_outerHeader.NumAssets);
        m_reader->ReadData(reinterpret_cast<char*>(m_assetDefinitions.get()), sizeof(AssetDefinition) * m_outerHeader.NumAssets);
        for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
        {
            char* assetType = reinterpret_cast<char*>(&m_assetDefinitions[i].Type);
            spdlog::trace("{}: {:.4s}", i, assetType);
        }

        // Read extra header
        spdlog::debug("====== Extra Header ======");
        size_t extraHeaderSize = (m_outerHeader.Header72 * 8) + (m_outerHeader.Header76 * 4) + (m_outerHeader.Header80 * 4) + m_outerHeader.Header84;
        spdlog::debug("Size: 0x{:x}", extraHeaderSize);

        m_extraHeader = std::make_unique<char[]>(extraHeaderSize);
        m_reader->ReadData(reinterpret_cast<char*>(m_extraHeader.get()), extraHeaderSize);

        // Read unknown block
        if (m_outerHeader.Header62 != 0)
        {
            spdlog::debug("====== Unknown Block ======");
            spdlog::debug("Size: 0x{:x}", m_unknownBlockSize);

            m_unknownBlock = std::make_unique<char[]>(m_unknownBlockSize);
            m_reader->ReadData(reinterpret_cast<char*>(m_unknownBlock.get()), m_unknownBlockSize);
        }
    }

    void LoadAllSections()
    {
        // Sectors are stored sequentially after the header information, so just read them in order
        for (uint32_t i = 0; i < m_outerHeader.NumSections; i++)
        {
            if (m_sectionDescriptors[i].Size > 0)
            {
                m_reader->ReadData(m_sectionPointers[i], m_sectionDescriptors[i].Size);
            }
        }
    }

    bool IsReferenceValid(SectionReference& ref)
    {
        if (ref.Section >= m_outerHeader.NumSections)
        {
            return false;
        }

        if (ref.Offset >= m_sectionDescriptors[ref.Section].Size)
        {
            return false;
        }

        return true;
    }

    void ApplyRelocations()
    {
        // Each relocation entry is a reference to some offset in a section. Located at this offset is another reference
        // to a section and an offset. This latter reference is converted into a real pointer (8 bytes since we only deal
        // with 64-bit) and written to the location specified in the relocation.
        for (uint32_t i = 0; i < m_outerHeader.NumRelocations; i++)
        {
            SectionReference& relocLoc = m_relocationDescriptors[i];
            if (!IsReferenceValid(relocLoc))
            {
                throw std::runtime_error(fmt::format("Relocation descriptor {} is invalid: offset 0x{:x} in section {}", i, relocLoc.Offset, relocLoc.Section));
            }

            // Get the reference at the location referenced by the relocation entry
            SectionReference* ref = reinterpret_cast<SectionReference*>(m_sectionPointers[relocLoc.Section] + relocLoc.Offset);
            if (!IsReferenceValid(*ref))
            {
                throw std::runtime_error(fmt::format("Relocation {}'s inner reference is invalid: offset 0x{:x} in section {}", i, ref->Offset, ref->Section));
            }

            // Update the reference data to be a real pointer
            *reinterpret_cast<char**>(ref) = m_sectionPointers[ref->Section] + ref->Offset;
        }
    }

    void SaveTexture(TextureMetadata* metadata, char* textureData)
    {
        const uint64_t MIP_ALIGNMENT = 16;
        D3D11_TEXTURE2D_DESC desc;
        D3D11_SUBRESOURCE_DATA subResources[16];

        if (metadata->Unknown2_ShouldBeZero)
        {
            spdlog::error("Cannot dump texture - Unknown2_ShouldBeZero was non-zero");
            return;
        }

        if (metadata->Height == 0)
        {
            spdlog::error("Cannot dump texture - Height was zero");
            return;
        }

        char* nextTextureData = textureData;
        for (int32_t mip = metadata->MipLevels + metadata->SkippedMips - 1; mip >= metadata->SkippedMips; mip--)
        {
            int32_t width = std::max(1, metadata->Width >> mip);
            int32_t height = std::max(1, metadata->Height >> mip);

            uint8_t bytesPerBlock = COMPRESSION_INFO[metadata->Format].BytesPerBlock;
            uint8_t blockSize = COMPRESSION_INFO[metadata->Format].BlockSize;

            subResources[mip].pSysMem = nextTextureData;
            subResources[mip].SysMemPitch = bytesPerBlock * ((width + blockSize - 1) / blockSize);
            subResources[mip].SysMemSlicePitch = bytesPerBlock * ((width + blockSize - 1) / blockSize) * ((height + blockSize - 1) / blockSize);

            nextTextureData += (subResources[mip].SysMemSlicePitch + MIP_ALIGNMENT - 1) & ~MIP_ALIGNMENT;
        }

        desc.Width = std::max(1, metadata->Width >> metadata->SkippedMips);
        desc.Height = std::max(1, metadata->Height >> metadata->SkippedMips);
        desc.MipLevels = metadata->MipLevels;
        desc.ArraySize = 1;
        desc.Format = TEXTURE_FORMATS[metadata->Format];
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT; // TTF2 has this as D3D11_USAGE_IMMUTABLE
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // TTF2 has this as 0
        desc.MiscFlags = 0;

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = dev->CreateTexture2D(&desc, &subResources[metadata->SkippedMips], &tex);
        spdlog::info("texture {}, hr = 0x{:x}", (void*)tex, hr);
        std::string name = fmt::format("{}.dds", metadata->Name);
        std::string realName = "textures\\" + name.substr(name.find_last_of("\\") + 1);
        spdlog::info("out file = {}", realName);
        hr = DirectX::SaveDDSTextureToFile(devCon.Get(), tex, realName.c_str());
        spdlog::info("save hr = 0x{:x}", hr);
    }

    void PrintAssets()
    {
        for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
        {
            AssetDefinition& def = m_assetDefinitions[i];
            if (def.Type == 0x72747874) // txtr
            {
                TextureMetadata* metadata = reinterpret_cast<TextureMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                char* textureData = reinterpret_cast<char*>(m_sectionPointers[def.DataRef.Section] + def.DataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Name: {}, Data: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->Name, (void*)textureData);
                SaveTexture(metadata, textureData);
            }
            /*else if (def.Type == 0x73646873) // shds
            {
                BaseAssetMetadata* metadata = reinterpret_cast<BaseAssetMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Name: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->Name);
            }
            else if (def.Type == 0x6C74616D) // matl
            {
                MaterialGlue* metadata = reinterpret_cast<MaterialGlue*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Name: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->Name);
            }
            else if (def.Type == 0x72646873) // shdr
            {
                ShdrMetadata* metadata = reinterpret_cast<ShdrMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Name: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->Name);
            }
            else
            {
                BaseAssetMetadata* metadata = reinterpret_cast<BaseAssetMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Metadata: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, (void*)metadata);
            }*/
        }
    }

    void PrintDatatables()
    {
        for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
        {
            AssetDefinition& def = m_assetDefinitions[i];
            if (def.Type != 0x6C627464)
            {
                continue;
            }

            DatatableMetadata* metadata = reinterpret_cast<DatatableMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
            spdlog::debug("{}: ID: 0x{:x}, Metadata: {}, Rows: {}, Columns: {}, Row Size: 0x{:x}", i, def.ID, (void*)metadata, metadata->RowCount, metadata->ColumnCount, metadata->RowSize);
            for (uint32_t j = 0; j < metadata->ColumnCount; j++)
            {
                spdlog::debug("    Column {}: {}, Type: {}, Offset: 0x{:x}", j, metadata->Columns[j].Name, DATATABLE_TYPES[metadata->Columns[j].Type], metadata->Columns[j].Offset);
            }

            DumpDatatable(metadata, fmt::format("E:\\temp\\dumped_paks\\dt\\{:x}.csv", def.ID));
        }
    }

    void DumpDatatable(DatatableMetadata* dt, const std::string& filename)
    {
        std::ofstream output;
        output.open(filename);
        for (uint32_t col = 0; col < dt->ColumnCount; col++)
        {
            std::string name = dt->Columns[col].Name;
            ReplaceAll(name, "\"", "\"\"");
            output << "\"" << name << "\"";
            if (col != (dt->ColumnCount - 1))
            {
                output << ",";
            }
            else
            {
                output << std::endl;
            }
        }

        for (uint32_t row = 0; row < dt->RowCount; row++)
        {
            for (uint32_t col = 0; col < dt->ColumnCount; col++)
            {
                std::string val;
                DatatableColumn& dtCol = dt->Columns[col];
                void* data = dt->RowData + (dt->RowSize * row) + dtCol.Offset;
                if (dtCol.Type == 0)
                {
                    val = fmt::format("{}", *static_cast<bool*>(data));
                }
                else if (dtCol.Type == 1)
                {
                    val = fmt::format("{}", *static_cast<int*>(data));
                }
                else if (dtCol.Type == 2)
                {
                    val = fmt::format("{}", *static_cast<float*>(data));
                }
                else if (dtCol.Type == 3)
                {
                    val = fmt::format("VECTOR");
                }
                else if (dtCol.Type == 4)
                {
                    val = *static_cast<char**>(data);
                    ReplaceAll(val, "\"", "\"\"");
                }
                else if (dtCol.Type == 5)
                {
                    val = fmt::format("ASSET");
                }
                else if (dtCol.Type == 6)
                {
                    val = *static_cast<char**>(data);
                    ReplaceAll(val, "\"", "\"\"");
                }
                else
                {
                    val = fmt::format("UNKNOWN");
                }

                output << "\"" << val << "\"";

                if (col != (dt->ColumnCount - 1))
                {
                    output << ",";
                }
                else
                {
                    output << std::endl;
                }
            }
        }
    }

    ~PakFile()
    {
        for (uint32_t i = 0; i < kNumSlots; i++)
        {
            _aligned_free(m_slotData[i]);
            m_slotData[i] = nullptr;
        }
    }

private:
    std::unique_ptr<IFileReader> m_reader;
    std::string m_name;
    OuterHeader m_outerHeader;
    uint32_t m_unknownBlockSize;
    uint32_t m_beforeStarpakSecond;
    std::unique_ptr<Header62Elem[]> m_header62Elems;
    std::vector<std::string> m_starpakPaths;
    std::unique_ptr<SlotDescriptor[]> m_slotDescriptors;
    std::unique_ptr<SectionDescriptor[]> m_sectionDescriptors;
    char* m_slotData[kNumSlots] = {}; // TODO: Make this a unique_ptr. No idea why it's so hard to do it.
    std::unique_ptr<char*[]> m_sectionPointers; // TODO: This is not really safe because it contains pointers into the allocated memory in m_slotData
    std::unique_ptr<SectionReference[]> m_relocationDescriptors;
    std::unique_ptr<AssetDefinition[]> m_assetDefinitions;
    std::unique_ptr<char[]> m_extraHeader;
    std::unique_ptr<char[]> m_unknownBlock;
};

int main()
{
    spdlog::set_level(spdlog::level::debug);

    rtech::Initialize("C:\\Games\\Origin\\Titanfall2\\bin\\x64_retail");

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &dev,
        nullptr,
        &devCon);

    if (FAILED(hr))
    {
        spdlog::critical("Failed to initialize D3D11 device");
        return 1;
    }

    spdlog::debug("hash = 0x{:x}", rtech::HashData("datatable/bt_player_conversations.rpak"));

    //const char* name = "E:\\temp\\dumped_paks\\common_sp.rpak21";
    //const char* name = "E:\\temp\\dumped_paks\\sp_training.rpak43";
    //const char* name = "E:\\temp\\dumped_paks\\sp_training_loadscreen.rpak13";
    const char* name = "E:\\temp\\dumped_paks\\ui_mp.rpak";
    PakFile pak("common_sp.rpak", std::make_unique<PreprocessedFileReader>(name));
    pak.Initialize();
    pak.LoadAllSections();
    pak.ApplyRelocations();
    pak.PrintAssets();
    //system("pause");
    //pak.PrintDatatables();
}
