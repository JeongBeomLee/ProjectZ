#include "pch.h"
#include "TextureResource.h"
#include "Engine.h"
#include "Logger.h"

namespace Resource
{
    TextureResource::~TextureResource()
    {
        Unload();
    }

    bool TextureResource::Load(const std::string& path)
    {
        m_path = path;
        SetState(ResourceState::Loading);

        std::wstring widePath(path.begin(), path.end());

        try {
            if (!CreateTextureFromDDS(widePath)) {
                SetError("Failed to create texture from DDS file");
                return false;
            }

            if (!CreateShaderResourceView()) {
                SetError("Failed to create shader resource view");
                return false;
            }

            SetState(ResourceState::Loaded);
            Logger::Instance().Info("텍스처 로드 성공: {}", path);
            return true;
        }
        catch (const std::exception& e) {
            SetError(std::string("텍스처 로드 실패: ") + e.what());
            return false;
        }
    }

    void TextureResource::Unload()
    {
        m_texture.Reset();
        m_uploadBuffer.Reset();
        SetState(ResourceState::Unloaded);
        Logger::Instance().Debug("텍스처 언로드: {}", m_path);
    }

    bool TextureResource::CreateTextureFromDDS(const std::wstring& widePath)
    {
        auto device = Engine::Instance().GetDevice();
        auto commandQueue = Engine::Instance().GetCommandQueue();
        if (!device || !commandQueue) {
            return false;
        }

        // 리소스 업로드 배치 생성
        DirectX::ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // DDS 텍스처 로드
        ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
            device,
            resourceUpload,
            widePath.c_str(),
            m_texture.ReleaseAndGetAddressOf()));

        // SRV 설명자 설정
        m_srvDesc = {};
        m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_srvDesc.Format = m_texture->GetDesc().Format;
        m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        m_srvDesc.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;

        // 업로드 완료 대기
        auto uploadResourcesFinished = resourceUpload.End(commandQueue);
        uploadResourcesFinished.wait();

        return true;
    }

    bool TextureResource::CreateShaderResourceView()
    {
        auto device = Engine::Instance().GetDevice();
        auto descHeap = Engine::Instance().GetDescriptorHeap();
        if (!device || !descHeap) return false;

        // 디스크립터 할당 및 생성
        UINT descriptorIndex = Engine::Instance().GetSrvDescriptorIndex();
        UINT descriptorSize = Engine::Instance().GetDescriptorIncrementSize();

        m_srvDesc = {};
        m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_srvDesc.Format = m_texture->GetDesc().Format;
        m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        m_srvDesc.Texture2D.MipLevels = m_texture->GetDesc().MipLevels;

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.Offset(descriptorIndex, descriptorSize);

        device->CreateShaderResourceView(m_texture.Get(), &m_srvDesc, srvHandle);

		// GPU 핸들 저장
        m_srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            descHeap->GetGPUDescriptorHandleForHeapStart(),
            descriptorIndex,
            descriptorSize);

        return true;
    }
}