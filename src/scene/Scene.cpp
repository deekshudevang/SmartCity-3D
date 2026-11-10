#include "scene/Scene.h"
#include "core/Logger.h"
#include <algorithm>

namespace SmartCity::scene {

Scene::Scene(const std::string& name)
    : m_name(name)
    , m_nextId(1)
{
}

Scene::~Scene() {
    Clear();
}

std::shared_ptr<SceneObject> Scene::CreateObject(const std::string& name) {
    auto object = std::make_shared<SceneObject>(m_nextId++, name);
    m_objects.push_back(object);
    return object;
}

void Scene::AddObject(std::shared_ptr<SceneObject> object) {
    if (object) {
        m_objects.push_back(object);
    }
}

bool Scene::RemoveObject(uint32_t id) {
    auto it = std::remove_if(m_objects.begin(), m_objects.end(), [id](const std::shared_ptr<SceneObject>& obj) {
        return obj && obj->GetId() == id;
    });

    if (it != m_objects.end()) {
        m_objects.erase(it, m_objects.end());
        return true;
    }
    return false;
}

std::shared_ptr<SceneObject> Scene::FindObjectById(uint32_t id) const {
    for (const auto& obj : m_objects) {
        if (obj && obj->GetId() == id) {
            return obj;
        }
    }
    return nullptr;
}

std::shared_ptr<SceneObject> Scene::FindObjectByName(const std::string& name) const {
    for (const auto& obj : m_objects) {
        if (obj && obj->GetName() == name) {
            return obj;
        }
    }
    return nullptr;
}

void Scene::Clear() {
    m_objects.clear();
}

} // namespace SmartCity::scene

