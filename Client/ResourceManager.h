// ResourceManager.h
#pragma once
#include "IResource.h"
#include "LinearAllocator.h"
#include "Logger.h"

namespace Resource
{
    class ResourceManager {
    public:
        static ResourceManager& Instance();

        template<typename T>
        std::shared_ptr<T> Load(const std::string& path) {
            // 캐시된 리소스가 있는지 확인
            auto it = m_resources.find(path);
            if (it != m_resources.end()) {
                if (auto resource = it->second.lock()) {
                    if (auto typedResource = std::dynamic_pointer_cast<T>(resource)) {
                        Logger::Instance().Debug("리소스 재사용: {}", path);
                        return typedResource;
                    }
                }
                // weak_ptr가 만료되었다면 맵에서 제거
                m_resources.erase(it);
				Logger::Instance().Debug("만료된 리소스 제거: {}", path);
            }

            // 새 리소스 생성 및 로드
            auto resource = std::make_shared<T>();
            if (!resource->Load(path)) {
                Logger::Instance().Error("리소스 로드 실패: {}, 에러: {}",
                    path, resource->GetError());
                return nullptr;
            }

            m_resources[path] = resource;
            Logger::Instance().Info("새 리소스 로드: {}", path);
            return resource;
        }

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
        std::unordered_map<std::string, std::weak_ptr<IResource>> m_resources;
        Memory::LinearAllocator m_resourceAllocator;
    };
}