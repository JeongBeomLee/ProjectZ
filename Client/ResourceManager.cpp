// ResourceManager.cpp
#include "pch.h"
#include "ResourceManager.h"

namespace Resource
{
    ResourceManager::ResourceManager()
        : m_resourceAllocator(32 * 1024 * 1024, "ResourceAllocator")
    {
        Logger::Instance().Info("리소스 매니저 초기화됨");
    }

    ResourceManager::~ResourceManager()
    {
        CleanupUnusedResources();
        Logger::Instance().Info("리소스 매니저 종료됨");
    }

    ResourceManager& ResourceManager::Instance()
    {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::CleanupUnusedResources()
    {
        size_t unloadedCount = 0;

        auto textureIt = m_textureCache.begin();
        while (textureIt != m_textureCache.end()) {
            if (textureIt->second.expired()) {
                Logger::Instance().Debug("만료된 텍스처 제거: {}", textureIt->first);
                textureIt = m_textureCache.erase(textureIt);
                unloadedCount++;
            }
            else {
                ++textureIt;
            }
        }

        auto shaderIt = m_shaderCache.begin();
        while (shaderIt != m_shaderCache.end()) {
            if (shaderIt->second.expired()) {
                Logger::Instance().Debug("만료된 셰이더 제거: {}", shaderIt->first);
                shaderIt = m_shaderCache.erase(shaderIt);
                unloadedCount++;
            }
            else {
                ++shaderIt;
            }
        }

        Logger::Instance().Info("리소스 정리 완료. 제거된 리소스: {}", unloadedCount);
    }

    size_t ResourceManager::GetLoadedResourceCount() const
    {
        size_t count = 0;

        for (const auto& [path, weakResource] : m_textureCache) {
            if (!weakResource.expired()) {
                count++;
            }
        }

        for (const auto& [path, weakResource] : m_shaderCache) {
            if (!weakResource.expired()) {
                count++;
            }
        }

        return count;
    }

    void ResourceManager::PrintResourceStats() const
    {
        size_t activeCount = GetLoadedResourceCount();
		size_t totalCount = m_textureCache.size() + m_shaderCache.size();

        Logger::Instance().Info("=== 리소스 통계 ===");
        Logger::Instance().Info("활성 리소스: {}", activeCount);
        Logger::Instance().Info("캐시된 총 리소스: {}", totalCount);
        Logger::Instance().Info("메모리 사용량: {}/{} 바이트",
            m_resourceAllocator.GetUsedMemory(),
            m_resourceAllocator.GetTotalMemory());

        // TODO: 타입별 리소스 카운트 출력은 나중에 구현
    }

    std::shared_ptr<TextureResource> ResourceManager::LoadTexture(const std::string& path)
    {
        // 캐시된 텍스처가 있는지 확인
        auto it = m_textureCache.find(path);
        if (it != m_textureCache.end()) {
            if (auto texture = it->second.lock()) {
                Logger::Instance().Debug("텍스처 재사용: {}", path);
                return texture;
            }
            m_textureCache.erase(it);
            Logger::Instance().Debug("만료된 텍스처 제거: {}", path);
        }

        // 새 텍스처 생성 및 로드
        auto texture = std::make_shared<TextureResource>();
        if (!texture->Load(path)) {
			Logger::Instance().Error("텍스처 로드 실패: {}, 에러: {}",
				path, texture->GetError());
            return nullptr;
        }

        m_textureCache[path] = texture;
        Logger::Instance().Info("새 텍스처 로드: {}", path);
        return texture;
    }

    std::shared_ptr<ShaderResource> ResourceManager::LoadShader(const std::string& path, ShaderType type)
    {
        std::string cacheKey = CreateShaderCacheKey(path, type);

        // 캐시된 셰이더가 있는지 확인
        auto it = m_shaderCache.find(cacheKey);
        if (it != m_shaderCache.end()) {
            if (auto shader = it->second.lock()) {
                Logger::Instance().Debug("셰이더 재사용: {}", path);
                return shader;
            }

            Logger::Instance().Debug("만료된 셰이더 제거: {}", path);
            m_shaderCache.erase(it);
        }

        // 새 셰이더 생성 및 로드
        auto shader = std::make_shared<ShaderResource>(type);
        if (!shader->Load(path)) {
			Logger::Instance().Error("셰이더 로드 실패: {}, 에러: {}",
				path, shader->GetError());
            return nullptr;
        }

        m_shaderCache[cacheKey] = shader;
        Logger::Instance().Info("새 셰이더 로드: {}", path);
        return shader;
    }

    std::shared_ptr<MaterialResource> ResourceManager::LoadMaterial(const std::string& path)
    {
        // 캐시된 머티리얼이 있는지 확인
        auto it = m_materialCache.find(path);
        if (it != m_materialCache.end()) {
            if (auto material = it->second.lock()) {
                Logger::Instance().Debug("머티리얼 재사용: {}", path);
                return material;
            }
            m_materialCache.erase(it);
        }

        // 새 머티리얼 생성 및 로드
        auto material = std::make_shared<MaterialResource>();
        if (!material->Load(path)) {
            Logger::Instance().Error("머티리얼 로드 실패: {}", path);
            return nullptr;
        }

        m_materialCache[path] = material;
        Logger::Instance().Info("새 머티리얼 로드: {}", path);
        return material;
    }

    void ResourceManager::PreloadResources(const std::string& manifestPath)
    {
        // TODO: Phase 5에서 구현 예정
        Logger::Instance().Info("리소스 프리로딩 시작: {}", manifestPath);
    }
}