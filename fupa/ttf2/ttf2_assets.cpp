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

    bool HasName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetName() override
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

    bool HasName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetName() override
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

#pragma pack(push, 1)
struct RSONDataDescriptor
{
    uint32_t Type;
    uint32_t NumEntries;
    void* Data;
};

struct RSONObjectEntry
{
    char* Key;
    RSONDataDescriptor Data;
    RSONObjectEntry* Next;
};
#pragma pack(pop)

const uint32_t kRSONStringType = 0x2;
const uint32_t kRSONObjectType = 0x8;
const uint32_t kRSONIntegerType = 0x20;
const uint32_t kRSONStringListType = 0x1002;
const uint32_t kRSONObjectListType = 0x1008;

using json = nlohmann::json;

class RSONFileAsset : public BaseAsset<RSONFileAsset, RSONDataDescriptor>
{
public:
    using BaseAsset<RSONFileAsset, RSONDataDescriptor>::BaseAsset;

    bool CanDump() override
    {
        return true;
    }

    json ParseObject(const void* data)
    {
        json object;
        const RSONObjectEntry* entry = (const RSONObjectEntry*)(data);
        do
        {
            object[entry->Key] = ParseData(&entry->Data);
            entry = entry->Next;
        } while (entry != nullptr);
        return object;
    }

    json ParseData(const RSONDataDescriptor* data)
    {
        if (data->Type == kRSONStringType)
        {
            return reinterpret_cast<char*>(data->Data);
        }
        else if (data->Type == kRSONObjectType)
        {
            return ParseObject(data->Data);
        }
        else if (data->Type == kRSONIntegerType)
        {
            return reinterpret_cast<int32_t>(data->Data);
        }
        else if (data->Type == kRSONStringListType)
        {
            json outputArray = json::array();
            const char** entries = (const char**)(data->Data);
            for (uint32_t i = 0; i < data->NumEntries; i++)
            {
                outputArray.push_back(entries[i]);
            }
            return outputArray;
        }
        else if (data->Type == kRSONObjectListType)
        {
            json outputArray = json::array();
            const void** entries = (const void**)(data->Data);
            for (uint32_t i = 0; i < data->NumEntries; i++)
            {
                outputArray.push_back(ParseObject(entries[i]));
            }
            return outputArray;
        }
        else
        {
            spdlog::get("logger")->warn("No RSON parser found for type 0x{:x}", data->Type);
            return nullptr;
        }
    }

    void Dump(const std::filesystem::path& outputDir) override
    {
        auto logger = spdlog::get("logger");

        std::filesystem::path outFilePath = outputDir / "rson" / (GetNameOrHash() + ".json");
        std::filesystem::path outFileDir = outFilePath;
        outFileDir.remove_filename();
        std::filesystem::create_directories(outFileDir);

        std::ofstream output(outFilePath);
        output << std::setw(2) << ParseData(m_metadata) << std::endl;
        logger->debug("Wrote rson file with hash {} to {}", m_asset->Hash, outFilePath.string());
    }
};

void RegisterAssetTypes()
{
    AssetFactory::Register(kRSONFileAssetType, &RSONFileAsset::CreateMethod);
    AssetFactory::Register(kTextureAssetType, &TextureAsset::CreateMethod);
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);

    AssetFactory::Register(kShaderSetAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kLCDScreenEffectAssetType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIImageAtlasType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kParticleScriptType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kAnimationRecordingType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIType, &UnknownAsset::CreateMethod);
    AssetFactory::Register(kUIFontAtlasType, &UnknownAsset::CreateMethod);
}

#endif
