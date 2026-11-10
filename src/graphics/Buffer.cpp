#include "graphics/Buffer.h"
#include "core/Logger.h"

namespace SmartCity {

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:    return GL_FLOAT;
        case ShaderDataType::Float2:   return GL_FLOAT;
        case ShaderDataType::Float3:   return GL_FLOAT;
        case ShaderDataType::Float4:   return GL_FLOAT;
        case ShaderDataType::Mat3:     return GL_FLOAT;
        case ShaderDataType::Mat4:     return GL_FLOAT;
        case ShaderDataType::Int:      return GL_INT;
        case ShaderDataType::Int2:     return GL_INT;
        case ShaderDataType::Int3:     return GL_INT;
        case ShaderDataType::Int4:     return GL_INT;
        case ShaderDataType::Bool:     return GL_BOOL;
        case ShaderDataType::None:     return 0;
    }
    return 0;
}

// ---------------- VertexBuffer ----------------
VertexBuffer::VertexBuffer(const void* vertices, uint32_t size)
    : m_rendererId(0)
{
    glGenBuffers(1, &m_rendererId);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererId);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    if (m_rendererId != 0) {
        glDeleteBuffers(1, &m_rendererId);
        m_rendererId = 0;
    }
}

void VertexBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererId);
}

void VertexBuffer::Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------------- IndexBuffer ----------------
IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count)
    : m_rendererId(0), m_count(count)
{
    glGenBuffers(1, &m_rendererId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    if (m_rendererId != 0) {
        glDeleteBuffers(1, &m_rendererId);
        m_rendererId = 0;
    }
}

void IndexBuffer::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
}

void IndexBuffer::Unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ---------------- VertexArray ----------------
VertexArray::VertexArray()
    : m_rendererId(0), m_vertexBufferIndex(0), m_indexBuffer(nullptr)
{
    glGenVertexArrays(1, &m_rendererId);
}

VertexArray::~VertexArray() {
    if (m_rendererId != 0) {
        glDeleteVertexArrays(1, &m_rendererId);
        m_rendererId = 0;
    }
}

void VertexArray::Bind() const {
    glBindVertexArray(m_rendererId);
}

void VertexArray::Unbind() const {
    glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const VertexBuffer* vertexBuffer) {
    if (!vertexBuffer) return;

    Bind();
    vertexBuffer->Bind();

    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout) {
        glEnableVertexAttribArray(m_vertexBufferIndex);
        glVertexAttribPointer(
            m_vertexBufferIndex,
            element.GetComponentCount(),
            ShaderDataTypeToOpenGLBaseType(element.type),
            element.normalized ? GL_TRUE : GL_FALSE,
            layout.GetStride(),
            reinterpret_cast<const void*>(element.offset)
        );
        m_vertexBufferIndex++;
    }
}

void VertexArray::SetIndexBuffer(const IndexBuffer* indexBuffer) {
    Bind();
    if (indexBuffer) {
        indexBuffer->Bind();
    }
    m_indexBuffer = indexBuffer;
}

} // namespace SmartCity
