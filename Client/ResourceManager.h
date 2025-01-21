#pragma once
#include "Logger.h"
#include "IResource.h"
#include "MeshResource.h"
#include "ShaderResource.h"
#include "MaterialResource.h"

class ResourceManager {
public:
    static ResourceManager& Instance();

    // 府家胶 积己 棺 肺靛
    std::shared_ptr<MeshResource> CreateMesh(const std::string& name, const std::string& path);
    std::shared_ptr<ShaderResource> CreateShader(const std::string& name, const std::string& path, ShaderResource::ShaderType type);
    std::shared_ptr<MaterialResource> CreateMaterial(const std::string& name, const std::string& path);

    // 府家胶 八祸
    std::shared_ptr<MeshResource> GetMesh(const std::string& name);
    std::shared_ptr<ShaderResource> GetShader(const std::string& name);
    std::shared_ptr<MaterialResource> GetMaterial(const std::string& name);

    // 府家胶 包府
    void ReleaseResource(const std::string& name);
    void ReleaseUnusedResources();
    void ReleaseAllResources();

private:
    ResourceManager() = default;
    ~ResourceManager() { ReleaseAllResources(); }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    template<typename T>
    std::shared_ptr<T> GetResource(const std::string& name,
        const std::unordered_map<std::string, std::shared_ptr<IResource>>& resourceMap) {
        auto it = resourceMap.find(name);
        if (it != resourceMap.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    template<typename T>
    std::shared_ptr<T> CreateResource(const std::string& name, const std::string& path,
        std::unordered_map<std::string, std::shared_ptr<IResource>>& resourceMap) {
        if (auto existingResource = GetResource<T>(name, resourceMap)) {
            return existingResource;
        }

        auto resource = std::make_shared<T>(name, path);
        if (!resource->Load()) {
            Logger::Instance().Error("府家胶 肺靛 角菩: {}", path);
            return nullptr;
        }

        resourceMap[name] = resource;
        Logger::Instance().Info("府家胶 积己凳: {}", name);
        return resource;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<IResource>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<IResource>> m_shaders;
    std::unordered_map<std::string, std::shared_ptr<IResource>> m_materials;
};