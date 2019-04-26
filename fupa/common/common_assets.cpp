#include "pch.h"
#include "../pch.h"

std::map<std::string, int> PatchAsset::BuildRPakMap()
{
    std::map<std::string, int> rpaks;
    for (uint32_t i = 0; i < m_metadata->NumFiles; i++)
    {
        rpaks[m_metadata->PakNames[i]] = m_metadata->PakNumbers[i];
    }

    return rpaks;
}

const char* DATATABLE_TYPES[] = {
    "bool",
    "int",
    "float",
    "vector",
    "string",
    "asset",
    "asset_noprecache"
};

class DatatableAsset : public BaseAsset<DatatableAsset, DatatableMetadata>
{
public:
    using BaseAsset<DatatableAsset, DatatableMetadata>::BaseAsset;

    bool CanDump() override
    {
        return true;
    }

    std::filesystem::path GetBaseOutputDirectory() override
    {
        return "datatables";
    }

    std::string GetOutputFileExtension() override
    {
        return ".csv";
    }

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
    {
        auto logger = spdlog::get("logger");

        std::ofstream output(outFilePath);
        for (int32_t col = 0; col < m_metadata->ColumnCount; col++)
        {
            std::string name = m_metadata->Columns[col].Name;
            Util::ReplaceAll(name, "\"", "\"\"");
            output << "\"" << name << "\"";
            if (col != (m_metadata->ColumnCount - 1))
            {
                output << ",";
            }
            else
            {
                output << std::endl;
            }
        }

        std::unordered_set<std::string> names;

        for (int32_t row = 0; row < m_metadata->RowCount; row++)
        {
            for (int32_t col = 0; col < m_metadata->ColumnCount; col++)
            {
                std::string val;
                DatatableColumn& dtCol = m_metadata->Columns[col];
                void* data = m_metadata->RowData + (m_metadata->RowSize * row) + dtCol.Offset;
                if (dtCol.Type == 0)
                {
                    val = fmt::format("{}", *static_cast<bool*>(data));
                }
                else if (dtCol.Type == 1)
                {
                    val = fmt::format("{}", *static_cast<int*>(data));
                }
                else if (dtCol.Type == 2)
                {
                    val = fmt::format("{}", *static_cast<float*>(data));
                }
                else if (dtCol.Type == 3)
                {
                    val = fmt::format("VECTOR");
                }
                else if (dtCol.Type == 4)
                {
                    val = *static_cast<char**>(data);
                    names.emplace(val);
                    Util::ReplaceAll(val, "\"", "\"\"");
                }
                else if (dtCol.Type == 5)
                {
                    val = *static_cast<char**>(data);
                    names.emplace(val);
                    Util::ReplaceAll(val, "\"", "\"\"");
                }
                else if (dtCol.Type == 6)
                {
                    val = *static_cast<char**>(data);
                    names.emplace(val);
                    Util::ReplaceAll(val, "\"", "\"\"");
                }
                else
                {
                    val = fmt::format("UNKNOWN");
                }

                output << "\"" << val << "\"";

                if (col != (m_metadata->ColumnCount - 1))
                {
                    output << ",";
                }
                else
                {
                    output << std::endl;
                }
            }
        }

        logger->debug("Wrote datatable with hash {} to {}", m_asset->Hash, outFilePath.string());
        return std::move(names);
    }
};

DXGI_FORMAT TEXTURE_FORMATS[] = {
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D16_UNORM
};

struct CompressionInfo
{
    uint8_t BytesPerBlock;
    uint8_t BlockSize;
};

CompressionInfo COMPRESSION_INFO[] = {
    { 8, 4 },
    { 8, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 8, 4 },
    { 8, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 16, 4 },
    { 12, 1 },
    { 12, 1 },
    { 12, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 8, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 2, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 4, 1 },
    { 4, 1 },
    { 4, 1 },
    { 2, 1 },
};

class TextureAsset : public BaseAsset<TextureAsset, TextureMetadata>
{
public:
    using BaseAsset<TextureAsset, TextureMetadata>::BaseAsset;

    bool HasEmbeddedName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetEmbeddedName() override
    {
        return m_metadata->Name;
    }

    bool CanDump() override
    {
        return m_data != nullptr;
    }

    std::filesystem::path GetBaseOutputDirectory() override
    {
        return "textures";
    }

    std::string GetOutputFileExtension() override
    {
        return ".dds";
    }

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
    {
        auto logger = spdlog::get("logger");

        if (m_metadata->Unknown2_ShouldBeZero)
        {
            logger->error("Cannot dump texture {} - Unknown2_ShouldBeZero was non-zero", GetNameOrHash());
            return {};
        }

        if (m_metadata->Height == 0)
        {
            logger->error("Cannot dump texture {} - Height was zero", GetNameOrHash());
            return {};
        }

        const uint64_t MIP_ALIGNMENT = 16;
        D3D11_SUBRESOURCE_DATA subResources[16];
        const uint8_t* nextTextureData = m_data;
#ifdef APEX
        uint8_t skippedMips = m_metadata->SkippedMips + m_metadata->SomeOtherSkippedMips;
#elif defined(TTF2)
        uint8_t skippedMips = m_metadata->SkippedMips;
#endif
        for (int32_t mip = m_metadata->MipLevels + skippedMips - 1; mip >= skippedMips; mip--)
        {
            int32_t width = std::max(1, m_metadata->Width >> mip);
            int32_t height = std::max(1, m_metadata->Height >> mip);

            uint8_t bytesPerBlock = COMPRESSION_INFO[m_metadata->Format].BytesPerBlock;
            uint8_t blockSize = COMPRESSION_INFO[m_metadata->Format].BlockSize;

            subResources[mip].pSysMem = nextTextureData;
            subResources[mip].SysMemPitch = bytesPerBlock * ((width + blockSize - 1) / blockSize);
            subResources[mip].SysMemSlicePitch = bytesPerBlock * ((width + blockSize - 1) / blockSize) * ((height + blockSize - 1) / blockSize);

            nextTextureData += (subResources[mip].SysMemSlicePitch + MIP_ALIGNMENT - 1) & ~(MIP_ALIGNMENT - 1);
        }

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = std::max(1, m_metadata->Width >> skippedMips);
        desc.Height = std::max(1, m_metadata->Height >> skippedMips);
        desc.MipLevels = m_metadata->MipLevels;
        desc.ArraySize = 1;
        desc.Format = TEXTURE_FORMATS[m_metadata->Format];
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING; // TTF2 has this as D3D11_USAGE_IMMUTABLE
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // TTF2 has this as 0
        desc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = D3D::Device->CreateTexture2D(&desc, &subResources[skippedMips], &tex);
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to CreateTexture2D (0x{:x})", GetNameOrHash(), hr);
            return {};
        }

        DirectX::ScratchImage image;
        hr = DirectX::CaptureTexture(D3D::Device.Get(), D3D::DeviceContext.Get(), tex.Get(), image);
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to CaptureTexture (0x{:x})", GetNameOrHash(), hr);
            return {};
        }

        hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, Util::Widen(outFilePath.string()).c_str());
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to SaveToDDSFile (0x{:x})", GetNameOrHash(), hr);
            return {};
        }

        logger->debug("Wrote texture {} to {}", GetNameOrHash(), outFilePath.string());

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

class UIImageAtlasAsset : public BaseAsset<UIImageAtlasAsset, UIImageAtlasMetadata>
{
    using BaseAsset<UIImageAtlasAsset, UIImageAtlasMetadata>::BaseAsset;

    bool CanDump() override
    {
        return m_data != nullptr;
    }

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
    {
        using json = nlohmann::json;
        auto logger = spdlog::get("logger");
        const AtlasElement* data = reinterpret_cast<const AtlasElement*>(m_data);

        std::unordered_set<std::string> names;

        json outputObj = json::object();
        outputObj["texture_hash"] = Util::HashToString(m_metadata->TextureHash);
        outputObj["width"] = m_metadata->FullWidth;
        outputObj["height"] = m_metadata->FullHeight;

        json outputArray = json::array();
        for (uint16_t i = 0; i < m_metadata->NumElements; i++)
        {
            json obj;

            if (m_metadata->ElementStrings != nullptr)
            {
                const char* name = &m_metadata->ElementStrings[m_metadata->UnknownEntries[i].NameStringOffset];
                obj["name"] = std::string(name);
                names.emplace(name);
            }

            obj["subtexture_hash"] = Util::HashToString(m_metadata->UnknownEntries[i].HalfHashName);
            obj["width"] = m_metadata->PixelSizes[i].Width;
            obj["height"] = m_metadata->PixelSizes[i].Height;
            obj["u"] = data[i].U;
            obj["v"] = data[i].V;
            obj["u_width"] = data[i].Width;
            obj["v_height"] = data[i].Height;

            outputArray.push_back(obj);
        }

        outputObj["elements"] = outputArray;
        
        std::ofstream output(outFilePath);
        output << std::setw(2) << outputObj << std::endl;
        logger->debug("Wrote uimg data with hash {:x} to {}", m_asset->Hash, outFilePath.string());

        return names;
    }
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

class RSONFileAsset : public BaseAsset<RSONFileAsset, RSONDataDescriptor>
{
public:
    using BaseAsset<RSONFileAsset, RSONDataDescriptor>::BaseAsset;
    using json = nlohmann::json;

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
            const char* str = reinterpret_cast<char*>(data->Data);
            m_strings.emplace(str);
            return str;
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
                m_strings.emplace(entries[i]);
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
                if (entries[i])
                {
                    outputArray.push_back(ParseObject(entries[i]));
                }
            }
            return outputArray;
        }
        else
        {
            spdlog::get("logger")->error("No RSON parser found for type 0x{:x}", data->Type);
            return nullptr;
        }
    }

    std::filesystem::path GetBaseOutputDirectory() override
    {
        return "rson";
    }

    std::string GetOutputFileExtension() override
    {
        return ".json";
    }

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
    {
        auto logger = spdlog::get("logger");
        std::ofstream output(outFilePath);
        output << std::setw(2) << ParseData(m_metadata) << std::endl;
        logger->debug("Wrote rson file with hash {:x} to {}", m_asset->Hash, outFilePath.string());
        return std::move(m_strings);
    }

private:
    std::unordered_set<std::string> m_strings;
};

class MaterialAsset : public BaseAsset<MaterialAsset, MaterialMetadata>
{
public:
    using BaseAsset<MaterialAsset, MaterialMetadata>::BaseAsset;
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

    std::unordered_set<std::string> Dump(const std::filesystem::path& outFilePath, StarpakReader& starpakReader) override
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

#ifdef APEX
        if (m_metadata->ColpassMaterialHash != 0)
        {
            info["colpass_material"] = Util::HashToString(m_metadata->ColpassMaterialHash);
        }
#endif

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

void RegisterCommonAssetTypes()
{
    AssetFactory::Register(kPatchAssetType, &PatchAsset::CreateMethod);
    AssetFactory::Register(kDatatableAssetType, &DatatableAsset::CreateMethod);
    AssetFactory::Register(kTextureAssetType, &TextureAsset::CreateMethod);
    AssetFactory::Register(kUIImageAtlasType, &UIImageAtlasAsset::CreateMethod);
    AssetFactory::Register(kRSONFileAssetType, &RSONFileAsset::CreateMethod);
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
}
