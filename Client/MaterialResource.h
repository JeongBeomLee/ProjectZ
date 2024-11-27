#pragma once
#include "Resource.h"
#include "TextureResource.h"
#include "ResourceHandle.h"
struct MaterialProperties {
    XMFLOAT4 baseColor;
    float roughness;
    float metallic;
    XMFLOAT2 textureScale;
    std::string albedoTexture;
    std::string normalTexture;
    std::string roughnessTexture;
    std::string metallicTexture;
};

class MaterialResource : public Resource {
public:
    explicit MaterialResource(const std::string& name)
        : Resource(name) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Material; }
    size_t GetMemoryUsage() const override;

    const MaterialProperties& GetProperties() const { return m_properties; }
    void SetProperties(const MaterialProperties& props) { m_properties = props; }

private:
    MaterialProperties m_properties;
    std::vector<ResourceHandle<TextureResource>> m_textures;
};