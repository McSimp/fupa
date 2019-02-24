#include "pch.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

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
    uint64_t SizeOnDisk;
};

static_assert(sizeof(SlotDescriptor) == 16, "SlotDescriptor must be 16 bytes");

#pragma pack(pop)



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
        spdlog::debug("====== Starpak Paths ======");
        spdlog::debug("Size: 0x{:x}", m_outerHeader.StarpakPathBlockSize);
        if (m_outerHeader.StarpakPathBlockSize > 0)
        {
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
        }

        // Read slot descriptors and allocate memory
        spdlog::debug("====== Slot Descriptors ======");
        spdlog::debug("Size: 0x{:x}", sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);
        if (m_outerHeader.NumSlotDescriptors > 0)
        {
            m_slotDescriptors = std::make_unique<SlotDescriptor[]>(m_outerHeader.NumSlotDescriptors);
            m_reader->ReadData(reinterpret_cast<char*>(m_slotDescriptors.get()), sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);
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
};

int main()
{
    spdlog::set_level(spdlog::level::debug);
    const char* name = "E:\\temp\\dumped_paks\\common_sp.rpak20";
    PakFile pak("common_sp.rpak", std::make_unique<PreprocessedFileReader>(name));
    pak.Initialize();
}
