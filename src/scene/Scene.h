#pragma once

#include <vector>
#include <memory>
#include <string>
#include "scene/SceneObject.h"

namespace SmartCity::scene {

class Scene {
public:
    Scene(const std::string& name = "MainScene");
    ~Scene();

    std::shared_ptr<SceneObject> CreateObject(const std::string& name);
    void AddObject(std::shared_ptr<SceneObject> object);
    bool RemoveObject(uint32_t id);
    std::shared_ptr<SceneObject> FindObjectById(uint32_t id) const;
    std::shared_ptr<SceneObject> FindObjectByName(const std::string& name) const;

    const std::vector<std::shared_ptr<SceneObject>>& GetObjects() const { return m_objects; }
    size_t GetObjectCount() const { return m_objects.size(); }

    void Clear();

    const std::string& GetName() const { return m_name; }

private:
    std::string m_name;
    uint32_t m_nextId;
    std::vector<std::shared_ptr<SceneObject>> m_objects;
};

} // namespace SmartCity::scene

