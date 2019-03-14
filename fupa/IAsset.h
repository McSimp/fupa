#pragma once

typedef std::function<std::optional<std::ifstream>(uint64_t)> tDumpedFileOpenerFunc;

class IAsset
{
public:
    virtual uint32_t GetMetadataSize() = 0;
    virtual const uint8_t* GetData() = 0;
    virtual uint32_t GetType() = 0;
    virtual uint64_t GetHash() = 0;
    virtual std::filesystem::path GetOutputFilePath() = 0;

    virtual bool HasEmbeddedName() = 0;
    virtual std::string GetEmbeddedName() = 0;
    virtual std::filesystem::path GetBaseOutputDirectory() = 0;
    virtual std::string GetOutputFileExtension() = 0;

    virtual bool CanDump() = 0;
    virtual std::set<std::string> Dump(const std::filesystem::path& outFilePath) = 0; // return a list of strings that can be used later for asset names

    virtual bool CanDumpPost() = 0;
    virtual std::set<std::string> DumpPost(tDumpedFileOpenerFunc opener, const std::filesystem::path& outFilePath) = 0; // return a list of strings that can be used later for asset names
};

struct AssetDefinition;

template<typename T, typename M>
class BaseAsset : public IAsset
{
public:
    BaseAsset(const AssetDefinition* asset, const uint8_t* metadata, const uint8_t* data)
    {
        m_asset = asset;
        m_metadata = reinterpret_cast<const M*>(metadata);
        m_data = data;
    }

    uint32_t GetMetadataSize() override
    {
        return m_asset->MetadataSize;
    }

    const uint8_t* GetData() override
    {
        return m_data;
    }

    uint32_t GetType() override
    {
        return m_asset->Type;
    }

    uint64_t GetHash() override
    {
        return m_asset->Hash;
    }

    bool HasEmbeddedName() override
    {
        return false;
    }

    std::string GetEmbeddedName() override
    {
        throw std::runtime_error("BaseAssets do not have embedded names");
    }

    bool CanDump() override
    {
        return false;
    }

    std::set<std::string> Dump(const std::filesystem::path& outputFilePath) override
    {
        throw std::runtime_error(fmt::format("Dump not implemented for {}", m_asset->Type));
    }

    bool CanDumpPost() override
    {
        return false;
    }

    std::set<std::string> DumpPost(tDumpedFileOpenerFunc opener, const std::filesystem::path& outFilePath) override
    {
        throw std::runtime_error(fmt::format("DumpPost not implemented for {}", m_asset->Type));
    }

    std::string GetNameOrHash()
    {
        if (HasEmbeddedName())
        {
            return GetEmbeddedName();
        }
        else
        {
            return fmt::format("{:x}", m_asset->Hash);
        }
    }

    std::filesystem::path GetBaseOutputDirectory() override
    {
        const char* str = reinterpret_cast<const char*>(&m_asset->Type);
        return std::string(str, strnlen(str, 4));
    }

    std::string GetOutputFileExtension() override
    {
        return ".dat";
    }

    std::filesystem::path GetOutputFilePath() override
    {
        return GetBaseOutputDirectory() / (GetNameOrHash() + GetOutputFileExtension());
    }

    static std::unique_ptr<IAsset> CreateMethod(const AssetDefinition* asset, const uint8_t* metadata, const uint8_t* data)
    {
        return std::make_unique<T>(asset, metadata, data);
    }

protected:
    const AssetDefinition* m_asset;
    const M* m_metadata;
    const uint8_t* m_data;
};

void RegisterCommonAssetTypes();
void RegisterAssetTypes();
