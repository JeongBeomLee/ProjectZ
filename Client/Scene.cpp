#include "pch.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "Engine.h"
#include "Camera.h"
#include "Logger.h"

Scene::Scene(const std::string& name)
    : m_name(name)
{
    Logger::Instance().Info("Scene '{}' 생성됨", m_name);
}

Scene::~Scene() 
{
    Destroy();
    Logger::Instance().Info("Scene '{}' 소멸됨", m_name);
}

void Scene::Initialize() 
{
    Logger::Instance().Info("Scene '{}' 초기화 시작", m_name);

    for (const auto& gameObject : m_gameObjects) {
        gameObject->Initialize();
    }
}

void Scene::Update(float deltaTime) 
{
    if (!m_isActive) return;

    for (const auto& gameObject : m_gameObjects) {
        gameObject->Update(deltaTime);
    }
}

void Scene::Render(ID3D12GraphicsCommandList* commandList) 
{
    if (!m_isActive) return;

    // 현재 카메라 가져오기
    auto camera = Engine::Instance().GetMainCamera();
    if (!camera) return;

    for (const auto& gameObject : m_gameObjects) {
        if (auto renderer = gameObject->GetComponent<MeshRenderer>()) {
            // 월드 공간에서의 바운딩 스피어 중심점 계산
            XMFLOAT3 worldCenter;
            XMStoreFloat3(&worldCenter,
                XMVector3Transform(
                    XMLoadFloat3(&renderer->GetBoundingSphereCenter()),
                    gameObject->GetTransform()->GetWorldMatrix()
                )
            );

            // 프러스텀 컬링 검사
            if (camera->IsInFrustum(worldCenter, renderer->GetBoundingSphereRadius())) {
                renderer->Render(commandList);
            }
        }
    }
}

void Scene::Destroy() 
{
    Logger::Instance().Info("Scene '{}' 정리 시작", m_name);

    for (const auto& gameObject : m_gameObjects) {
        gameObject->Destroy();
    }
    m_gameObjects.clear();
    m_gameObjectMap.clear();
}

GameObject* Scene::CreateGameObject(const std::string& name) 
{
    auto gameObject = std::make_shared<GameObject>();
    auto rawPtr = gameObject.get();

    // 이름이 이미 존재하면 번호를 붙여서 고유한 이름 생성
    std::string uniqueName = name;
    int suffix = 1;
    while (m_gameObjectMap.find(uniqueName) != m_gameObjectMap.end()) {
        uniqueName = name + " (" + std::to_string(suffix++) + ")";
    }

    m_gameObjects.push_back(gameObject);
    m_gameObjectMap[uniqueName] = rawPtr;

    Logger::Instance().Debug("GameObject '{}' 생성됨", uniqueName);
    return rawPtr;
}

void Scene::DestroyGameObject(GameObject* gameObject) 
{
    if (!gameObject) return;

    // gameObjectMap에서 제거
    for (auto it = m_gameObjectMap.begin(); it != m_gameObjectMap.end(); ++it) {
        if (it->second == gameObject) {
            Logger::Instance().Debug("GameObject '{}' 제거됨", it->first);
            m_gameObjectMap.erase(it);
            break;
        }
    }

    // gameObjects 벡터에서 제거
    m_gameObjects.erase(
        std::remove_if(m_gameObjects.begin(), m_gameObjects.end(),
            [gameObject](const std::shared_ptr<GameObject>& obj) {
                return obj.get() == gameObject;
            }),
        m_gameObjects.end()
    );
}

GameObject* Scene::FindGameObject(const std::string& name) 
{
    auto it = m_gameObjectMap.find(name);
    if (it != m_gameObjectMap.end()) {
        return it->second;
    }
    return nullptr;
}