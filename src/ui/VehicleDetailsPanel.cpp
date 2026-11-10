#include "ui/VehicleDetailsPanel.h"
#include "smartcity/vehicles/VehicleLibrary.h"
#include <imgui.h>

namespace SmartCity::ui {

void VehicleDetailsPanel::Render(bool* pOpen) {
    if (pOpen && !*pOpen) return;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Vehicle Configuration Details", pOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    auto& lib = vehicles::VehicleLibrary::Get();
    const auto* preset = lib.GetSelectedPreset();

    if (!preset) {
        ImGui::TextDisabled("No vehicle preset selected. Select a vehicle from the Gallery.");
        ImGui::End();
        return;
    }

    // 1. Title & Identity
    ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "%s", preset->displayName.c_str());
    ImGui::TextDisabled("Model Hash: %u (ID: %s) | Category: %s",
                        preset->modelHash, preset->modelIdentifier.c_str(), preset->GetCategoryName().c_str());
    ImGui::TextDisabled("Source: %s [%s]", preset->sourceFile.c_str(), preset->sourceFormat.c_str());

    // 3D Model Asset Status Banner
    if (preset->IsModelAvailable()) {
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "Asset Status: AVAILABLE (%s)", preset->modelAssetPath.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.2f, 1.0f), "Asset Status: MISSING_MODEL (No compatible 3D asset assigned)");
    }
    ImGui::Separator();

    // 2. Tabs for configuration inspection
    if (ImGui::BeginTabBar("VehicleDetailsTabs")) {
        // Tab A: Paint & Appearance
        if (ImGui::BeginTabItem("Paint & Appearance")) {
            glm::vec3 priRgb = preset->GetEffectivePrimaryColorRgb();
            glm::vec3 secRgb = preset->GetEffectiveSecondaryColorRgb();

            ImGui::Text("Primary Color ID: %d %s", preset->paint.primaryColor,
                        preset->paint.isPrimaryColorCustom ? "(Custom RGB)" : "");
            ImGui::SameLine();
            ImGui::ColorButton("##PriColorSwatch", ImVec4(priRgb.r, priRgb.g, priRgb.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));

            ImGui::Text("Secondary Color ID: %d %s", preset->paint.secondaryColor,
                        preset->paint.isSecondaryColorCustom ? "(Custom RGB)" : "");
            ImGui::SameLine();
            ImGui::ColorButton("##SecColorSwatch", ImVec4(secRgb.r, secRgb.g, secRgb.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));

            ImGui::Text("Pearlescent Color ID: %d", preset->paint.pearlescentColor);
            ImGui::Text("Window Tint Level: %d", preset->paint.windowTint);
            ImGui::Text("Interior Color ID: %d | Dashboard Color ID: %d",
                        preset->paint.interiorColor, preset->paint.dashBoardColor);
            ImGui::EndTabItem();
        }

        // Tab B: Wheels & Neons
        if (ImGui::BeginTabItem("Wheels & Neons")) {
            ImGui::Text("Wheel Type: %d | Wheel Color ID: %d", preset->wheels.wheelType, preset->wheels.wheelColor);
            ImGui::Text("Bulletproof Tyres: %s | Drift Tyres: %s",
                        preset->wheels.bulletproofTyres ? "YES" : "NO",
                        preset->wheels.driftTyres ? "YES" : "NO");

            glm::vec3 smokeCol = glm::vec3(preset->wheels.tyreSmokeColor) / 255.0f;
            ImGui::Text("Tyre Smoke Color: (%d, %d, %d)",
                        preset->wheels.tyreSmokeColor.r, preset->wheels.tyreSmokeColor.g, preset->wheels.tyreSmokeColor.b);
            ImGui::SameLine();
            ImGui::ColorButton("##SmokeSwatch", ImVec4(smokeCol.r, smokeCol.g, smokeCol.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(18, 18));

            ImGui::Separator();
            ImGui::Text("Neon Underglow: %s", preset->neon.hasNeon ? "EQUIPPED" : "NONE");
            if (preset->neon.hasNeon) {
                ImGui::BulletText("Left: %s | Right: %s | Front: %s | Rear: %s",
                                  preset->neon.left ? "ON" : "OFF", preset->neon.right ? "ON" : "OFF",
                                  preset->neon.front ? "ON" : "OFF", preset->neon.back ? "ON" : "OFF");
                glm::vec3 neonCol = glm::vec3(preset->neon.color) / 255.0f;
                ImGui::Text("Neon Color RGB: (%d, %d, %d)", preset->neon.color.r, preset->neon.color.g, preset->neon.color.b);
                ImGui::SameLine();
                ImGui::ColorButton("##NeonSwatch", ImVec4(neonCol.r, neonCol.g, neonCol.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(18, 18));
            }
            ImGui::EndTabItem();
        }

        // Tab C: Modifications
        if (ImGui::BeginTabItem("Modifications (0..49)")) {
            ImGui::Text("Equipped Modification Slots:");
            if (ImGui::BeginTable("ModsTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY, ImVec2(0, 220))) {
                ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Variation", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& [slot, mod] : preset->modifications) {
                    if (mod.present) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("mod%d", slot);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", mod.index);
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", mod.variation ? "true" : "false");
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // Tab D: Extras & Plate
        if (ImGui::BeginTabItem("Extras & Plate")) {
            ImGui::Text("Number Plate Text: '%s' (Style Index: %d)", preset->plate.text.c_str(), preset->plate.styleIndex);
            ImGui::Separator();
            ImGui::Text("Extras (0..15): %s", preset->hasExtras ? "Present" : "None specified");
            if (preset->hasExtras) {
                for (int i = 0; i < 16; ++i) {
                    auto it = preset->extras.find(i);
                    bool active = (it != preset->extras.end() && it->second);
                    if (active) {
                        ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "  [X] extra%d : ACTIVE", i);
                    } else {
                        ImGui::TextDisabled("  [ ] extra%d : inactive", i);
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // Tab E: Simulation Profile
        if (ImGui::BeginTabItem("Simulation Profile")) {
            const auto& sp = preset->simProfile;
            ImGui::Text("Collision Dimensions: %.2f m x %.2f m x %.2f m (L x W x H)",
                        sp.dimensions.x, sp.dimensions.y, sp.dimensions.z);
            ImGui::Text("Max Speed: %.1f m/s (%.1f km/h)", sp.maxSpeed, sp.maxSpeed * 3.6f);
            ImGui::Text("Max Acceleration: %.2f m/s^2", sp.maxAcceleration);
            ImGui::Text("Comfortable Deceleration: %.2f m/s^2", sp.comfortableDecel);
            ImGui::Text("Max Emergency Braking: %.2f m/s^2", sp.maxDeceleration);
            ImGui::Text("IDM Min Gap: %.2f m", sp.minGap);
            ImGui::EndTabItem();
        }

        // Tab F: Extended & Raw JSON
        if (ImGui::BeginTabItem("Raw JSON & Extensions")) {
            if (!preset->unknownProperties.empty()) {
                ImGui::TextColored(ImVec4(0.85f, 0.7f, 0.2f, 1.0f), "Extended / Non-standard Properties:");
                std::string extDump = preset->unknownProperties.dump(2);
                ImGui::InputTextMultiline("##ExtendedJson", const_cast<char*>(extDump.c_str()), extDump.size(),
                                          ImVec2(-1.0f, 140.0f), ImGuiInputTextFlags_ReadOnly);
            }
            ImGui::Spacing();
            ImGui::Text("Complete Source JSON:");
            std::string rawDump = preset->rawSourceJson.dump(2);
            ImGui::InputTextMultiline("##RawSourceJson", const_cast<char*>(rawDump.c_str()), rawDump.size(),
                                      ImVec2(-1.0f, 200.0f), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace SmartCity::ui

