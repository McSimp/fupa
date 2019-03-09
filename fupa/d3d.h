#pragma once

namespace D3D {
extern Microsoft::WRL::ComPtr<ID3D11Device> Device;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;

void Initialize();
}
