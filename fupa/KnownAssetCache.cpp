#include "pch.h"

std::map<uint64_t, std::string> KnownAssetCache::NameCache;
std::map<uint32_t, std::string> KnownAssetCache::HalfNameCache;

bool KnownAssetCache::HasName(uint64_t hash)
{
    return NameCache.find(hash) != NameCache.end();
}

bool KnownAssetCache::HasHalfName(uint32_t hash)
{
    return HalfNameCache.find(hash) != HalfNameCache.end();
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

const std::string& KnownAssetCache::GetHalfName(uint32_t hash)
{
    auto it = HalfNameCache.find(hash);
    if (it == HalfNameCache.end())
    {
        throw std::runtime_error("Failed to find half hash in KnownAssetCache");
    }

    return it->second;
}

void KnownAssetCache::AddName(std::string name)
{
    uint64_t hash = rtech::HashData(name.c_str());
    NameCache.emplace(hash, name);
    HalfNameCache.emplace(static_cast<uint32_t>(hash ^ (hash >> 32)), name);
}

