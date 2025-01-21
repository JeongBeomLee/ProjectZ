#include "pch.h"
#include "MaterialResource.h"
#include "Engine.h"
#include "Logger.h"

MaterialResource::MaterialResource(const std::string& id, const std::string& path)
    : IResource(id, path)
    , m_constantBufferDirty(true)
{
    // 기본 속성 초기화
    m_properties.albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_properties.emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_properties.roughness = 0.5f;
    m_properties.metallic = 0.0f;
    m_properties.ambientOcclusion = 1.0f;
    m_properties.padding = 0.0f;

    // 텍스처 플래그 초기화
    ZeroMemory(&m_textureFlags, sizeof(TextureFlags));
}

MaterialResource::~MaterialResource()
{
    Unload();
}

bool MaterialResource::Load()
{
    if (m_isLoaded) return true;

    // 상수 버퍼 생성
    if (!CreateConstantBuffer()) {
        Logger::Instance().Error("머티리얼 상수 버퍼 생성 실패: {}", m_path);
        return false;
    }

    // TODO: JSON 파일에서 머티리얼 속성 로드
    // 현재는 기본값 사용

    m_isLoaded = true;
    Logger::Instance().Info("머티리얼 로드 완료: {}", m_path);
    return true;
}

void MaterialResource::Unload()
{
    if (!m_isLoaded) return;

    m_materialConstantBuffer.Reset();
    m_textureFlagsConstantBuffer.Reset();
    m_albedoTexture.Reset();
    m_normalTexture.Reset();
    m_metallicRoughnessTexture.Reset();
    m_emissiveTexture.Reset();
    m_occlusionTexture.Reset();

    m_vertexShader.reset();
    m_pixelShader.reset();

    m_isLoaded = false;
    Logger::Instance().Info("머티리얼 언로드: {}", m_path);
}

bool MaterialResource::IsLoaded() const
{
    return m_isLoaded;
}

void MaterialResource::SetVertexShader(std::shared_ptr<ShaderResource> shader)
{
    m_vertexShader = shader;
}

void MaterialResource::SetPixelShader(std::shared_ptr<ShaderResource> shader)
{
    m_pixelShader = shader;
}

void MaterialResource::SetAlbedoTexture(ID3D12Resource* texture)
{
    m_albedoTexture = texture;
    m_textureFlags.hasAlbedoTexture = texture != nullptr;
    UpdateTextureFlagsBuffer();
}

void MaterialResource::SetNormalTexture(ID3D12Resource* texture)
{
    m_normalTexture = texture;
    m_textureFlags.hasNormalTexture = texture != nullptr;
    UpdateTextureFlagsBuffer();
}

void MaterialResource::SetMetallicRoughnessTexture(ID3D12Resource* texture)
{
    m_metallicRoughnessTexture = texture;
    m_textureFlags.hasMetallicRoughnessTexture = texture != nullptr;
    UpdateTextureFlagsBuffer();
}

void MaterialResource::SetEmissiveTexture(ID3D12Resource* texture)
{
    m_emissiveTexture = texture;
    m_textureFlags.hasEmissiveTexture = texture != nullptr;
    UpdateTextureFlagsBuffer();
}

void MaterialResource::SetOcclusionTexture(ID3D12Resource* texture)
{
    m_occlusionTexture = texture;
    m_textureFlags.hasOcclusionTexture = texture != nullptr;
    UpdateTextureFlagsBuffer();
}

void MaterialResource::SetProperties(const MaterialProperties& properties)
{
    m_properties = properties;
    m_constantBufferDirty = true;
}

void MaterialResource::UpdateConstantBuffer()
{
    if (!m_constantBufferDirty || !m_materialConstantBuffer) return;

    // 상수 버퍼 데이터 업데이트
    UINT8* mappedData;
    CD3DX12_RANGE readRange(0, 0);
    if (SUCCEEDED(m_materialConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)))) {
        memcpy(mappedData, &m_properties, sizeof(MaterialProperties));
        m_materialConstantBuffer->Unmap(0, nullptr);
    }

    m_constantBufferDirty = false;
}

void MaterialResource::UpdateTextureFlagsBuffer()
{
    if (!m_textureFlagsConstantBuffer) return;

    UINT8* mappedData;
    CD3DX12_RANGE readRange(0, 0);
    if (SUCCEEDED(m_textureFlagsConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)))) {
        memcpy(mappedData, &m_textureFlags, sizeof(TextureFlags));
        m_textureFlagsConstantBuffer->Unmap(0, nullptr);
    }
}

bool MaterialResource::CreateConstantBuffer()
{
    auto& engine = Engine::Instance();
    auto device = engine.GetDevice();
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    // 머티리얼 속성 상수 버퍼 생성
    const UINT materialBufferSize = (sizeof(MaterialProperties) + 255) & ~255;
    auto materialResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBufferSize);

    if (FAILED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &materialResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_materialConstantBuffer)))) {
        return false;
    }

    m_materialConstantBufferGPUAddress = m_materialConstantBuffer->GetGPUVirtualAddress();

    // 텍스처 플래그 상수 버퍼 생성
    const UINT flagsBufferSize = (sizeof(TextureFlags) + 255) & ~255;
    auto flagsResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(flagsBufferSize);

    if (FAILED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &flagsResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_textureFlagsConstantBuffer)))) {
        return false;
    }

    m_textureFlagsConstantBufferGPUAddress = m_textureFlagsConstantBuffer->GetGPUVirtualAddress();

    UpdateTextureFlagsBuffer();
    return true;
}
