#pragma once
#include "pch.h"
#include "IResource.h"
#include "GraphicsDevice.h"
#include "Logger.h"

namespace Resource
{
    class TextureResource : public IResource {
    public:
        explicit TextureResource(const std::string& path)
            : IResource(Type::Texture, path,
                std::filesystem::path(path).filename().string())
            , m_texture(nullptr)
        {
            Logger::Instance().Debug("텍스처 리소스 생성: {}", GetName());
        }

        ~TextureResource() override {
            Unload();
        }

        bool Load() override {
            if (IsReady()) return true;

            SetState(State::Loading);

            auto* device = GraphicsDevice::Instance().GetDevice();
            auto* cmdQueue = GraphicsDevice::Instance().GetCommandQueue();

            if (!device || !cmdQueue) {
                SetState(State::Failed);
                Logger::Instance().Error("디바이스 또는 커맨드 큐가 없습니다: {}", GetPath());
                return false;
            }

            try {
                // 리소스 업로드 배치 생성
                DirectX::ResourceUploadBatch resourceUpload(device);

                // 업로드 시작
                resourceUpload.Begin();

                // 파일 경로 변환
                std::wstring widePath(GetPath().begin(), GetPath().end());

                // 파일 존재 여부 확인
                if (!std::filesystem::exists(GetPath())) {
                    throw std::runtime_error("텍스처 파일을 찾을 수 없습니다.");
                }

                // 텍스처 로드 (임시 변수 생성하여 설정하도록 변경)
                ComPtr<ID3D12Resource> textureResource;
                ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
                    device,
                    resourceUpload,
                    widePath.c_str(),
                    textureResource.GetAddressOf()
                ));

                // 업로드 완료 대기
                auto uploadResult = resourceUpload.End(cmdQueue);
                uploadResult.wait();

                // 리소스가 유효한지 확인
                if (!textureResource) {
                    throw std::runtime_error("텍스처 리소스 생성 실패");
                }

                // 리소스 설정
                m_texture.Swap(textureResource);

                // 크기 정보 설정
                auto desc = m_texture->GetDesc();
                SetSize(static_cast<size_t>(desc.Width * desc.Height *
                    GetBytesPerPixel(desc.Format)));

                SetState(State::Ready);
                Logger::Instance().Info("텍스처 로드 완료: {} ({}x{}, {}bytes)",
                    GetName(), desc.Width, desc.Height, GetSize());

                return true;
            }
            catch (const std::exception& e) {
                SetState(State::Failed);
                Logger::Instance().Error("텍스처 로드 중 예외 발생: {} - {}",
                    GetPath(), e.what());
                return false;
            }
        }

        void Unload() override {
            if (m_texture) {
                m_texture.Reset();
                SetState(State::Unloaded);
                Logger::Instance().Debug("텍스처 언로드: {}", GetName());
            }
        }

        ID3D12Resource* GetTexture() const { return m_texture.Get(); }

        D3D12_RESOURCE_DESC GetDesc() const {
            return m_texture ? m_texture->GetDesc() : D3D12_RESOURCE_DESC{};
        }

    private:
        size_t GetBytesPerPixel(DXGI_FORMAT format) const {
            switch (format) {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return 4;
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return 4;
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                return 16;
            default:
                return 4;
            }
        }

        ComPtr<ID3D12Resource> m_texture;
    };
}