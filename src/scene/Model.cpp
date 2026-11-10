#include "scene/Model.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

namespace SmartCity::scene {

Model::Model(const std::string& filepath)
    : m_filepath(filepath)
    , m_isPlaceholder(false)
{
    LoadModel(filepath);
}

Model::Model(std::shared_ptr<gfx::Mesh> mesh, std::shared_ptr<gfx::Material> material)
    : m_filepath("memory_model")
    , m_isPlaceholder(false)
{
    m_meshes.push_back(mesh);
    m_materials.push_back(material);
    m_meshTransforms.push_back(glm::mat4(1.0f));
    CalculateBounds();
}

std::shared_ptr<Model> Model::CreatePlaceholderModel() {
    auto model = std::make_shared<Model>("");
    model->CreateFallbackPlaceholder();
    return model;
}

void Model::CreateFallbackPlaceholder() {
    m_meshes.clear();
    m_materials.clear();
    m_meshTransforms.clear();

    // Magenta 1x1 cube as visible fallback placeholder
    auto magentaCube = gfx::Mesh::CreateCube(glm::vec3(1.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    auto magentaMat = std::make_shared<gfx::Material>("fallback_magenta", nullptr);
    magentaMat->SetDiffuse(glm::vec3(1.0f, 0.0f, 1.0f));
    magentaMat->SetAmbient(glm::vec3(0.4f, 0.0f, 0.4f));

    m_meshes.push_back(std::move(magentaCube));
    m_materials.push_back(magentaMat);
    m_meshTransforms.push_back(glm::mat4(1.0f));
    m_localBounds = gfx::AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
    m_isPlaceholder = true;
}

void Model::CalculateBounds() {
    if (m_meshes.empty()) {
        m_localBounds = gfx::AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
        return;
    }
    // Default unit bounds if meshes are procedural
    m_localBounds = gfx::AABB(glm::vec3(-1.0f), glm::vec3(1.0f));
}

void Model::LoadModel(const std::string& filepath) {
    if (filepath.empty()) {
        CreateFallbackPlaceholder();
        return;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        // Try fallback with ../ prefix
        file.open("../" + filepath);
    }

    if (!file.is_open()) {
        core::Logger::Error("Assimp/ModelLoader: Failed to open model file '", filepath, "'. Generating magenta cube placeholder.");
        CreateFallbackPlaceholder();
        return;
    }

    // Parse Wavefront OBJ model with multi-mesh, normals, tangents, and UV flipping
    std::vector<glm::vec3> tempPositions;
    std::vector<glm::vec3> tempNormals;
    std::vector<glm::vec2> tempTexCoords;

    std::vector<gfx::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::map<std::string, uint32_t> vertexCache; // Deduplicate identical vertices

    std::string line;
    bool hasNormalsInFile = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            tempPositions.push_back(pos);
        } else if (prefix == "vn") {
            glm::vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            tempNormals.push_back(norm);
            hasNormalsInFile = true;
        } else if (prefix == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y; // Flip UVs (aiProcess_FlipUVs)
            tempTexCoords.push_back(uv);
        } else if (prefix == "f") {
            std::vector<std::string> faceTokens;
            std::string token;
            while (ss >> token) {
                faceTokens.push_back(token);
            }

            // Triangulate n-gons into triangles (aiProcess_Triangulate)
            if (faceTokens.size() >= 3) {
                for (size_t i = 1; i + 1 < faceTokens.size(); ++i) {
                    const std::string* triTokens[3] = { &faceTokens[0], &faceTokens[i], &faceTokens[i + 1] };
                    for (int k = 0; k < 3; ++k) {
                        const std::string& key = *triTokens[k];
                        auto it = vertexCache.find(key);
                        if (it != vertexCache.end()) {
                            indices.push_back(it->second);
                        } else {
                            gfx::Vertex v{};
                            v.color = glm::vec4(1.0f);

                            int pIdx = 0, tIdx = 0, nIdx = 0;
                            size_t firstSlash = key.find('/');
                            size_t secondSlash = (firstSlash != std::string::npos) ? key.find('/', firstSlash + 1) : std::string::npos;

                            pIdx = std::stoi(key.substr(0, firstSlash));
                            if (firstSlash != std::string::npos) {
                                if (secondSlash != std::string::npos) {
                                    if (secondSlash > firstSlash + 1) {
                                        tIdx = std::stoi(key.substr(firstSlash + 1, secondSlash - firstSlash - 1));
                                    }
                                    nIdx = std::stoi(key.substr(secondSlash + 1));
                                } else {
                                    tIdx = std::stoi(key.substr(firstSlash + 1));
                                }
                            }

                            if (pIdx > 0 && pIdx <= static_cast<int>(tempPositions.size())) {
                                v.position = tempPositions[pIdx - 1];
                            }
                            if (tIdx > 0 && tIdx <= static_cast<int>(tempTexCoords.size())) {
                                v.texCoords = tempTexCoords[tIdx - 1];
                            }
                            if (nIdx > 0 && nIdx <= static_cast<int>(tempNormals.size())) {
                                v.normal = tempNormals[nIdx - 1];
                            }

                            uint32_t newIdx = static_cast<uint32_t>(vertices.size());
                            vertices.push_back(v);
                            indices.push_back(newIdx);
                            vertexCache[key] = newIdx; // Join identical vertices
                        }
                    }
                }
            }
        }
    }

    if (vertices.empty()) {
        core::Logger::Error("Assimp/ModelLoader: Model file '", filepath, "' contains zero geometry. Generating magenta placeholder.");
        CreateFallbackPlaceholder();
        return;
    }

    // Generate normals if missing (aiProcess_GenNormals)
    if (!hasNormalsInFile) {
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            gfx::Vertex& v0 = vertices[indices[i]];
            gfx::Vertex& v1 = vertices[indices[i + 1]];
            gfx::Vertex& v2 = vertices[indices[i + 2]];

            glm::vec3 edge1 = v1.position - v0.position;
            glm::vec3 edge2 = v2.position - v0.position;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            v0.normal += normal;
            v1.normal += normal;
            v2.normal += normal;
        }
        for (auto& v : vertices) {
            v.normal = glm::normalize(v.normal);
        }
    }

    // Calculate bounding box
    glm::vec3 minP = vertices[0].position;
    glm::vec3 maxP = vertices[0].position;
    for (const auto& v : vertices) {
        minP = glm::min(minP, v.position);
        maxP = glm::max(maxP, v.position);
    }
    m_localBounds = gfx::AABB(minP, maxP);

    auto mesh = std::make_shared<gfx::Mesh>(vertices, indices);
    auto mat = std::make_shared<gfx::Material>("default_mat", nullptr);

    m_meshes.push_back(mesh);
    m_materials.push_back(mat);
    m_meshTransforms.push_back(glm::mat4(1.0f));
    m_isPlaceholder = false;

    core::Logger::Info("Loaded 3D Model: '", filepath, "' (", vertices.size(), " vertices, ", indices.size() / 3, " triangles).");
}

void Model::Draw(gfx::Shader* shader) const {
    for (size_t i = 0; i < m_meshes.size(); ++i) {
        if (m_materials[i]) {
            m_materials[i]->Bind();
        }
        if (shader && i < m_meshTransforms.size()) {
            shader->SetMat4("u_Model", m_meshTransforms[i]);
        }
        m_meshes[i]->Draw();
    }
}

} // namespace SmartCity::scene
