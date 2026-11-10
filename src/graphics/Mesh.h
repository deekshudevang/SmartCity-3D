#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "graphics/Buffer.h"

namespace SmartCity {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec4 color;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh() = default;

    void Draw() const;

    uint32_t GetIndexCount() const { return m_indexCount; }
    uint32_t GetVertexCount() const { return m_vertexCount; }

    // Procedural geometry factories
    static std::unique_ptr<Mesh> CreateCube(const glm::vec3& size = glm::vec3(1.0f), const glm::vec4& color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> CreatePlane(float width, float height, const glm::vec4& color = glm::vec4(1.0f), float uvTileX = 1.0f, float uvTileY = 1.0f);
    static std::unique_ptr<Mesh> CreateCylinder(float radius, float height, int segments = 24, const glm::vec4& color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> CreateSphere(float radius, int rings = 16, int sectors = 24, const glm::vec4& color = glm::vec4(1.0f));

private:
    std::unique_ptr<VertexArray> m_vertexArray;
    std::unique_ptr<VertexBuffer> m_vertexBuffer;
    std::unique_ptr<IndexBuffer> m_indexBuffer;
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
};

} // namespace SmartCity
