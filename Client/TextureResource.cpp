#include "pch.h"
#include "TextureResource.h"
#include "Engine.h"
#include "Logger.h"

TextureResource::TextureResource(const std::string& id, const std::string& path)
	: IResource(id, path)
	, m_texture(nullptr)
{
}

TextureResource::~TextureResource()
{
	Unload();
}

bool TextureResource::Load()
{
    if (m_isLoaded) return true;

    // 리소스 업로드 배치 생성
    auto& engine = Engine::Instance();
    DirectX::ResourceUploadBatch resourceUpload(engine.GetDevice());
    resourceUpload.Begin();

    // DDS 텍스처 로드
    try {
        ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
            engine.GetDevice(),
            resourceUpload,
            std::wstring(m_path.begin(), m_path.end()).c_str(),
            m_texture.ReleaseAndGetAddressOf()));
    }
    catch (const std::exception& e) {
        Logger::Instance().Error("텍스처 로드 실패: {}, 오류: {}", m_path, e.what());
        return false;
    }

    // 업로드 완료 대기
    auto uploadResourcesFinished = resourceUpload.End(engine.GetCommandQueue());
    uploadResourcesFinished.wait();

    m_isLoaded = true;
    Logger::Instance().Info("텍스처 로드 완료: {}", m_path);
    return true;
}

void TextureResource::Unload()
{
    if (!m_isLoaded) return;

    m_texture.Reset();
    m_isLoaded = false;
    Logger::Instance().Info("텍스처 언로드: {}", m_path);
}
