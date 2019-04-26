#pragma once

class StarpakReader
{
public:
    void AddStarpakFile(const std::filesystem::path& basePath, const std::string& name);
    std::vector<uint8_t> ReadStarpakData(uint32_t index, size_t offset);
#ifdef APEX
    void AddFullStarpakFile(const std::filesystem::path& basePath, const std::string& name);
    std::vector<uint8_t> ReadFullStarpakData(uint32_t index, size_t offset);
#endif

private:
    void AddStarpakInternal(const std::filesystem::path& basePath, const std::string& name, std::vector<std::ifstream>& starpakFiles, std::vector<std::unordered_map<size_t, size_t>>& starpakOffsetMaps);
    std::vector<uint8_t> ReadStarpakInternal(uint32_t index, size_t offset, std::vector<std::ifstream>& starpakFiles, std::vector<std::unordered_map<size_t, size_t>>& starpakOffsetMaps);

    std::vector<std::ifstream> m_starpakFiles;
    std::vector<std::unordered_map<size_t, size_t>> m_starpakOffsetMaps;
#ifdef APEX
    std::vector<std::ifstream> m_fullStarpakFiles;
    std::vector<std::unordered_map<size_t, size_t>> m_fullStarpakOffsetMaps;
#endif
};
