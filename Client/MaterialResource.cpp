#include "pch.h"
#include "MaterialResource.h"
#include "Logger.h"
#include "ResourceManager.h"

bool MaterialResource::Load()
{
    try {
        m_state = ResourceState::Loading;

        // JSON 파일에서 재질 속성 로드
        std::ifstream file(m_name);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open material file");
        }

        json materialData;
        file >> materialData;

        // 기본 속성 로드
        m_properties.baseColor = {
            materialData["baseColor"]["r"],
            materialData["baseColor"]["g"],
            materialData["baseColor"]["b"],
            materialData["baseColor"]["a"]
        };

        m_properties.roughness = materialData["roughness"];
        m_properties.metallic = materialData["metallic"];
        m_properties.textureScale = {
            materialData["textureScale"]["x"],
            materialData["textureScale"]["y"]
        };

        // albedo 텍스처 로드
        if (materialData.contains("albedoTexture")) {
            m_properties.albedoTexture = materialData["albedoTexture"];
            auto texture = ResourceManager::GetInstance().LoadResource<TextureResource>(
                m_properties.albedoTexture);
            m_textures.push_back(texture);
        }

		// normal 텍스처 로드
		if (materialData.contains("normalTexture")) {
			m_properties.normalTexture = materialData["normalTexture"];
			auto texture = ResourceManager::GetInstance().LoadResource<TextureResource>(
				m_properties.normalTexture);
			m_textures.push_back(texture);
		}

		// roughness 텍스처 로드
		if (materialData.contains("roughnessTexture")) {
			m_properties.roughnessTexture = materialData["roughnessTexture"];
			auto texture = ResourceManager::GetInstance().LoadResource<TextureResource>(
				m_properties.roughnessTexture);
			m_textures.push_back(texture);
		}

		// metallic 텍스처 로드
		if (materialData.contains("metallicTexture")) {
			m_properties.metallicTexture = materialData["metallicTexture"];
			auto texture = ResourceManager::GetInstance().LoadResource<TextureResource>(
				m_properties.metallicTexture);
			m_textures.push_back(texture);
		}

        m_state = ResourceState::Ready;
        LOG_INFO("Material loaded: {}", m_name);
        return true;
    }
    catch (const std::exception& e) {
        m_state = ResourceState::Failed;
        m_errorMessage = e.what();
        LOG_ERROR("Failed to load material {}: {}", m_name, e.what());
        return false;
    }
}

void MaterialResource::Unload()
{
}

size_t MaterialResource::GetMemoryUsage() const
{
    return size_t();
}
