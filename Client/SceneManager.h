#pragma once
#include "Scene.h"

class SceneManager {
public:
    static SceneManager& Instance();

    Scene* CreateScene(const std::string& name);
    void LoadScene(Scene* scene);
    void LoadScene(const std::string& name);
    Scene* GetCurrentScene() const { return m_currentScene; }
    Scene* GetScene(const std::string& name) const;
    void DestroyScene(Scene* scene);
    void DestroyScene(const std::string& name);

    void Update(float deltaTime);
    void Render(ID3D12GraphicsCommandList* commandList);

    void Clear();

private:
    SceneManager() = default;
    ~SceneManager() { Clear(); }

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

private:
    Scene* m_currentScene = nullptr;
    std::vector<std::unique_ptr<Scene>> m_scenes;
    std::unordered_map<std::string, Scene*> m_sceneMap;
};