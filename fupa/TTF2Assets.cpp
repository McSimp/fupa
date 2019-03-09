#include "pch.h"

std::map<std::string, int> PatchAsset::BuildRPakMap()
{
    std::map<std::string, int> rpaks;
    for (uint32_t i = 0; i < m_metadata->NumFiles; i++)
    {
        rpaks[m_metadata->PakNames[i]] = m_metadata->PakNumbers[i];
    }

    return rpaks;
}

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

struct TextureMetadata
{
    uint64_t Unknown1;
    char* Name;
    uint16_t Width; // This is the max - might not actually contain this in the rpak
    uint16_t Height;
    uint16_t Unknown2_ShouldBeZero;
    uint16_t Format;
    uint32_t DataSize;
    uint32_t Unknown3;
    uint8_t Unknown4;
    uint8_t MipLevels;
    uint8_t SkippedMips; // Number of mip levels that aren't present (starting from largest size)
};

class TextureAsset : public BaseAsset<TextureAsset, TextureMetadata>
{
public:
    using BaseAsset<TextureAsset, TextureMetadata>::BaseAsset;

    bool HasName() override
    {
        return m_metadata->Name != nullptr;
    }

    std::string GetName() override
    {
        return m_metadata->Name;
    }

    bool CanDump() override
    {
        return m_data != nullptr;
    }

    void Dump(const std::filesystem::path& outputDir) override
    {
        auto logger = spdlog::get("logger");

        if (m_metadata->Unknown2_ShouldBeZero)
        {
            logger->error("Cannot dump texture {} - Unknown2_ShouldBeZero was non-zero", m_metadata->Name);
            return;
        }

        if (m_metadata->Height == 0)
        {
            logger->error("Cannot dump texture {} - Height was zero", m_metadata->Name);
            return;
        }

        const uint64_t MIP_ALIGNMENT = 16;
        D3D11_SUBRESOURCE_DATA subResources[16];
        const uint8_t* nextTextureData = m_data;
        for (int32_t mip = m_metadata->MipLevels + m_metadata->SkippedMips - 1; mip >= m_metadata->SkippedMips; mip--)
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
        desc.Width = std::max(1, m_metadata->Width >> m_metadata->SkippedMips);
        desc.Height = std::max(1, m_metadata->Height >> m_metadata->SkippedMips);
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
        HRESULT hr = D3D::Device->CreateTexture2D(&desc, &subResources[m_metadata->SkippedMips], &tex);
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to CreateTexture2D (0x{:x})", m_metadata->Name, hr);
            return;
        }

        std::filesystem::path outFilePath = outputDir / "textures" / (std::string(m_metadata->Name) + ".dds");
        std::filesystem::path outFileDir = outFilePath;
        outFileDir.remove_filename();
        std::filesystem::create_directories(outFileDir);

        DirectX::ScratchImage image;
        hr = DirectX::CaptureTexture(D3D::Device.Get(), D3D::DeviceContext.Get(), tex.Get(), image);
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to CaptureTexture (0x{:x})", m_metadata->Name, hr);
            return;
        }

        hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, Util::Widen(outFilePath.string()).c_str());
        if (FAILED(hr))
        {
            logger->error("Cannot dump texture {} - failed to SaveToDDSFile (0x{:x})", m_metadata->Name, hr);
            return;
        }

        logger->info("Wrote {} to {}", m_metadata->Name, outFilePath.string());
    }
};


void RegisterAssetTypes()
{
    AssetFactory::Register(kPatchAssetType, &PatchAsset::CreateMethod);
    AssetFactory::Register(kTextureAssetType, &TextureAsset::CreateMethod);
}
