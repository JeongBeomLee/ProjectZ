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
            Logger::Instance().Debug("�ؽ�ó ���ҽ� ����: {}", GetName());
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
                Logger::Instance().Error("����̽� �Ǵ� Ŀ�ǵ� ť�� �����ϴ�: {}", GetPath());
                return false;
            }

            try {
                // ���ҽ� ���ε� ��ġ ����
                DirectX::ResourceUploadBatch resourceUpload(device);

                // ���ε� ����
                resourceUpload.Begin();

                // ���� ��� ��ȯ
                std::wstring widePath(GetPath().begin(), GetPath().end());

                // ���� ���� ���� Ȯ��
                if (!std::filesystem::exists(GetPath())) {
                    throw std::runtime_error("�ؽ�ó ������ ã�� �� �����ϴ�.");
                }

                // �ؽ�ó �ε� (�ӽ� ���� �����Ͽ� �����ϵ��� ����)
                ComPtr<ID3D12Resource> textureResource;
                ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
                    device,
                    resourceUpload,
                    widePath.c_str(),
                    textureResource.GetAddressOf()
                ));

                // ���ε� �Ϸ� ���
                auto uploadResult = resourceUpload.End(cmdQueue);
                uploadResult.wait();

                // ���ҽ��� ��ȿ���� Ȯ��
                if (!textureResource) {
                    throw std::runtime_error("�ؽ�ó ���ҽ� ���� ����");
                }

                // ���ҽ� ����
                m_texture.Swap(textureResource);

                // ũ�� ���� ����
                auto desc = m_texture->GetDesc();
                SetSize(static_cast<size_t>(desc.Width * desc.Height *
                    GetBytesPerPixel(desc.Format)));

                SetState(State::Ready);
                Logger::Instance().Info("�ؽ�ó �ε� �Ϸ�: {} ({}x{}, {}bytes)",
                    GetName(), desc.Width, desc.Height, GetSize());

                return true;
            }
            catch (const std::exception& e) {
                SetState(State::Failed);
                Logger::Instance().Error("�ؽ�ó �ε� �� ���� �߻�: {} - {}",
                    GetPath(), e.what());
                return false;
            }
        }

        void Unload() override {
            if (m_texture) {
                m_texture.Reset();
                SetState(State::Unloaded);
                Logger::Instance().Debug("�ؽ�ó ��ε�: {}", GetName());
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