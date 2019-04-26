#include "pch.h"

void StarpakReader::AddStarpakFile(const std::filesystem::path& basePath, const std::string& name)
{
    AddStarpakInternal(basePath, name, m_starpakFiles, m_starpakOffsetMaps);
}

std::vector<uint8_t> StarpakReader::ReadStarpakData(uint32_t index, size_t offset)
{
    return ReadStarpakInternal(index, offset, m_starpakFiles, m_starpakOffsetMaps);
}

#ifdef APEX
void StarpakReader::AddFullStarpakFile(const std::filesystem::path& basePath, const std::string& name)
{
    AddStarpakInternal(basePath, name, m_fullStarpakFiles, m_fullStarpakOffsetMaps);
}

std::vector<uint8_t> StarpakReader::ReadFullStarpakData(uint32_t index, size_t offset)
{
    return ReadStarpakInternal(index, offset, m_fullStarpakFiles, m_fullStarpakOffsetMaps);
}
#endif

struct StarpakEntry
{
    uint64_t Offset;
    uint64_t Size;
};

void StarpakReader::AddStarpakInternal(
    const std::filesystem::path& basePath,
    const std::string& name,
    std::vector<std::ifstream>& starpakFiles,
    std::vector<std::unordered_map<size_t, size_t>>& starpakOffsetMaps)
{
    auto logger = spdlog::get("logger");
    std::string path = (basePath / name).string();
    logger->debug("Opening starpak: {}", path);

    // Open the file
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    // Check the magic matches
    uint32_t magic;
    f.read(reinterpret_cast<char*>(&magic), 4);
    if (magic != 0x6B505253)
    {
        throw std::runtime_error(fmt::format("{} is not a valid starpak file", name));
    }

    // Check the version matches
    uint32_t version;
    f.read(reinterpret_cast<char*>(&version), 4);
    if (version != 1)
    {
        throw std::runtime_error(fmt::format("{} is not a version 1 starpak file (version = {})", name, version));
    }

    // Read the entries into a map
    f.seekg(-8, std::ios_base::end);
    int64_t numEntries;
    f.read(reinterpret_cast<char*>(&numEntries), 8);

    std::unordered_map<size_t, size_t> entryOffsetMap(numEntries);
    f.seekg(-numEntries * sizeof(StarpakEntry) - 8, std::ios_base::end);

    for (int64_t i = 0; i < numEntries; i++)
    {
        StarpakEntry entry;
        f.read(reinterpret_cast<char*>(&entry), sizeof(StarpakEntry));
        entryOffsetMap.emplace(entry.Offset, entry.Size);
    }

    // Add the map and file into the supplied vectors
    starpakOffsetMaps.emplace_back(std::move(entryOffsetMap));
    starpakFiles.emplace_back(std::move(f));
}

std::vector<uint8_t> StarpakReader::ReadStarpakInternal(
    uint32_t index,
    size_t offset,
    std::vector<std::ifstream>& starpakFiles,
    std::vector<std::unordered_map<size_t, size_t>>& starpakOffsetMaps)
{
    // Lookup the size in the map
    if (index >= starpakOffsetMaps.size())
    {
        throw std::runtime_error("Starpak index out of bounds");
    }

    auto& offsetMap = starpakOffsetMaps[index];
    auto it = offsetMap.find(offset);
    if (it == offsetMap.end())
    {
        throw std::runtime_error(fmt::format("Offset {} not found in offset map for index {}", offset, index));
    }

    size_t entrySize = it->second;

    // Seek to the offset and read the data
    auto& f = starpakFiles[index];
    f.seekg(offset);

    std::vector<uint8_t> data(entrySize);
    f.read(reinterpret_cast<char*>(data.data()), entrySize);

    return std::move(data);
}
