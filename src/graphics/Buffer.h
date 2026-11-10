#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>

namespace SmartCity {

enum class ShaderDataType {
    None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
};

static uint32_t ShaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:    return 4;
        case ShaderDataType::Float2:   return 4 * 2;
        case ShaderDataType::Float3:   return 4 * 3;
        case ShaderDataType::Float4:   return 4 * 4;
        case ShaderDataType::Mat3:     return 4 * 3 * 3;
        case ShaderDataType::Mat4:     return 4 * 4 * 4;
        case ShaderDataType::Int:      return 4;
        case ShaderDataType::Int2:     return 4 * 2;
        case ShaderDataType::Int3:     return 4 * 3;
        case ShaderDataType::Int4:     return 4 * 4;
        case ShaderDataType::Bool:     return 1;
        case ShaderDataType::None:     return 0;
    }
    return 0;
}

struct BufferElement {
    std::string name;
    ShaderDataType type;
    uint32_t size;
    size_t offset;
    bool normalized;

    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
        : name(name), type(type), size(ShaderDataTypeSize(type)), offset(0), normalized(normalized) {}

    uint32_t GetComponentCount() const {
        switch (type) {
            case ShaderDataType::Float:   return 1;
            case ShaderDataType::Float2:  return 2;
            case ShaderDataType::Float3:  return 3;
            case ShaderDataType::Float4:  return 4;
            case ShaderDataType::Mat3:    return 3 * 3;
            case ShaderDataType::Mat4:    return 4 * 4;
            case ShaderDataType::Int:     return 1;
            case ShaderDataType::Int2:    return 2;
            case ShaderDataType::Int3:    return 3;
            case ShaderDataType::Int4:    return 4;
            case ShaderDataType::Bool:    return 1;
            case ShaderDataType::None:    return 0;
        }
        return 0;
    }
};

class BufferLayout {
public:
    BufferLayout() : m_stride(0) {}
    BufferLayout(std::initializer_list<BufferElement> elements)
        : m_elements(elements), m_stride(0) {
        CalculateOffsetsAndStride();
    }

    uint32_t GetStride() const { return m_stride; }
    const std::vector<BufferElement>& GetElements() const { return m_elements; }

    std::vector<BufferElement>::iterator begin() { return m_elements.begin(); }
    std::vector<BufferElement>::iterator end() { return m_elements.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return m_elements.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return m_elements.end(); }

private:
    void CalculateOffsetsAndStride() {
        size_t offset = 0;
        m_stride = 0;
        for (auto& element : m_elements) {
            element.offset = offset;
            offset += element.size;
            m_stride += element.size;
        }
    }

    std::vector<BufferElement> m_elements;
    uint32_t m_stride;
};

// RAII VertexBuffer
class VertexBuffer {
public:
    VertexBuffer(const void* vertices, uint32_t size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;

    const BufferLayout& GetLayout() const { return m_layout; }
    void SetLayout(const BufferLayout& layout) { m_layout = layout; }

private:
    uint32_t m_rendererId;
    BufferLayout m_layout;
};

// RAII IndexBuffer / ElementBuffer
class IndexBuffer {
public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
    void Unbind() const;

    uint32_t GetCount() const { return m_count; }

private:
    uint32_t m_rendererId;
    uint32_t m_count;
};

// RAII VertexArray
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(const VertexBuffer* vertexBuffer);
    void SetIndexBuffer(const IndexBuffer* indexBuffer);

    const IndexBuffer* GetIndexBuffer() const { return m_indexBuffer; }

private:
    uint32_t m_rendererId;
    uint32_t m_vertexBufferIndex;
    const IndexBuffer* m_indexBuffer;
};

} // namespace SmartCity
