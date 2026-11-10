#pragma once

#include "smartcity/vehicles/VehiclePresetRegistry.h"
#include <string>

namespace SmartCity::ui {

class VehicleGalleryPanel {
public:
    VehicleGalleryPanel() = default;

    // Renders the vehicle gallery ImGui window/panel
    void Render(bool* pOpen = nullptr);

private:
    char m_searchBuffer[128]{ 0 };
    int m_categoryFilterIndex{ 0 };     // 0 = All, 1 = Supercar, 2 = Sports, 3 = Sedan, 4 = SUV, 5 = Bus, 6 = Emergency, 7 = Motorcycle
    int m_assetStatusFilterIndex{ 0 };  // 0 = All, 1 = Available 3D Models, 2 = Missing 3D Models
    int m_validationFilterIndex{ 0 };   // 0 = All, 1 = Valid Only, 2 = Invalid/Warning Only
};

} // namespace SmartCity::ui

