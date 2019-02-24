#include "pch.h"

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
    uint32_t Unknown1;
    uint32_t Unknown2;
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
            spdlog::debug("{}: Section: {}, Offset: 0x{:x}", i, reloc.Section, reloc.Offset);
        }

        // Read asset definitions
        spdlog::debug("====== Asset Definitions ======");
        spdlog::debug("Size: 0x{:x}", sizeof(AssetDefinition) * m_outerHeader.NumAssets);

        m_assetDefinitions = std::make_unique<AssetDefinition[]>(m_outerHeader.NumAssets);
        m_reader->ReadData(reinterpret_cast<char*>(m_assetDefinitions.get()), sizeof(AssetDefinition) * m_outerHeader.NumAssets);

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
    //const char* name = "E:\\temp\\dumped_paks\\common_sp.rpak20";
    const char* name = "E:\\temp\\dumped_paks\\sp_training.rpak42";
    //const char* name = "E:\\temp\\dumped_paks\\sp_training_loadscreen.rpak13";
    PakFile pak("common_sp.rpak", std::make_unique<PreprocessedFileReader>(name));
    pak.Initialize();
}
