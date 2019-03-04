#include "pch.h"

#pragma comment (lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device> dev;
ComPtr<ID3D11DeviceContext> devCon;
std::ofstream DebugFile("E:\\temp\\dumped_paks\\debug.dat", std::ios::out | std::ios::binary);

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

struct PatchMetadata
{
    uint32_t Unknown1;
    uint32_t NumFiles;
    char** PakNames;
    uint8_t* PakNumbers;
};

#pragma pack(pop)

// Taken from https://stackoverflow.com/a/18374698
std::wstring Widen(const std::string& input)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.from_bytes(input);
}

class IFileReader
{
public:
    virtual void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes = 0) = 0;
};
/*
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
        spdlog::debug("Offset after reading 0x{:x} bytes (skipping 0x{:x}): 0x{:x}", bytesToRead, skipBytes, m_file.tellg());
        if (m_file.fail())
        {
            throw std::runtime_error("File read failed");
        }
    }

private:
    std::ifstream m_file;
};
*/

class ChainedReader : public IFileReader
{
public:
    ChainedReader(const std::string& filename)
    {
        PushFile(filename, false);
        m_currentFile = 0;
    }

    void PushFile(const std::string& filename, bool skipHeader)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error(fmt::format("Failed to open file {}", filename));
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        if (skipHeader)
        {
            file.seekg(sizeof(OuterHeader), std::ios::beg);
            size -= sizeof(OuterHeader);
        }
        else
        {
            file.seekg(0, std::ios::beg);
        }
        m_files.emplace_back(std::move(file), size);
        spdlog::debug("Pushed file {} with size 0x{:x}", filename, size);
    }

    void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes = 0) override
    {
        size_t bytesSkipped = 0;
        while (bytesSkipped != skipBytes)
        {
            FileDescriptor& f = m_files[m_currentFile];
            if (f.BytesRemaining == 0)
            {
                if ((m_currentFile + 1) >= m_files.size())
                {
                    throw std::runtime_error("Attempted to seek beyond end of chained files");
                }

                m_currentFile++;
            }
            size_t skipFromCurrent = std::min(f.BytesRemaining, skipBytes - bytesSkipped);
            f.File.seekg(skipFromCurrent, std::ios::cur);
            f.BytesRemaining -= skipFromCurrent;
            bytesSkipped += skipFromCurrent;
        }

        size_t bytesRead = 0;
        while (bytesRead != bytesToRead)
        {
            FileDescriptor& f = m_files[m_currentFile];
            if (f.BytesRemaining == 0)
            {
                if ((m_currentFile + 1) >= m_files.size())
                {
                    throw std::runtime_error("Attempted to read beyond end of chained files");
                }

                m_currentFile++;
            }
            size_t readFromCurrent = std::min(f.BytesRemaining, bytesToRead - bytesRead);
            f.File.read(buffer + bytesRead, readFromCurrent);
            DebugFile.write(buffer + bytesRead, readFromCurrent);
            spdlog::debug("Read 0x{:x} bytes from file {}, now at 0x{:x}", readFromCurrent, m_currentFile, f.File.tellg());
            if (f.File.fail())
            {
                throw std::runtime_error("File read failed");
            }
            f.BytesRemaining -= readFromCurrent;
            bytesRead += readFromCurrent;
        }
    }

private:
    struct FileDescriptor
    {
        std::ifstream File;
        size_t BytesRemaining;

        FileDescriptor(std::ifstream file, size_t bytesRemaining) :
            File(std::move(file)),
            BytesRemaining(bytesRemaining)
        {

        }
    };

    std::vector<FileDescriptor> m_files;
    size_t m_currentFile;
};

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
const char kPatchArray1Values[] = { 0, 1, 2, 3, 4, 5, 6 };
const char kPatchArray2Values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

std::string GetRpakPath(const std::string& baseFolder, const std::string& name, int pakNumber)
{
    if (pakNumber == 0)
    {
        return baseFolder + "/" + name + ".rpak";
    }
    else
    {
        return fmt::format("{}/{}({:02d}).rpak", baseFolder, name, pakNumber);
    }
}

class PakFile
{
public:
    PakFile(std::string baseFolder, std::string name, int pakNumber) :
        m_reader(std::make_unique<ChainedReader>(GetRpakPath(baseFolder, name, pakNumber))),
        m_name(std::move(name)),
        m_baseFolder(std::move(baseFolder))
    {
        
    }

    // PATCH FUNCTIONS

    size_t PatchFuncRead(char* buffer, size_t bytesToRead)
    {
        size_t read = std::min(bytesToRead, m_bytesUntilNextPatch);
        spdlog::debug("Reading 0x{:x} bytes (wanted to read 0x{:x}) - m_bytesUntilNextPatch = 0x{:x}", read, bytesToRead, m_bytesUntilNextPatch);
        m_reader->ReadData(buffer, read);
        m_bytesUntilNextPatch -= read;
        return read;
    }

    size_t PatchFuncSkip(char* buffer, size_t bytesToRead)
    {
        // Advance the internal pointer by as much as possible, but don't read anything
        size_t skip = m_bytesUntilNextPatch;
        spdlog::debug("Skipping 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", skip, m_bytesUntilNextPatch);
        m_reader->ReadData(buffer, 0, skip);
        m_bytesUntilNextPatch -= skip;
        return 0;
    }

    size_t PatchFuncInsert(char* buffer, size_t bytesToRead)
    {
        // For an insert, the input data stream doesn't progress - only the output
        size_t insert = std::min(bytesToRead, m_bytesUntilNextPatch);
        spdlog::debug("Inserting 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", insert, m_bytesUntilNextPatch);
        memcpy(buffer, m_currentPatchData, insert);
        DebugFile.write((char*)m_currentPatchData, insert);
        m_currentPatchData += insert;
        m_bytesUntilNextPatch -= insert;
        return insert;
    }

    size_t PatchFuncReplace(char* buffer, size_t bytesToRead)
    {
        // For an insert, the input data also progresses
        size_t replace = std::min(bytesToRead, m_bytesUntilNextPatch);
        spdlog::debug("Replacing 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", replace, m_bytesUntilNextPatch);
        memcpy(buffer, m_currentPatchData, replace);
        m_reader->ReadData(buffer, 0, replace);
        DebugFile.write((char*)m_currentPatchData, replace);
        m_currentPatchData += replace;
        m_bytesUntilNextPatch -= replace;
        return replace;
    }

    // END PATCH FUNCTIONS

    void UpdatePatchInstruction()
    {
        m_patch696 |= *m_patch680 << (64 - static_cast<uint8_t>(m_patch704));
        m_patch680 = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_patch680) + (m_patch704 >> 3));
        m_patch704 = m_patch704 & 7;
        
        int64_t index = m_patch696 & 0x3F;
        uint8_t opcode = m_patchData1[index];
        uint8_t patchData2Val = m_patchData2[index];

        m_patch696 >>= patchData2Val;
        m_patch704 += patchData2Val;

        if (opcode > 3)
        {
            // This probably should not happen
            spdlog::error("This shouldn't happen");
            DebugBreak();
        }
        else
        {
            uint8_t patchData3Val = m_patchData3[static_cast<uint8_t>(m_patch696)];
            uint8_t patchData4Val = m_patchData4[static_cast<uint8_t>(m_patch696)];
            uint64_t new696Val = m_patch696 >> patchData4Val;
            m_patch696 = new696Val >> patchData3Val;
            m_bytesUntilNextPatch = (1ULL << patchData3Val) + (new696Val & ((1ULL << patchData3Val) - 1));
            m_patch704 += patchData3Val + patchData4Val;
            if (opcode == 0)
            {
                m_patchInstruction = &PakFile::PatchFuncRead;
            }
            else if (opcode == 1)
            {
                m_patchInstruction = &PakFile::PatchFuncSkip;
            }
            else if (opcode == 2)
            {
                m_patchInstruction = &PakFile::PatchFuncInsert;
            }
            else if (opcode == 3)
            {
                m_patchInstruction = &PakFile::PatchFuncReplace;
            }
            else
            {
                throw std::runtime_error("Unsupported patch opcode");
            }
        }
    }

    void ReadPatchedData(char* buffer, size_t bytesToRead)
    {
        size_t bytesRead = 0;
        while (bytesRead != bytesToRead)
        {
            if (m_bytesUntilNextPatch > 0)
            {
                size_t read = (*this.*m_patchInstruction)(buffer + bytesRead, bytesToRead - bytesRead);
                bytesRead += read;
            }
            else
            {
                UpdatePatchInstruction();
            }
        }
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
        spdlog::debug("NumRPakLinks: {}", m_outerHeader.NumRPakLinks);
        spdlog::debug("NumRelocations: {}", m_outerHeader.NumRelocations);
        spdlog::debug("NumAssets: {}", m_outerHeader.NumAssets);
        spdlog::debug("Header72: {}", m_outerHeader.Header72);
        spdlog::debug("Header76: {}", m_outerHeader.Header76);
        spdlog::debug("Header80: {}", m_outerHeader.Header80);
        spdlog::debug("Header84: {}", m_outerHeader.Header84);

        m_bytesUntilNextPatch = m_outerHeader.DecompressedSize - 0x58 + (m_outerHeader.NumRPakLinks != 0 ? 0 : 1);
        m_patchInstruction = &PakFile::PatchFuncRead;

        // Read data on links to other RPaks
        if (m_outerHeader.NumRPakLinks != 0)
        {
            ReadPatchedData(reinterpret_cast<char*>(&m_patchDataBlockSize), sizeof(m_patchDataBlockSize));
            ReadPatchedData(reinterpret_cast<char*>(&m_beforeStarpakSecond), sizeof(m_beforeStarpakSecond));

            m_linkedRPakSizes = std::make_unique<LinkedRPakSize[]>(m_outerHeader.NumRPakLinks);
            ReadPatchedData(reinterpret_cast<char*>(m_linkedRPakSizes.get()), sizeof(LinkedRPakSize) * m_outerHeader.NumRPakLinks);

            m_linkedRPakNumbers = std::make_unique<uint16_t[]>(m_outerHeader.NumRPakLinks);
            ReadPatchedData(reinterpret_cast<char*>(m_linkedRPakNumbers.get()), sizeof(uint16_t) * m_outerHeader.NumRPakLinks);

            spdlog::debug("====== Patch Information ======");
            spdlog::debug("Patch Block Size: 0x{:x}", m_patchDataBlockSize);
            spdlog::debug("Second: 0x{:x}", m_beforeStarpakSecond);

            spdlog::debug("====== RPak Links ======");
            for (uint16_t i = 0; i < m_outerHeader.NumRPakLinks; i++)
            {
                m_reader->PushFile(GetRpakPath(m_baseFolder, m_name, m_linkedRPakNumbers[i]), true);
                spdlog::debug("{}: Size: 0x{:x}, Decompressed Size: 0x{:x}, Number: {}", i, m_linkedRPakSizes[i].SizeOnDisk, m_linkedRPakSizes[i].DecompressedSize, m_linkedRPakNumbers[i]);
            }
        }

        // Read starpak paths
        if (m_outerHeader.StarpakPathBlockSize != 0)
        {
            spdlog::debug("====== Starpak Paths ======");
            spdlog::debug("Size: 0x{:x}", m_outerHeader.StarpakPathBlockSize);

            std::unique_ptr<char[]> starpakBlock = std::make_unique<char[]>(m_outerHeader.StarpakPathBlockSize);
            ReadPatchedData(starpakBlock.get(), m_outerHeader.StarpakPathBlockSize);

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
        }

        if (m_outerHeader.NumSlotDescriptors == 0)
        {
            throw std::runtime_error("NumSlotDescriptors was 0");
        }

        // Read slot descriptors and allocate memory
        spdlog::debug("====== Slot Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);
        
        m_slotDescriptors = std::make_unique<SlotDescriptor[]>(m_outerHeader.NumSlotDescriptors);
        ReadPatchedData(reinterpret_cast<char*>(m_slotDescriptors.get()), sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);

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
        ReadPatchedData(reinterpret_cast<char*>(m_sectionDescriptors.get()), sizeof(SectionDescriptor) * m_outerHeader.NumSections);

        m_sectionPointers = std::make_unique<char*[]>(m_outerHeader.NumSections);

        for (uint16_t i = 0; i < m_outerHeader.NumSections; i++)
        {
            SectionDescriptor& sectDesc = m_sectionDescriptors[i];
            uint64_t offset = (slotDescOffsets[sectDesc.SlotDescIndex] + sectDesc.Alignment - 1) & ~static_cast<uint64_t>(sectDesc.Alignment - 1);
            m_sectionPointers[i] = m_slotData[m_slotDescriptors[sectDesc.SlotDescIndex].Slot & 3] + offset;
            slotDescOffsets[sectDesc.SlotDescIndex] = offset + sectDesc.Size;
            spdlog::debug("{}: SlotDescIdx: {}, Alignment: 0x{:x}, Size: 0x{:x}, Offset: 0x{:x}, Data: {}", i, sectDesc.SlotDescIndex, sectDesc.Alignment, sectDesc.Size, offset, static_cast<void*>(m_sectionPointers[i]));
        }

        // Read relocation information
        spdlog::debug("====== Relocation Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SectionReference) * m_outerHeader.NumRelocations);

        m_relocationDescriptors = std::make_unique<SectionReference[]>(m_outerHeader.NumRelocations);
        ReadPatchedData(reinterpret_cast<char*>(m_relocationDescriptors.get()), sizeof(SectionReference) * m_outerHeader.NumRelocations);

        for (uint32_t i = 0; i < m_outerHeader.NumRelocations; i++)
        {
            SectionReference& reloc = m_relocationDescriptors[i];
            spdlog::trace("{}: Section: {}, Offset: 0x{:x}", i, reloc.Section, reloc.Offset);
        }

        // Read asset definitions
        spdlog::debug("====== Asset Definitions ======");
        spdlog::debug("Size: 0x{:x}", sizeof(AssetDefinition) * m_outerHeader.NumAssets);

        m_assetDefinitions = std::make_unique<AssetDefinition[]>(m_outerHeader.NumAssets);
        ReadPatchedData(reinterpret_cast<char*>(m_assetDefinitions.get()), sizeof(AssetDefinition) * m_outerHeader.NumAssets);
        for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
        {
            char* assetType = reinterpret_cast<char*>(&m_assetDefinitions[i].Type);
            spdlog::debug("{}: {:.4s}", i, assetType);
        }

        // Read extra header
        spdlog::debug("====== Extra Header ======");
        size_t extraHeaderSize = (m_outerHeader.Header72 * 8) + (m_outerHeader.Header76 * 4) + (m_outerHeader.Header80 * 4) + m_outerHeader.Header84;
        spdlog::debug("Size: 0x{:x}", extraHeaderSize);

        m_extraHeader = std::make_unique<char[]>(extraHeaderSize);
        ReadPatchedData(reinterpret_cast<char*>(m_extraHeader.get()), extraHeaderSize);

        // Read patch data block
        if (m_outerHeader.NumRPakLinks != 0)
        {
            spdlog::debug("====== Patch Data Block ======");
            spdlog::debug("Size: 0x{:x}", m_patchDataBlockSize);

            m_patchDataBlock = std::make_unique<uint8_t[]>(m_patchDataBlockSize);
            ReadPatchedData(reinterpret_cast<char*>(m_patchDataBlock.get()), m_patchDataBlockSize);

            // Construct patch data arrays
            uint8_t* nextBlock = m_patchDataBlock.get() + rtech::ConstructPatchArray(m_patchDataBlock.get(), 6, kPatchArray1Values, m_patchData1, m_patchData2);
            nextBlock += rtech::ConstructPatchArray(nextBlock, 8, kPatchArray2Values, m_patchData3, m_patchData4);
            uint64_t val = *reinterpret_cast<uint64_t*>(nextBlock);

            // Set the initial patch variables
            m_patch704 = 24;
            m_patch696 = val >> 24;
            m_patch680 = reinterpret_cast<uint64_t*>(nextBlock) + 1;
            m_currentPatchData = nextBlock + (val & 0xFFFFFF);
        }
    }

    int32_t NormalizeSection(uint32_t section)
    {
        int32_t val = section + m_beforeStarpakSecond;
        if (val >= m_outerHeader.NumSections)
        {
            val -= m_outerHeader.NumSections;
        }
        return val;
    }

    void LoadAllSections()
    {
        // Sectors are stored sequentially after the header information, so just read them in order
        for (uint32_t i = 0; i < m_outerHeader.NumSections; i++)
        {
            int32_t section = NormalizeSection(i);
            if (m_sectionDescriptors[section].Size > 0)
            {
                spdlog::debug("Reading section {} (0x{:x} bytes)", section, m_sectionDescriptors[section].Size);
                ReadPatchedData(m_sectionPointers[section], m_sectionDescriptors[section].Size);
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

            nextTextureData += (subResources[mip].SysMemSlicePitch + MIP_ALIGNMENT - 1) & ~(MIP_ALIGNMENT - 1);
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
        std::string name = fmt::format("textures\\{}.dds", metadata->Name);
        std::filesystem::path p = name;
        p.remove_filename();
        std::filesystem::create_directories(p);
        spdlog::info("out file = {}", name);
        hr = DirectX::SaveDDSTextureToFile(devCon.Get(), tex, Widen(name).c_str());
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to save texture");
        }
        spdlog::info("save hr = 0x{:x}", hr);
    }

    void PrintPatch(PatchMetadata* metadata)
    {
        for (uint32_t i = 0; i < metadata->NumFiles; i++)
        {
            spdlog::debug("{}: Name: {}, Num: {}", i, metadata->PakNames[i], metadata->PakNumbers[i]);
        }
    }

    void PrintAssets()
    {
        uint32_t numTexts = 0;
        for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
        {
            AssetDefinition& def = m_assetDefinitions[i];
            if (def.Type == 0x72747874) // txtr
            {
                TextureMetadata* metadata = reinterpret_cast<TextureMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                char* textureData = reinterpret_cast<char*>(m_sectionPointers[def.DataRef.Section] + def.DataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Name: {}, Data: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->Name, (void*)textureData);
                SaveTexture(metadata, textureData);
                numTexts++;
            }
            else if (def.Type == 0x73646873) // shds
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
            else if (def.Type == 0x68637450) // ptch
            {
                PatchMetadata* metadata = reinterpret_cast<PatchMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Num Paks: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, metadata->NumFiles);
                PrintPatch(metadata);
            }
            else
            {
                BaseAssetMetadata* metadata = reinterpret_cast<BaseAssetMetadata*>(m_sectionPointers[def.MetadataRef.Section] + def.MetadataRef.Offset);
                spdlog::debug("{}: ID: 0x{:x}, Type: {:.4s}, Size: 0x{:x}, Metadata: {}", i, def.ID, reinterpret_cast<char*>(&def.Type), def.DataSize, (void*)metadata);
            }
        }
        spdlog::info("Num textures: {}", numTexts);
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
    std::unique_ptr<ChainedReader> m_reader;
    std::string m_name;
    std::string m_baseFolder;
    OuterHeader m_outerHeader;
    uint32_t m_patchDataBlockSize;
    uint32_t m_beforeStarpakSecond;
    std::unique_ptr<LinkedRPakSize[]> m_linkedRPakSizes;
    std::unique_ptr<uint16_t[]> m_linkedRPakNumbers;
    std::vector<std::string> m_starpakPaths;
    std::unique_ptr<SlotDescriptor[]> m_slotDescriptors;
    std::unique_ptr<SectionDescriptor[]> m_sectionDescriptors;
    char* m_slotData[kNumSlots] = {}; // TODO: Make this a unique_ptr. No idea why it's so hard to do it.
    std::unique_ptr<char*[]> m_sectionPointers; // TODO: This is not really safe because it contains pointers into the allocated memory in m_slotData
    std::unique_ptr<SectionReference[]> m_relocationDescriptors;
    std::unique_ptr<AssetDefinition[]> m_assetDefinitions;
    std::unique_ptr<char[]> m_extraHeader;

    std::unique_ptr<uint8_t[]> m_patchDataBlock;
    uint8_t m_patchData1[64];
    uint8_t m_patchData2[64];
    uint8_t m_patchData3[256];
    uint8_t m_patchData4[256];

    uint64_t* m_patch680;
    uint64_t m_patch696;
    uint32_t m_patch704;
    uint64_t m_bytesUntilNextPatch;
    uint8_t* m_currentPatchData;
    size_t (PakFile::*m_patchInstruction)(char* buffer, size_t bytesToRead);
};

//const size_t COMPRESSED_BUFFER_SIZE = 0x400000;
//const size_t DECOMPRESSED_BUFFER_SIZE = 0x1000000;
const size_t COMPRESSED_BUFFER_SIZE = 0x1000000;
const size_t DECOMPRESSED_BUFFER_SIZE = 0x400000;
const size_t CHUNK_SIZE = 512 * 1024;

void DoDecompress(std::string name)
{
    // DECOMPRESSION STUFF
    char* allData = (char*)_aligned_malloc(COMPRESSED_BUFFER_SIZE + DECOMPRESSED_BUFFER_SIZE, 0x1000);
    memset(allData, 0, COMPRESSED_BUFFER_SIZE + DECOMPRESSED_BUFFER_SIZE);
    char* compressedData = allData;
    char* decompressedData = allData + COMPRESSED_BUFFER_SIZE;

    std::ifstream input;
    input.open(fmt::format("C:\\Games\\Origin\\Titanfall2\\r2\\paks\\Win64\\{}", name), std::ios::in | std::ios::binary);
    //input.open("E:\\Games\\Origin\\Apex\\paks\\Win64\\common_mp(02).rpak", std::ios::in | std::ios::binary);

    std::ofstream output;
    output.open(fmt::format("E:\\temp\\dumped_paks\\decompressed\\{}", name), std::ios::out | std::ios::binary);
    //output.open("E:\\temp\\dumped_paks\\decompressed\\apex\\common_mp(02).rpak", std::ios::out | std::ios::binary);

    OuterHeader header;
    input.read(reinterpret_cast<char*>(&header), sizeof(OuterHeader));
    input.seekg(0, std::ios::beg);

    // Setup state
    uint64_t decompressionState[17];
    size_t dataRead = 0;
    size_t totalDecompressed = sizeof(OuterHeader);
    size_t dataToRead = std::min(COMPRESSED_BUFFER_SIZE, header.CompressedSize - dataRead);
    input.read(compressedData, dataToRead);
    dataRead += dataToRead;
    uint64_t expectedDecompressedSize = rtech::SetupDecompressState(decompressionState, compressedData, 0xFFFFFF, header.CompressedSize, 0, sizeof(OuterHeader));
    if (expectedDecompressedSize != header.DecompressedSize)
    {
        spdlog::error("Compressed size in header (0x{:x}) does not match compressed size from data (0x{:x})", header.DecompressedSize, expectedDecompressedSize);
        return;
    }
    decompressionState[1] = (uint64_t)decompressedData;
    decompressionState[3] = DECOMPRESSED_BUFFER_SIZE - 1;
    memcpy(decompressedData, reinterpret_cast<char*>(&header), sizeof(OuterHeader));

    uint64_t bufIndex = 0;
    while (totalDecompressed != header.DecompressedSize)
    {
        // Decompress data
        /*
        {
            std::ofstream out1(fmt::format("E:\\temp\\dumped_paks\\buffers_mine\\{}.dat", bufIndex++), std::ios::out | std::ios::binary);
            out1.write((const char*)decompressionState[0], 0x1400000);
        }
        */

        spdlog::info("BEFORE context={}, unk1={:x}, unk2={:x}, inputBuf={:x}, a1[1]={:x}, a1[2]={:x}, a1[3]={:x}, a1[5]={:x}, a1[6]={:x}, a1[7]={:x}, a1[9]={:x}, a1[10]={:x}, a1[11]={:x}, a1[12]={:x}, a1[14]={:x}, a1[15]={:x}, a1[16]={:x}, FirstDword={:x}, SecondDword={:x}, ThirdDword={:x}",
            (void*)decompressionState,
            dataRead, totalDecompressed + 0x400000,
            decompressionState[0],
            decompressionState[1],
            decompressionState[2],
            decompressionState[3],
            decompressionState[5],
            decompressionState[6],
            decompressionState[7],
            decompressionState[9],
            decompressionState[10],
            decompressionState[11],
            decompressionState[12],
            decompressionState[14],
            decompressionState[15],
            decompressionState[16],
            *rtech::FirstDword,
            *rtech::SecondDword,
            *rtech::ThirdDword
        );
        uint64_t totalDecompressedBefore = decompressionState[10] == sizeof(OuterHeader) ? 0 : decompressionState[10];
        //uint64_t inputProcessedBefore = decompressionState[9] == (sizeof(OuterHeader) + 0x10) ? 0 : decompressionState[9];
        uint64_t inputChunksProcessedBefore = decompressionState[9] / CHUNK_SIZE;
        rtech::DoDecompress(decompressionState, dataRead, totalDecompressed + 0x400000);
        uint64_t totalDecompressedAfter = decompressionState[10];
        uint64_t inputChunksProcessedAfter = decompressionState[9] / CHUNK_SIZE;
        spdlog::info("AFTER  context={}, unk1={:x}, unk2={:x}, inputBuf={:x}, a1[1]={:x}, a1[2]={:x}, a1[3]={:x}, a1[5]={:x}, a1[6]={:x}, a1[7]={:x}, a1[9]={:x}, a1[10]={:x}, a1[11]={:x}, a1[12]={:x}, a1[14]={:x}, a1[15]={:x}, a1[16]={:x}, FirstDword={:x}, SecondDword={:x}, ThirdDword={:x}",
            (void*)decompressionState,
            dataRead, totalDecompressed + 0x400000,
            decompressionState[0],
            decompressionState[1],
            decompressionState[2],
            decompressionState[3],
            decompressionState[5],
            decompressionState[6],
            decompressionState[7],
            decompressionState[9],
            decompressionState[10],
            decompressionState[11],
            decompressionState[12],
            decompressionState[14],
            decompressionState[15],
            decompressionState[16],
            *rtech::FirstDword,
            *rtech::SecondDword,
            *rtech::ThirdDword
        );

        /*
        {
            std::ofstream out1(fmt::format("E:\\temp\\dumped_paks\\buffers_mine\\{}.dat", bufIndex++), std::ios::out | std::ios::binary);
            out1.write((const char*)decompressionState[0], 0x1400000);
        }
        */

        uint64_t chunksProcessed = inputChunksProcessedAfter - inputChunksProcessedBefore;
        spdlog::info("Input chunks processed = 0x{:x}", chunksProcessed);
        for (uint64_t i = 0; i < chunksProcessed; i++)
        {
            dataToRead = std::min(CHUNK_SIZE, header.CompressedSize - dataRead);
            input.read(compressedData + (dataRead % COMPRESSED_BUFFER_SIZE), dataToRead);
            dataRead += dataToRead;
        }

        uint64_t decompressedThatRun = totalDecompressedAfter - totalDecompressedBefore;
        if (decompressedThatRun != DECOMPRESSED_BUFFER_SIZE)
        {
            spdlog::warn("Decompressed that run != buffer size (0{:x})", decompressedThatRun);
        }
        totalDecompressed = decompressionState[10];

        // Write to disk
        output.write(decompressedData, decompressedThatRun);
    }

    output.close();
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::trace);
    auto file_logger = spdlog::basic_logger_mt("basic_logger", "fupa.log");
    //spdlog::set_default_logger(file_logger);

    rtech::Initialize("C:\\Games\\Origin\\Titanfall2\\bin\\x64_retail");

    /*DoDecompress("sp_training.rpak");
    DoDecompress("sp_training(01).rpak");
    DoDecompress("sp_training(03).rpak");
    DoDecompress("sp_training(05).rpak");
    DoDecompress("sp_training(07).rpak");
    DoDecompress("sp_training(08).rpak");
    DoDecompress("sp_training(09).rpak");
    DoDecompress("sp_training(10).rpak");*/

    //return 0;

    
    /*uint64_t decompSizeFromFile = rtech::SetupDecompressState(decompressionState, data, 0xFFFFFF, 1895604, 0, sizeof(OuterHeader));
    
    spdlog::info(fmt::format("672 before: {}", *(uint64_t*)(decompressionState + 160)));

    rtech::DoDoDecompress(decompressionState, 1895604, 128 + 0x400000);

    spdlog::info(fmt::format("672 after: {}", *(uint64_t*)(decompressionState + 160)));

    FILE* fa = fopen("mp_lobby.dat", "wb");
    fwrite(decompData, 1, decompSize, fa);
    fclose(fa);*/

    //std::ifstream f2;
    //f2.open("E:\\temp\\dumped_paks\\sp_training_loadscreen.rpak13", std::ios::in | std::ios::binary);
    //f2.read(data, decompSize);
    //int comp = memcmp(data + 88, decompData + 88, decompSize - 88);

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
    //const char* name = "E:\\temp\\dumped_paks\\common_mp.rpak";
    //const char* name = "E:\\temp\\dumped_paks\\common_mp.rpak";
    const char* name = "E:\\temp\\dumped_paks\\sp_training.rpak43";
    //PakFile pak("E:\\temp\\dumped_paks\\decompressed", "sp_training", 11);
    PakFile pak("E:\\temp\\dumped_paks\\decompressed", "sp_training", 11);
    pak.Initialize();
    pak.LoadAllSections();
    pak.ApplyRelocations();
    pak.PrintAssets();
    
    //system("pause");
    //pak.PrintDatatables();
}
