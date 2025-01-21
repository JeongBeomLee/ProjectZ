#include "pch.h"
#include "ResourceManager.h"

ResourceManager& ResourceManager::Instance()
{
    static ResourceManager instance;
    return instance;
}

std::shared_ptr<MeshResource> ResourceManager::CreateMesh(const std::string& name, const std::string& path)
{
    return CreateResource<MeshResource>(name, path, m_meshes);
}

std::shared_ptr<ShaderResource> ResourceManager::CreateShader(const std::string& name, const std::string& path, ShaderResource::ShaderType type)
{
    if (auto existingShader = GetShader(name)) {
        return existingShader;
    }

    auto shader = std::make_shared<ShaderResource>(name, path, type);
    if (!shader->Load()) {
        Logger::Instance().Error("셰이더 로드 실패: {}", path);
        return nullptr;
    }

    m_shaders[name] = shader;
    Logger::Instance().Info("셰이더 생성됨: {}", name);
    return shader;
}

std::shared_ptr<MaterialResource> ResourceManager::CreateMaterial(const std::string& name, const std::string& path)
{
    return CreateResource<MaterialResource>(name, path, m_materials);
}

std::shared_ptr<MeshResource> ResourceManager::GetMesh(const std::string& name)
{
    return GetResource<MeshResource>(name, m_meshes);
}

std::shared_ptr<ShaderResource> ResourceManager::GetShader(const std::string& name)
{
    return GetResource<ShaderResource>(name, m_shaders);
}

std::shared_ptr<MaterialResource> ResourceManager::GetMaterial(const std::string& name)
{
    return GetResource<MaterialResource>(name, m_materials);
}

void ResourceManager::ReleaseResource(const std::string& name)
{
    bool released = false;

    if (m_meshes.erase(name) > 0) {
        released = true;
        Logger::Instance().Info("메시 리소스 해제됨: {}", name);
    }
    if (m_shaders.erase(name) > 0) {
        released = true;
        Logger::Instance().Info("셰이더 리소스 해제됨: {}", name);
    }
    if (m_materials.erase(name) > 0) {
        released = true;
        Logger::Instance().Info("머티리얼 리소스 해제됨: {}", name);
    }

    if (!released) {
        Logger::Instance().Warning("리소스를 찾을 수 없음: {}", name);
    }
}

void ResourceManager::ReleaseUnusedResources()
{
    auto removeUnused = [](auto& resourceMap) {
        for (auto it = resourceMap.begin(); it != resourceMap.end();) {
            if (it->second.use_count() == 1) {  // ResourceManager만 참조 중
                Logger::Instance().Info("미사용 리소스 해제됨: {}", it->first);
                it = resourceMap.erase(it);
            }
            else {
                ++it;
            }
        }
        };

    removeUnused(m_meshes);
    removeUnused(m_shaders);
    removeUnused(m_materials);
}

void ResourceManager::ReleaseAllResources()
{
    m_meshes.clear();
    m_shaders.clear();
    m_materials.clear();
    Logger::Instance().Info("모든 리소스가 해제됨");
}