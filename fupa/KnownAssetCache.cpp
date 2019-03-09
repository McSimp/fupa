#include "pch.h"

std::map<uint64_t, std::string> KnownAssetCache::NameCache;

bool KnownAssetCache::HasName(uint64_t hash)
{
    return NameCache.find(hash) != NameCache.end();
}

const std::string& KnownAssetCache::GetName(uint64_t hash)
{
    auto it = NameCache.find(hash);
    if (it == NameCache.end())
    {
        throw std::runtime_error("Failed to find hash in KnownAssetCache");
    }

    return it->second;
}

void KnownAssetCache::AddName(std::string name)
{
    uint64_t hash = rtech::HashData(name.c_str());
    NameCache.emplace(hash, std::move(name));
}

