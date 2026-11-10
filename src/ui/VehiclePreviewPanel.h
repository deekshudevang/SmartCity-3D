#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include "gfx/Shader.h"
#include <memory>

namespace SmartCity::sim {
class VehicleManager;
}

namespace SmartCity::ui {

class VehiclePreviewPanel {
public:
    VehiclePreviewPanel();
    ~VehiclePreviewPanel();

    // Renders the vehicle 3D preview window with ImGui controls and embedded OpenGL FBO
    void Render(sim::VehicleManager* simManager,
                gfx::Shader* sceneShader,
                gfx::Shader* emissiveShader,
                bool* pOpen = nullptr);

private:
    void InitFBO(int width, int height);
    void RenderOffscreenFBO(const vehicles::VehiclePreset& preset,
                            gfx::Shader* sceneShader,
                            gfx::Shader* emissiveShader);

    unsigned int m_fbo{ 0 };
    unsigned int m_colorTex{ 0 };
    unsigned int m_depthRbo{ 0 };
    int m_fboWidth{ 512 };
    int m_fboHeight{ 384 };

    // Interactive camera & turntable state
    float m_turntableAngle{ 0.0f };
    float m_cameraElevation{ 0.35f };
    float m_cameraDistance{ 7.5f };
    bool m_autoRotate{ true };

    float m_wheelRotation{ 0.0f };
    float m_steeringAngle{ 0.0f };

    int m_spawnApproachIndex{ 0 }; // 0=North, 1=South, 2=East, 3=West
    int m_spawnMovementIndex{ 1 }; // 0=Left, 1=Through, 2=Right
};

} // namespace SmartCity::ui

