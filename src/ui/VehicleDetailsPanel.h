#pragma once

namespace SmartCity::ui {

class VehicleDetailsPanel {
public:
    VehicleDetailsPanel() = default;

    // Renders the vehicle details ImGui window/panel
    void Render(bool* pOpen = nullptr);
};

} // namespace SmartCity::ui

