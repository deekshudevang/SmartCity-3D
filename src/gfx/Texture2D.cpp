#include "gfx/Texture2D.h"
#include "core/Logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace SmartCity::gfx {

Texture2D::Texture2D(const std::string& filepath, TextureWrap wrap, TextureFilter minFilter, TextureFilter magFilter)
    : m_rendererId(0)
    , m_filepath(filepath)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
    , m_isLoaded(false)
{
    stbi_set_flip_vertically_on_load(1);

    int w = 0, h = 0, ch = 0;
    stbi_uc* data = stbi_load(filepath.c_str(), &w, &h, &ch, 4);

    if (data) {
        m_width = static_cast<uint32_t>(w);
        m_height = static_cast<uint32_t>(h);
        m_channels = 4;
        m_isLoaded = true;

        glGenTextures(1, &m_rendererId);
        glBindTexture(GL_TEXTURE_2D, m_rendererId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        core::Logger::Info("Loaded Texture2D: ", filepath, " (", m_width, "x", m_height, ")");
    } else {
        core::Logger::Error("Failed to load texture at path '", filepath, "'. Generating magenta fallback texture.");
        CreateMagentaFallback();
    }
}

Texture2D::Texture2D(uint32_t width, uint32_t height, const uint8_t* rgbaData)
    : m_rendererId(0)
    , m_filepath("memory_texture")
    , m_width(width)
    , m_height(height)
    , m_channels(4)
    , m_isLoaded(true)
{
    glGenTextures(1, &m_rendererId);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
}

void Texture2D::CreateMagentaFallback() {
    m_width = 2;
    m_height = 2;
    m_channels = 4;
    m_isLoaded = false;

    // 2x2 magenta and black checkerboard
    uint8_t fallbackPixels[2 * 2 * 4] = {
        255, 0, 255, 255,     0,   0,   0, 255,
          0, 0,   0, 255,   255, 0, 255, 255
    };

    glGenTextures(1, &m_rendererId);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixels);
}

Texture2D::~Texture2D() {
    if (m_rendererId != 0) {
        glDeleteTextures(1, &m_rendererId);
        m_rendererId = 0;
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_rendererId(other.m_rendererId)
    , m_filepath(std::move(other.m_filepath))
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_channels(other.m_channels)
    , m_isLoaded(other.m_isLoaded)
{
    other.m_rendererId = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_channels = 0;
    other.m_isLoaded = false;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_rendererId != 0) {
            glDeleteTextures(1, &m_rendererId);
        }
        m_rendererId = other.m_rendererId;
        m_filepath = std::move(other.m_filepath);
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_isLoaded = other.m_isLoaded;

        other.m_rendererId = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_channels = 0;
        other.m_isLoaded = false;
    }
    return *this;
}

void Texture2D::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
}

void Texture2D::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace SmartCity::gfx

