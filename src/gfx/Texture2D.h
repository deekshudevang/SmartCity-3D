#pragma once

#include <string>
#include <cstdint>
#include <glad/glad.h>

namespace SmartCity::gfx {

enum class TextureWrap {
    Repeat = GL_REPEAT,
    MirroredRepeat = GL_MIRRORED_REPEAT,
    ClampToEdge = GL_CLAMP_TO_EDGE,
    ClampToBorder = GL_CLAMP_TO_BORDER
};

enum class TextureFilter {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR,
    LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR
};

class Texture2D {
public:
    Texture2D(const std::string& filepath,
              TextureWrap wrap = TextureWrap::Repeat,
              TextureFilter minFilter = TextureFilter::LinearMipmapLinear,
              TextureFilter magFilter = TextureFilter::Linear);
    
    // Create a procedural 1x1 or custom memory texture (e.g., fallback magenta or solid color)
    Texture2D(uint32_t width, uint32_t height, const uint8_t* rgbaData);

    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetRendererId() const { return m_rendererId; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool IsLoaded() const { return m_isLoaded; }

private:
    void CreateMagentaFallback();

    uint32_t m_rendererId;
    std::string m_filepath;
    uint32_t m_width;
    uint32_t m_height;
    int m_channels;
    bool m_isLoaded;
};

} // namespace SmartCity::gfx

