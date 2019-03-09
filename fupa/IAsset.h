#pragma once

class IAsset
{
public:
    virtual uint32_t GetDataSize() = 0;
    virtual const uint8_t* GetData() = 0;
    virtual uint32_t GetType() = 0;
    virtual bool HasName() = 0;
    virtual std::string GetName() = 0;
    virtual uint64_t GetHash() = 0;
    virtual bool CanDump() = 0;
    virtual void Dump(const std::filesystem::path& outputDir) = 0;
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

    uint32_t GetDataSize() override
    {
        return m_asset->DataSize;
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

    bool HasName() override
    {
        return KnownAssetCache::HasName(m_asset->Hash);
    }

    std::string GetName() override
    {
        return KnownAssetCache::GetName(m_asset->Hash);
    }

    bool CanDump() override
    {
        return false;
    }

    void Dump(const std::filesystem::path& outputDir) override
    {
        throw std::runtime_error(fmt::format("Dump not implemented for {}", m_asset->Type));
    }

    std::string GetNameOrHash()
    {
        if (HasName())
        {
            return GetName();
        }
        else
        {
            return std::to_string(m_asset->Hash);
        }
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

void RegisterAssetTypes();
