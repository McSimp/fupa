#pragma once

class KnownAssetCache
{
public:
    static bool HasName(uint64_t hash);
    static const std::string& GetName(uint64_t hash);
    static void AddName(std::string name);

private:
    static std::map<uint64_t, std::string> NameCache;
};
