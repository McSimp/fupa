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

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath) override
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
            info["shadow_material"] = Util::HashToString(m_metadata->ShadowMaterialHash);
        }
        
        if (m_metadata->PrepassMaterialHash != 0)
        {
            info["prepass_material"] = Util::HashToString(m_metadata->PrepassMaterialHash);
        }

        if (m_metadata->VSMMaterialHash != 0)
        {
            info["vsm_material"] = Util::HashToString(m_metadata->VSMMaterialHash);
        }
        
        if (m_metadata->TightShadowMaterialHash != 0)
        {
            info["tightshadow_material"] = Util::HashToString(m_metadata->TightShadowMaterialHash);
        }
        
        if (m_metadata->ColpassMaterialHash != 0)
        {
            info["colpass_material"] = Util::HashToString(m_metadata->ColpassMaterialHash);
        }

        if (m_metadata->ShaderSetHash != 0)
        {
            info["shader_set"] = Util::HashToString(m_metadata->ShaderSetHash);
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
                textures.push_back(Util::HashToString(*pCurrentHash));
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

        return { m_metadata->Name };
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

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath) override
    {
        using json = nlohmann::json;

        json data;
        std::unordered_set<std::string> names;
        for (uint32_t i = 0; i < m_metadata->NumEntries; i++)
        {
            data[Util::HashToString(m_metadata->pHashes[i])] = m_metadata->pNames[i];
            names.emplace(m_metadata->pNames[i]);
        }

        std::ofstream output(outFilePath);
        output << std::setw(2) << data << std::endl;
        spdlog::get("logger")->debug("Wrote texture list with hash {:x} to {}", m_asset->Hash, outFilePath.string());

        return std::move(names);
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

struct SettingsLayoutMetadata
{
    char* Name;
    SettingsFieldDescriptor* Fields;
    void* Unknown1;
    uint32_t NumFields;
    char Unknown2[16];
    uint32_t ArrayNumElements;
    uint32_t ArrayElementSize;
    uint32_t Unknown3;
    char* StringBuffer;
    SettingsLayoutMetadata* ArrayFields;
};

static_assert(sizeof(SettingsLayoutMetadata) == 72, "SettingsLayoutMetadata must be 72 bytes");

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

    json LayoutToJSON(const SettingsLayoutMetadata* layout)
    {
        json info;
        if (layout->Name != nullptr)
        {
            info["name"] = layout->Name;
        }
        
        if (layout->ArrayNumElements != 0 && layout->ArrayNumElements != 0xFFFFFFFF)
        {
            info["array_num_elements"] = layout->ArrayNumElements;
        }

        if (layout->ArrayElementSize != 0)
        {
            info["array_element_size"] = layout->ArrayElementSize;
        }

        json fields = json::array();
        for (uint32_t i = 0; i < layout->NumFields; i++)
        {
            json fieldObj;
            const SettingsFieldDescriptor* field = &layout->Fields[i];
            // The Fields seems to be a hash table, so only entries with NameStringOffset != 0 are actually there
            if (field->NameStringOffset != 0)
            {
                fieldObj["type"] = kSettingFieldTypes[field->Type];
                fieldObj["offset"] = field->DataOffset;
                fieldObj["name"] = &layout->StringBuffer[field->NameStringOffset];
                if (field->Type == 8 || field->Type == 9)
                {
                    fieldObj["array"] = LayoutToJSON(&layout->ArrayFields[field->ArrayDescriptorIndex]);
                }
                fields.push_back(fieldObj);
            }
        }

        info["fields"] = fields;

        return info;
    }

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath) override
    {
        json info = LayoutToJSON(m_metadata);
        std::ofstream output(outFilePath);
        output << std::setw(2) << info << std::endl;
        spdlog::get("logger")->debug("Wrote settings layout with hash {:x} to {}", m_asset->Hash, outFilePath.string());

        if (m_metadata->Name != nullptr)
        {
            return { m_metadata->Name };
        }
        else
        {
            return {};
        }
    }
};

struct SettingsMetadata
{
    uint64_t HashOfLayout;
    char* Data;
    char* AssetName;
    char Unknown3[32];
    uint32_t DataSize;
};

struct DynamicArrayInfo
{
    uint32_t NumElements;
    uint32_t DataOffset;
};

class SettingsAsset : public BaseAsset<SettingsAsset, SettingsMetadata>
{
public:
    using BaseAsset<SettingsAsset, SettingsMetadata>::BaseAsset;
    using json = nlohmann::json;

    bool CanDumpPost() override
    {
        return true;
    }

    bool HasEmbeddedName() override
    {
        return m_metadata->AssetName != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->AssetName;
    }

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    json DumpForLayout(const json& layout, char* data, std::unordered_set<std::string>& strings)
    {
        json out;
        for (const auto& field : layout["fields"])
        {
            std::string type = field["type"];
            std::string name = field["name"];
            strings.insert(name);
            if (type == "string" || type == "asset")
            {
                const char* str = *(const char**)(data + field["offset"]);
                strings.insert(str);
                out[name] = str;
            }
            else if (type == "int")
            {
                out[name] = *(int*)(data + field["offset"]);
            }
            else if (type == "bool")
            {
                out[name] = *(bool*)(data + field["offset"]);
            }
            else if (type == "float")
            {
                out[name] = *(float*)(data + field["offset"]);
            }
            else if (type == "float2")
            {
                json floatArray = json::array();
                floatArray.push_back(*(float*)(data + field["offset"]));
                floatArray.push_back(*(float*)(data + field["offset"] + 4));
                out[name] = floatArray;
            }
            else if (type == "float3")
            {
                json floatArray = json::array();
                floatArray.push_back(*(float*)(data + field["offset"]));
                floatArray.push_back(*(float*)(data + field["offset"] + 4));
                floatArray.push_back(*(float*)(data + field["offset"] + 8));
                out[name] = floatArray;
            }
            else if (type == "static_array")
            {
                json arrayOfSettings = json::array();
                for (uint32_t i = 0; i < field["array"]["array_num_elements"]; i++)
                {
                    arrayOfSettings.push_back(DumpForLayout(field["array"], data + field["offset"] + (i * field["array"]["array_element_size"]), strings));
                }
                out[name] = arrayOfSettings;
            }
            else if (type == "dynamic_array")
            {
                json arrayOfSettings = json::array();
                const DynamicArrayInfo* dynamicArrayInfo = reinterpret_cast<DynamicArrayInfo*>(data + field["offset"]);
                char* dynamicArrayData = data + dynamicArrayInfo->DataOffset;
                for (uint32_t i = 0; i < dynamicArrayInfo->NumElements; i++)
                {
                    arrayOfSettings.push_back(DumpForLayout(field["array"], dynamicArrayData + (i * field["array"]["array_element_size"]), strings));
                }
                out[name] = arrayOfSettings;
            }
            else
            {
                spdlog::get("logger")->error("No handler found for field type {} for settings layout {}", type, Util::HashToString(GetHash()));
            }
        }

        return out;
    }

    std::unordered_set<std::string> DumpPost(tDumpedFileOpenerFunc opener, const std::filesystem::path& outFilePath) override
    {
        auto logger = spdlog::get("logger");
        json data;
        data["layout_hash"] = Util::HashToString(m_metadata->HashOfLayout);

        if (m_metadata->AssetName != nullptr)
        {
            data["name"] = m_metadata->AssetName;
        }

        // Load the layout asset
        auto fLayoutAsset = opener(m_metadata->HashOfLayout);
        if (!fLayoutAsset.has_value())
        {
            logger->error("Setting asset with hash {} is missing layout with hash {}", Util::HashToString(GetHash()), Util::HashToString(m_metadata->HashOfLayout));
            return {};
        }

        json layout;
        fLayoutAsset.value() >> layout;

        // Iterate over the fields in the layout and read them out of the settings data
        std::unordered_set<std::string> strings;
        json out = DumpForLayout(layout, m_metadata->Data, strings);

        std::ofstream output(outFilePath);
        output << std::setw(2) << out << std::endl;
        logger->debug("Wrote settings with hash {} to {}", Util::HashToString(m_asset->Hash), outFilePath.string());
        
        return strings;
    }
};

void RegisterAssetTypes()
{
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
    AssetFactory::Register(kShaderSetAssetType, &ShaderSetAsset::CreateMethod);
    AssetFactory::Register(kTextureListType, &TextureListAsset::CreateMethod);
    AssetFactory::Register(kSettingsLayoutType, &SettingsLayoutAsset::CreateMethod);
    AssetFactory::Register(kSettingsType, &SettingsAsset::CreateMethod);
}

#endif
