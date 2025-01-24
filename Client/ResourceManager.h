// ResourceManager.h
#pragma once
#include "IResource.h"
#include "TextureResource.h"
#include "ShaderResource.h"
#include "MaterialResource.h"
#include "LinearAllocator.h"
#include "Logger.h"

namespace Resource
{
    class ResourceManager {
    public:
        static ResourceManager& Instance();

        // 텍스처 로드
        std::shared_ptr<TextureResource> LoadTexture(const std::string& path);

        // 셰이더 로드
        std::shared_ptr<ShaderResource> LoadShader(const std::string& path, ShaderType type);

        // 머티리얼 로드
        std::shared_ptr<MaterialResource> LoadMaterial(const std::string& path);

        void PreloadResources(const std::string& manifestPath);
        void CleanupUnusedResources();
        size_t GetLoadedResourceCount() const;
        void PrintResourceStats() const;

    private:
        ResourceManager();
        ~ResourceManager();

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

    private:
        // 캐시키 생성 헬퍼 함수
        std::string CreateShaderCacheKey(const std::string& path, ShaderType type) {
            std::string strType;
            switch (type) {
            case ShaderType::Vertex:
                strType = "Vertex";
                break;
            case ShaderType::Pixel:
                strType = "Pixel";
                break;
            case ShaderType::Compute:
                strType = "Compute";
                break;
            case ShaderType::Geometry:
                strType = "Geometry";
                break;
            default:
				throw std::runtime_error("Unknown shader type");
            }
			return path + "_" + strType;
        }

    private:
        // 리소스 타입별 캐시
        std::unordered_map<std::string, std::weak_ptr<TextureResource>> m_textureCache;
        std::unordered_map<std::string, std::weak_ptr<ShaderResource>> m_shaderCache;
        std::unordered_map<std::string, std::weak_ptr<MaterialResource>> m_materialCache;
        Memory::LinearAllocator m_resourceAllocator;
    };
}