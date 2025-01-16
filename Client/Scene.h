#pragma once
#include "GameObject.h"

class Scene {
public:
    Scene(const std::string& name);
    virtual ~Scene();

    // 씬 생명주기 메서드
    virtual void Initialize();
    virtual void Update(float deltaTime);
    virtual void Render(ID3D12GraphicsCommandList* commandList);
    virtual void Destroy();

    // 게임 오브젝트 관리
    GameObject* CreateGameObject(const std::string& name = "GameObject");
    void DestroyGameObject(GameObject* gameObject);
    GameObject* FindGameObject(const std::string& name);

    template<typename T>
    T* FindObjectOfType() {
        for (const auto& gameObject : m_gameObjects) {
            if (T* component = gameObject->GetComponent<T>()) {
                return component;
            }
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> FindObjectsOfType() {
        std::vector<T*> result;
        for (const auto& gameObject : m_gameObjects) {
            if (T* component = gameObject->GetComponent<T>()) {
                result.push_back(component);
            }
        }
        return result;
    }

    // 씬 정보 접근자
    const std::string& GetName() const { return m_name; }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active) { m_isActive = active; }
    size_t GetGameObjectCount() const { return m_gameObjects.size(); }

protected:
    std::string m_name;
    bool m_isActive = true;
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
    std::unordered_map<std::string, GameObject*> m_gameObjectMap;
};