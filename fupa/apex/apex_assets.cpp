#include "pch.h"
#include "../pch.h"

#ifdef APEX

struct MaterialGlue
{
    uint64_t VTableSlot;
    uint64_t Unknown0;
    uint64_t Hash;
    char* Name;
    char* SurfaceProp;
    uint64_t Unknown1;
    uint64_t ShadowMaterialHash;
    uint64_t PrepassMaterialHash;
    uint64_t VSMMaterialHash;
    uint64_t TightShadowMaterialHash;
    uint64_t ColpassMaterialHash;
    uint64_t ShaderSetHash;
    uint64_t* pTextureHashes;
    uint64_t* pUnknown;
    uint16_t Unknown3;
    uint16_t Width;
    uint16_t Height;
};

class MaterialAsset : public BaseAsset<MaterialAsset, MaterialGlue>
{
public:
    using BaseAsset<MaterialAsset, MaterialGlue>::BaseAsset;
    using json = nlohmann::json;

    bool CanDump() override
    {
        return true;
    }

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }

    std::filesystem::path GetBaseOutputDirectory() override
    {
        return "materials";
    }

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    void Dump(const std::filesystem::path& outFilePath) override
    {
        auto logger = spdlog::get("logger");
        json info;
        info["name"] = m_metadata->Name;

        if (m_metadata->SurfaceProp != nullptr)
        {
            info["surface_prop"] = m_metadata->SurfaceProp;
        }
        
        info["unknown1"] = fmt::format("{:x}", m_metadata->Unknown1);

        if (m_metadata->ShadowMaterialHash != 0)
        {
            info["shadow_material"] = fmt::format("{:x}", m_metadata->ShadowMaterialHash);
        }
        
        if (m_metadata->PrepassMaterialHash != 0)
        {
            info["prepass_material"] = fmt::format("{:x}", m_metadata->PrepassMaterialHash);
        }

        if (m_metadata->VSMMaterialHash != 0)
        {
            info["vsm_material"] = fmt::format("{:x}", m_metadata->VSMMaterialHash);
        }
        
        if (m_metadata->TightShadowMaterialHash != 0)
        {
            info["tightshadow_material"] = fmt::format("{:x}", m_metadata->TightShadowMaterialHash);
        }
        
        if (m_metadata->ColpassMaterialHash != 0)
        {
            info["colpass_material"] = fmt::format("{:x}", m_metadata->ColpassMaterialHash);
        }

        if (m_metadata->ShaderSetHash != 0)
        {
            info["shader_set"] = fmt::format("{:x}", m_metadata->ShaderSetHash);
        }

        if (m_metadata->Width != 0)
        {
            info["width"] = m_metadata->Width;
        }
        
        if (m_metadata->Height != 0)
        {
            info["height"] = m_metadata->Height;
        }
        
        if (m_metadata->pTextureHashes != nullptr)
        {
            if (m_metadata->pUnknown == nullptr)
            {
                throw std::runtime_error(fmt::format("pUnknown is null when pTextureHashes is not for {}", m_metadata->Name));
            }

            json textures = json::array();
            uint64_t* pCurrentHash = m_metadata->pTextureHashes;
            while (pCurrentHash != m_metadata->pUnknown)
            {
                textures.push_back(fmt::format("{:x}", *pCurrentHash));
                pCurrentHash++;
            }
            
            info["textures"] = textures;
        }

        if (m_metadata->pUnknown != nullptr && m_metadata->pUnknown[0] != 0)
        {
            info["p_unknown_nonzero"] = fmt::format("{:x}", m_metadata->pUnknown[0]);
        }

        std::ofstream output(outFilePath);
        output << std::setw(2) << info << std::endl;
        logger->debug("Wrote material metadata with hash {:x} to {}", m_asset->Hash, outFilePath.string());
    }
};

struct ShaderMetadata
{
    char* Name;
    uint8_t Type; // 0 = Pixel Shader, 1 = Vertex Shader, 2 = Geometry Shader, 3 = ??, 4 = ??, 5 = Compute Shader
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

struct ShaderSetMetadata
{
    uint16_t VTableSlot;
    char* Name;
};

class ShaderSetAsset : public BaseAsset<ShaderSetAsset, ShaderSetMetadata>
{
public:
    using BaseAsset<ShaderSetAsset, ShaderSetMetadata>::BaseAsset;

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }
};

struct TextureListMetadata
{
    uint64_t* pHashes;
    char** pNames;
    uint64_t NumEntries;
};

class TextureListAsset : public BaseAsset<TextureListAsset, TextureListMetadata>
{
public:
    using BaseAsset<TextureListAsset, TextureListMetadata>::BaseAsset;

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    bool CanDump() override
    {
        return true;
    }

    void Dump(const std::filesystem::path& outFilePath) override
    {
        using json = nlohmann::json;

        json data;
        for (uint32_t i = 0; i < m_metadata->NumEntries; i++)
        {
            data[fmt::format("{:x}", m_metadata->pHashes[i])] = m_metadata->pNames[i];
        }

        std::ofstream output(outFilePath);
        output << std::setw(2) << data << std::endl;
        spdlog::get("logger")->debug("Wrote texture list with hash {:x} to {}", m_asset->Hash, outFilePath.string());
    }
};

struct SettingsFieldDescriptor
{
    uint16_t Type;
    uint16_t NameStringOffset;
    uint32_t DataOffset : 24;
    uint32_t ArrayDescriptorIndex : 8;
};

static_assert(sizeof(SettingsFieldDescriptor) == 8, "SettingsFieldDescriptor must be 8 bytes");

struct ArrayFieldDescriptor
{
    char Unknown1[44];
    uint32_t NumElements;
    uint32_t ElementSize;
    char Unknown2[20];
};

static_assert(sizeof(ArrayFieldDescriptor) == 72, "ArrayFieldDescriptor must be 72 bytes");

struct SettingsLayoutMetadata
{
    char* Name;
    SettingsFieldDescriptor* Fields;
    void* Unknown1;
    uint32_t NumFields;
    char Unknown2[28];
    char* StringBuffer;
    ArrayFieldDescriptor* ArrayFields;
};

const char* kSettingFieldTypes[] = {
    "bool",
    "int",
    "float",
    "float2",
    "float3",
    "string",
    "asset",
    "asset",
    "static_array",
    "dynamic_array"
};

class SettingsLayoutAsset : public BaseAsset<SettingsLayoutAsset, SettingsLayoutMetadata>
{
public:
    using BaseAsset<SettingsLayoutAsset, SettingsLayoutMetadata>::BaseAsset;
    using json = nlohmann::json;

    bool CanDump() override
    {
        return true;
    }

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    void Dump(const std::filesystem::path& outFilePath) override
    {
        json info;
        info["name"] = m_metadata->Name;
        json fields = json::array();
        for (uint32_t i = 0; i < m_metadata->NumFields; i++)
        {
            json fieldObj;
            const SettingsFieldDescriptor* field = &m_metadata->Fields[i];
            // The Fields seems to be a hash table, so only entries with NameStringOffset != 0 are actually there
            if (field->NameStringOffset != 0)
            {
                fieldObj["type"] = kSettingFieldTypes[field->Type];
                fieldObj["offset"] = field->DataOffset;
                fieldObj["name"] = &m_metadata->StringBuffer[field->NameStringOffset];
                if (field->Type == 8)
                {
                    fieldObj["num_elements"] = m_metadata->ArrayFields[field->ArrayDescriptorIndex].NumElements;
                    fieldObj["element_size"] = m_metadata->ArrayFields[field->ArrayDescriptorIndex].ElementSize;
                }
                fields.push_back(fieldObj);
            }
        }

        info["fields"] = fields;

        std::ofstream output(outFilePath);
        output << std::setw(2) << info << std::endl;
        spdlog::get("logger")->debug("Wrote settings layout with hash {:x} to {}", m_asset->Hash, outFilePath.string());
    }
};

void RegisterAssetTypes()
{
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
    AssetFactory::Register(kShaderSetAssetType, &ShaderSetAsset::CreateMethod);
    AssetFactory::Register(kTextureListType, &TextureListAsset::CreateMethod);
    AssetFactory::Register(kSettingsLayoutType, &SettingsLayoutAsset::CreateMethod);
}

#endif
