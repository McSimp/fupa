#include "pch.h"

namespace D3D {
Microsoft::WRL::ComPtr<ID3D11Device> Device;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;

void Initialize()
{
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &Device,
        nullptr,
        &DeviceContext);

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to initialize D3D11 device");
    }
}
}
