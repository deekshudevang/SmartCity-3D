#include "graphics/Mesh.h"
#include <cmath>

namespace SmartCity {

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_vertexCount(static_cast<uint32_t>(vertices.size()))
    , m_indexCount(static_cast<uint32_t>(indices.size()))
{
    m_vertexArray = std::make_unique<VertexArray>();

    m_vertexBuffer = std::make_unique<VertexBuffer>(
        vertices.data(),
        static_cast<uint32_t>(vertices.size() * sizeof(Vertex))
    );

    m_vertexBuffer->SetLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float3, "a_Normal" },
        { ShaderDataType::Float2, "a_TexCoord" },
        { ShaderDataType::Float4, "a_Color" }
    });

    m_indexBuffer = std::make_unique<IndexBuffer>(
        indices.data(),
        static_cast<uint32_t>(indices.size())
    );

    m_vertexArray->AddVertexBuffer(m_vertexBuffer.get());
    m_vertexArray->SetIndexBuffer(m_indexBuffer.get());
}

void Mesh::Draw() const {
    if (m_vertexArray) {
        m_vertexArray->Bind();
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        m_vertexArray->Unbind();
    }
}

std::unique_ptr<Mesh> Mesh::CreateCube(const glm::vec3& size, const glm::vec4& color) {
    glm::vec3 h = size * 0.5f;

    std::vector<Vertex> vertices = {
        // Front face (+Z)
        { { -h.x, -h.y,  h.z }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }, color },
        { {  h.x, -h.y,  h.z }, { 0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f }, color },
        { {  h.x,  h.y,  h.z }, { 0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f }, color },
        { { -h.x,  h.y,  h.z }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }, color },

        // Back face (-Z)
        { {  h.x, -h.y, -h.z }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }, color },
        { { -h.x, -h.y, -h.z }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }, color },
        { { -h.x,  h.y, -h.z }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }, color },
        { {  h.x,  h.y, -h.z }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }, color },

        // Top face (+Y)
        { { -h.x,  h.y,  h.z }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  h.x,  h.y,  h.z }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  h.x,  h.y, -h.z }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -h.x,  h.y, -h.z }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Bottom face (-Y)
        { { -h.x, -h.y, -h.z }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  h.x, -h.y, -h.z }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  h.x, -h.y,  h.z }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -h.x, -h.y,  h.z }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Right face (+X)
        { {  h.x, -h.y,  h.z }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  h.x, -h.y, -h.z }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  h.x,  h.y, -h.z }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { {  h.x,  h.y,  h.z }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Left face (-X)
        { { -h.x, -h.y, -h.z }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { { -h.x, -h.y,  h.z }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { { -h.x,  h.y,  h.z }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -h.x,  h.y, -h.z }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color },
    };

    std::vector<uint32_t> indices = {
         0,  1,  2,  2,  3,  0, // Front
         4,  5,  6,  6,  7,  4, // Back
         8,  9, 10, 10, 11,  8, // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::CreatePlane(float width, float height, const glm::vec4& color, float uvTileX, float uvTileY) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    std::vector<Vertex> vertices = {
        { { -hw, 0.0f,  hh }, { 0.0f, 1.0f, 0.0f }, { 0.0f,    0.0f },    color },
        { {  hw, 0.0f,  hh }, { 0.0f, 1.0f, 0.0f }, { uvTileX, 0.0f },    color },
        { {  hw, 0.0f, -hh }, { 0.0f, 1.0f, 0.0f }, { uvTileX, uvTileY }, color },
        { { -hw, 0.0f, -hh }, { 0.0f, 1.0f, 0.0f }, { 0.0f,    uvTileY }, color },
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::CreateCylinder(float radius, float height, int segments, const glm::vec4& color) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float halfH = height * 0.5f;
    float angleStep = 2.0f * 3.14159265359f / static_cast<float>(segments);

    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        float u = static_cast<float>(i) / static_cast<float>(segments);

        glm::vec3 normal(std::cos(angle), 0.0f, std::sin(angle));

        // Bottom vertex
        vertices.push_back({ { x, -halfH, z }, normal, { u, 0.0f }, color });
        // Top vertex
        vertices.push_back({ { x,  halfH, z }, normal, { u, 1.0f }, color });
    }

    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = i * 2;
        uint32_t t0 = i * 2 + 1;
        uint32_t b1 = (i + 1) * 2;
        uint32_t t1 = (i + 1) * 2 + 1;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t1);

        indices.push_back(b0);
        indices.push_back(t1);
        indices.push_back(t0);
    }

    // Top cap center
    uint32_t topCenterIdx = static_cast<uint32_t>(vertices.size());
    vertices.push_back({ { 0.0f, halfH, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.5f }, color });
    for (int i = 0; i < segments; ++i) {
        uint32_t t0 = i * 2 + 1;
        uint32_t t1 = (i + 1) * 2 + 1;
        indices.push_back(topCenterIdx);
        indices.push_back(t0);
        indices.push_back(t1);
    }

    // Bottom cap center
    uint32_t bottomCenterIdx = static_cast<uint32_t>(vertices.size());
    vertices.push_back({ { 0.0f, -halfH, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.5f, 0.5f }, color });
    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = i * 2;
        uint32_t b1 = (i + 1) * 2;
        indices.push_back(bottomCenterIdx);
        indices.push_back(b1);
        indices.push_back(b0);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::CreateSphere(float radius, int rings, int sectors, const glm::vec4& color) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const float R = 1.0f / static_cast<float>(rings - 1);
    const float S = 1.0f / static_cast<float>(sectors - 1);
    const float PI = 3.14159265359f;

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float y = std::sin(-PI * 0.5f + PI * r * R);
            float x = std::cos(2 * PI * s * S) * std::sin(PI * r * R);
            float z = std::sin(2 * PI * s * S) * std::sin(PI * r * R);

            glm::vec3 pos = glm::vec3(x, y, z) * radius;
            glm::vec3 norm = glm::normalize(pos);
            glm::vec2 uv(s * S, r * R);

            vertices.push_back({ pos, norm, uv, color });
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            uint32_t cur = r * sectors + s;
            uint32_t next = cur + sectors;

            indices.push_back(cur);
            indices.push_back(cur + 1);
            indices.push_back(next);

            indices.push_back(cur + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace SmartCity
