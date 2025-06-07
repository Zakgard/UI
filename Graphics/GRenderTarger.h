#pragma once
#include "GDescriptor.h"
#include "GTexture.h"

namespace PEPEngine::Graphics
{
    class GRenderTexture
    {
        std::shared_ptr<GTexture> renderTexture;
        GDescriptor* rtv = nullptr;
        UINT offset = 0;

        void CreateRTV() const;

    public:
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpu() const;

        D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGpu() const;

        const GTexture* GetRenderTexture() const;

        void Resize(UINT newHeight, UINT newWeight) const;

        void ChangeTexture(const ComPtr<ID3D12Resource>& texture);

        void ChangeTexture(const std::shared_ptr<GTexture>& texture);

        void ChangeTexture(const std::shared_ptr<GTexture>& texture, GDescriptor* rtv);

        GRenderTexture(const std::shared_ptr<GTexture>& texture);

        GRenderTexture(const std::shared_ptr<GTexture>& texture, GDescriptor* rtv, UINT offset = 0);
    };
}
