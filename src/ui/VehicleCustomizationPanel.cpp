#include "ui/VehicleCustomizationPanel.h"
#include "smartcity/vehicles/VehicleLibrary.h"
#include <imgui.h>
#include <cstring>

namespace SmartCity::ui {

void VehicleCustomizationPanel::Render(bool* pOpen) {
    if (pOpen && !*pOpen) return;

    ImGui::SetNextWindowSize(ImVec2(480, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Vehicle Customization Editor", pOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    auto& lib = vehicles::VehicleLibrary::Get();
    auto* preset = lib.GetSelectedPresetMutable();

    if (!preset) {
        ImGui::TextDisabled("Select a vehicle preset from the Gallery to customize.");
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Customizing: %s", preset->displayName.c_str());
    ImGui::TextDisabled("Changes apply in real time to the 3D preview and newly spawned vehicles.");
    ImGui::Separator();

    // 1. Paint Customization
    if (ImGui::CollapsingHeader("Body Paint & Finish", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Primary Paint
        ImGui::Text("Primary Paint:");
        ImGui::Checkbox("Use Custom Primary RGB", &preset->paint.isPrimaryColorCustom);
        if (preset->paint.isPrimaryColorCustom) {
            float col[3] = {
                preset->paint.customPrimaryColor.r / 255.0f,
                preset->paint.customPrimaryColor.g / 255.0f,
                preset->paint.customPrimaryColor.b / 255.0f
            };
            if (ImGui::ColorEdit3("Primary Color", col)) {
                preset->paint.customPrimaryColor.r = static_cast<int>(col[0] * 255.0f);
                preset->paint.customPrimaryColor.g = static_cast<int>(col[1] * 255.0f);
                preset->paint.customPrimaryColor.b = static_cast<int>(col[2] * 255.0f);
            }
        } else {
            ImGui::SliderInt("Primary Palette ID (0..159)", &preset->paint.primaryColor, 0, 159);
        }

        ImGui::Spacing();
        // Secondary Paint
        ImGui::Text("Secondary Paint:");
        ImGui::Checkbox("Use Custom Secondary RGB", &preset->paint.isSecondaryColorCustom);
        if (preset->paint.isSecondaryColorCustom) {
            float col[3] = {
                preset->paint.customSecondaryColor.r / 255.0f,
                preset->paint.customSecondaryColor.g / 255.0f,
                preset->paint.customSecondaryColor.b / 255.0f
            };
            if (ImGui::ColorEdit3("Secondary Color", col)) {
                preset->paint.customSecondaryColor.r = static_cast<int>(col[0] * 255.0f);
                preset->paint.customSecondaryColor.g = static_cast<int>(col[1] * 255.0f);
                preset->paint.customSecondaryColor.b = static_cast<int>(col[2] * 255.0f);
            }
        } else {
            ImGui::SliderInt("Secondary Palette ID (0..159)", &preset->paint.secondaryColor, 0, 159);
        }

        ImGui::Spacing();
        ImGui::SliderInt("Pearlescent Finish ID (0..159)", &preset->paint.pearlescentColor, 0, 159);
        ImGui::SliderInt("Window Tint Darkness (0..6)", &preset->paint.windowTint, 0, 6);
    }

    // 2. Neon Underglow
    if (ImGui::CollapsingHeader("Neon Underglow Kits", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Neon Kits", &preset->neon.hasNeon);
        if (preset->neon.hasNeon) {
            ImGui::Checkbox("Left Neon", &preset->neon.left);
            ImGui::SameLine();
            ImGui::Checkbox("Right Neon", &preset->neon.right);
            ImGui::SameLine();
            ImGui::Checkbox("Front Neon", &preset->neon.front);
            ImGui::SameLine();
            ImGui::Checkbox("Back Neon", &preset->neon.back);

            float nCol[3] = {
                preset->neon.color.r / 255.0f,
                preset->neon.color.g / 255.0f,
                preset->neon.color.b / 255.0f
            };
            if (ImGui::ColorEdit3("Neon Glow Color", nCol)) {
                preset->neon.color.r = static_cast<int>(nCol[0] * 255.0f);
                preset->neon.color.g = static_cast<int>(nCol[1] * 255.0f);
                preset->neon.color.b = static_cast<int>(nCol[2] * 255.0f);
            }
        }
    }

    // 3. Wheels & Tyres
    if (ImGui::CollapsingHeader("Wheels & Tyres")) {
        ImGui::SliderInt("Wheel Type Index", &preset->wheels.wheelType, 0, 12);
        ImGui::SliderInt("Wheel Color ID (0..159)", &preset->wheels.wheelColor, 0, 159);
        ImGui::Checkbox("Bulletproof Tyres", &preset->wheels.bulletproofTyres);
        ImGui::SameLine();
        ImGui::Checkbox("Low Grip / Drift Tyres", &preset->wheels.driftTyres);

        float sCol[3] = {
            preset->wheels.tyreSmokeColor.r / 255.0f,
            preset->wheels.tyreSmokeColor.g / 255.0f,
            preset->wheels.tyreSmokeColor.b / 255.0f
        };
        if (ImGui::ColorEdit3("Tyre Smoke Color", sCol)) {
            preset->wheels.tyreSmokeColor.r = static_cast<int>(sCol[0] * 255.0f);
            preset->wheels.tyreSmokeColor.g = static_cast<int>(sCol[1] * 255.0f);
            preset->wheels.tyreSmokeColor.b = static_cast<int>(sCol[2] * 255.0f);
        }
    }

    // 4. Number Plate
    if (ImGui::CollapsingHeader("License Plate")) {
        char plateBuf[32];
        std::strncpy(plateBuf, preset->plate.text.c_str(), sizeof(plateBuf) - 1);
        plateBuf[sizeof(plateBuf) - 1] = '\0';

        if (ImGui::InputText("Plate Text", plateBuf, sizeof(plateBuf))) {
            preset->plate.text = plateBuf;
        }
        ImGui::SliderInt("Plate Background Style", &preset->plate.styleIndex, 0, 6);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset All to Source JSON Values", ImVec2(-1.0f, 28.0f))) {
        // Re-parse original JSON
        if (!preset->rawSourceJson.empty()) {
            vehicles::VehiclePresetImporter imp;
            auto res = imp.ImportFromString(preset->rawSourceJson.dump(), preset->sourceFile);
            if (res.isSuccess) {
                *preset = res.preset;
                lib.GetAssetRegistry().ResolvePresetAssets(*preset);
            }
        }
    }

    ImGui::End();
}

} // namespace SmartCity::ui

