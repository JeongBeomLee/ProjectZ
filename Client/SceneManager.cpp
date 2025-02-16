#include "pch.h"
#include "SceneManager.h"
#include "Logger.h"

SceneManager& SceneManager::Instance() 
{
    static SceneManager instance;
    return instance;
}

Scene* SceneManager::CreateScene(const std::string& name) 
{
    // 이미 존재하는 씬인지 확인
    if (m_sceneMap.find(name) != m_sceneMap.end()) {
        Logger::Instance().Warning("이미 존재하는 씬 이름입니다: {}", name);
        return nullptr;
    }

    auto scene = std::make_unique<Scene>(name);
    auto scenePtr = scene.get();

    m_scenes.push_back(std::move(scene));
    m_sceneMap[name] = scenePtr;

    Logger::Instance().Info("새로운 씬이 생성되었습니다: {}", name);
    return scenePtr;
}

void SceneManager::LoadScene(Scene* scene) 
{
    if (!scene) {
        Logger::Instance().Error("유효하지 않은 씬입니다.");
        return;
    }

    if (m_currentScene == scene) {
        Logger::Instance().Warning("이미 로드된 씬입니다: {}", scene->GetName());
        return;
    }

    Logger::Instance().Info("씬 전환: {} -> {}",
        m_currentScene ? m_currentScene->GetName() : "없음",
        scene->GetName());

    // 현재 씬 비활성화
    if (m_currentScene) {
        m_currentScene->SetActive(false);
    }

    // 새로운 씬으로 전환
    m_currentScene = scene;
    m_currentScene->SetActive(true);
    m_currentScene->Initialize();
}

void SceneManager::LoadScene(const std::string& name) 
{
    Scene* scene = GetScene(name);
    if (scene) {
        LoadScene(scene);
    }
    else {
        Logger::Instance().Error("씬을 찾을 수 없습니다: {}", name);
    }
}

Scene* SceneManager::GetScene(const std::string& name) const 
{
    auto it = m_sceneMap.find(name);
    if (it != m_sceneMap.end()) {
        return it->second;
    }
    return nullptr;
}

void SceneManager::DestroyScene(Scene* scene) 
{
    if (!scene) return;

    // 현재 씬인 경우 nullptr로 설정
    if (m_currentScene == scene) {
        m_currentScene = nullptr;
    }

    // sceneMap에서 제거
    for (auto it = m_sceneMap.begin(); it != m_sceneMap.end(); ++it) {
        if (it->second == scene) {
            m_sceneMap.erase(it);
            break;
        }
    }

    // scenes 벡터에서 제거
    m_scenes.erase(
        std::remove_if(m_scenes.begin(), m_scenes.end(),
            [scene](const std::unique_ptr<Scene>& s) {
                return s.get() == scene;
            }),
        m_scenes.end()
    );

    Logger::Instance().Info("씬이 제거되었습니다: {}", scene->GetName());
}

void SceneManager::DestroyScene(const std::string& name) 
{
    Scene* scene = GetScene(name);
    if (scene) {
        DestroyScene(scene);
    }
}

void SceneManager::Update(float deltaTime) 
{
    if (m_currentScene) {
        m_currentScene->Update(deltaTime);
    }
}

void SceneManager::Render(ID3D12GraphicsCommandList* commandList) 
{
    if (m_currentScene) {
        m_currentScene->Render(commandList);
    }
}

void SceneManager::Clear() 
{
    m_currentScene = nullptr;
    m_scenes.clear();
    m_sceneMap.clear();
    Logger::Instance().Info("SceneManager가 초기화되었습니다.");
}