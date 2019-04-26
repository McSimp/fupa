#include "pch.h"
#include "../pch.h"

#ifdef TTF2

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
    AssetFactory::Register(kShaderSetAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kLCDScreenEffectAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kParticleScriptType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kAnimationRecordingType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIFontAtlasType, &UnknownAsset::CreateMethod);
}

#endif
