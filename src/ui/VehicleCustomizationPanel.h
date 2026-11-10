#pragma once

namespace SmartCity::ui {

class VehicleCustomizationPanel {
public:
    VehicleCustomizationPanel() = default;

    // Renders the interactive vehicle customization panel
    void Render(bool* pOpen = nullptr);
};

} // namespace SmartCity::ui

