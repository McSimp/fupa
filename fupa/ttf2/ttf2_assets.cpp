#include "pch.h"

#ifdef TTF2

struct ShaderMetadata
{
    char* Name;
};

class ShaderAsset : public BaseAsset<ShaderAsset, ShaderMetadata>
{
public:
    using BaseAsset<ShaderAsset, ShaderMetadata>::BaseAsset;

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }
};

struct MaterialGlue
{
    char Unknown[24];
    char* Name;
};

class MaterialAsset : public BaseAsset<MaterialAsset, MaterialGlue>
{
public:
    using BaseAsset<MaterialAsset, MaterialGlue>::BaseAsset;

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }
};

struct UnknownAssetMetadata
{

};

class UnknownAsset : public BaseAsset<UnknownAsset, UnknownAssetMetadata>
{
public:
    using BaseAsset<UnknownAsset, UnknownAssetMetadata>::BaseAsset;
};

void RegisterAssetTypes()
{
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);

    AssetFactory::Register(kShaderSetAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kLCDScreenEffectAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kParticleScriptType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kAnimationRecordingType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIFontAtlasType, &UnknownAsset::CreateMethod);
}

#endif
