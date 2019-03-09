#pragma once

class AssetFactory
{
public:
    using TCreateMethod = std::unique_ptr<IAsset>(*)(const AssetDefinition*, const uint8_t*, const uint8_t*);

    AssetFactory() = delete;
    static void Register(uint32_t type, TCreateMethod create);
    static std::unique_ptr<IAsset> Create(const AssetDefinition* asset, const uint8_t* metadata, const uint8_t* data);

private:
    static std::map<uint32_t, TCreateMethod> Methods;
};
