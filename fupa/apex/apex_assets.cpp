#include "pch.h"
#include "../pch.h"

#ifdef APEX

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

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
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

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
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

    std::unordered_set<std::string> DumpPost(tDumpedFileOpenerFunc opener, const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
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
    AssetFactory::Register(kShaderSetAssetType, &ShaderSetAsset::CreateMethod);
    AssetFactory::Register(kTextureListType, &TextureListAsset::CreateMethod);
    AssetFactory::Register(kSettingsLayoutType, &SettingsLayoutAsset::CreateMethod);
    AssetFactory::Register(kSettingsType, &SettingsAsset::CreateMethod);
}

#endif
