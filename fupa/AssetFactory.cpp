#include "pch.h"

std::map<uint32_t, AssetFactory::TCreateMethod> AssetFactory::Methods;

void AssetFactory::Register(uint32_t type, TCreateMethod create)
{
    auto it = Methods.find(type);
    if (it == Methods.end())
    {
        Methods[type] = create;
    }
}

std::unique_ptr<IAsset> AssetFactory::Create(const AssetDefinition* asset, const uint8_t* metadata, const uint8_t* data)
{
    auto it = Methods.find(asset->Type);
    if (it != Methods.end())
    {
        return it->second(asset, metadata, data);
    }
    else
    {
        return nullptr;
    }
}
