#include "pch.h"

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

    bool HasName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetName() override
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

    bool HasName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetName() override
    {
        return m_metadata->Name;
    }
};

void RegisterAssetTypes()
{
    AssetFactory::Register(kMaterialAssetType, &MaterialAsset::CreateMethod);
    AssetFactory::Register(kShaderAssetType, &ShaderAsset::CreateMethod);
}

#endif
