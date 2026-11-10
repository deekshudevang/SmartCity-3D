#include "ui/VehicleGalleryPanel.h"
#include "smartcity/vehicles/VehicleLibrary.h"
#include <imgui.h>

namespace SmartCity::ui {

void VehicleGalleryPanel::Render(bool* pOpen) {
    if (pOpen && !*pOpen) return;

    ImGui::SetNextWindowSize(ImVec2(780, 560), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Vehicle Gallery & Asset Catalog", pOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    auto& lib = vehicles::VehicleLibrary::Get();
    auto& reg = lib.GetPresetRegistry();

    // 1. Header controls: Search & Rescan
    ImGui::TextDisabled("Search Catalog:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputTextWithHint("##SearchVehicles", "Search by name, model hash, file...", m_searchBuffer, sizeof(m_searchBuffer));

    ImGui::SameLine();
    if (ImGui::Button("Rescan Source JSONs")) {
        lib.RescanSourcePresets();
    }

    // 2. Filters
    ImGui::Separator();
    ImGui::TextDisabled("Category:");
    ImGui::SameLine();
    const char* categories[] = { "All Categories", "Supercars", "Sports", "Sedans", "SUVs", "Buses", "Emergency", "Motorcycles" };
    ImGui::SetNextItemWidth(150.0f);
    ImGui::Combo("##CategoryFilter", &m_categoryFilterIndex, categories, IM_ARRAYSIZE(categories));

    ImGui::SameLine();
    ImGui::TextDisabled("3D Asset:");
    ImGui::SameLine();
    const char* assetFilters[] = { "All Models", "3D Models Available", "Missing 3D Models" };
    ImGui::SetNextItemWidth(170.0f);
    ImGui::Combo("##AssetFilter", &m_assetStatusFilterIndex, assetFilters, IM_ARRAYSIZE(assetFilters));

    ImGui::SameLine();
    ImGui::TextDisabled("Validation:");
    ImGui::SameLine();
    const char* valFilters[] = { "All States", "Valid Only", "Warnings / Errors" };
    ImGui::SetNextItemWidth(130.0f);
    ImGui::Combo("##ValidationFilter", &m_validationFilterIndex, valFilters, IM_ARRAYSIZE(valFilters));

    // Construct filter criteria
    vehicles::PresetFilterCriteria crit;
    crit.searchQuery = m_searchBuffer;
    crit.assetStatusFilter = m_assetStatusFilterIndex;
    crit.validationFilter = m_validationFilterIndex;

    switch (m_categoryFilterIndex) {
        case 1: crit.category = vehicles::VehicleCategory::Supercar; break;
        case 2: crit.category = vehicles::VehicleCategory::Sports; break;
        case 3: crit.category = vehicles::VehicleCategory::Sedan; break;
        case 4: crit.category = vehicles::VehicleCategory::SUV; break;
        case 5: crit.category = vehicles::VehicleCategory::Bus; break;
        case 6: crit.category = vehicles::VehicleCategory::Emergency; break;
        case 7: crit.category = vehicles::VehicleCategory::Motorcycle; break;
        default: crit.category = vehicles::VehicleCategory::Unknown; break;
    }

    auto filtered = reg.QueryPresets(crit);

    // Summary Telemetry
    ImGui::Spacing();
    ImGui::Text("Catalog: %zu loaded | Displaying %zu matching filters | %zu with 3D Assets | %zu Missing 3D Models",
                reg.GetTotalCount(), filtered.size(), reg.GetAvailableModelCount(), reg.GetMissingModelCount());
    ImGui::Separator();

    // 3. Preset Table
    const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                       ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

    if (ImGui::BeginTable("VehicleGalleryTable", 6, tableFlags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Display Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Model Hash", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Source JSON", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("3D Asset Status", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableHeadersRow();

        const auto* selectedPreset = lib.GetSelectedPreset();

        for (const auto* p : filtered) {
            ImGui::TableNextRow();
            bool isSelected = (selectedPreset && selectedPreset->id == p->id);

            // Column 0: Display Name
            ImGui::TableNextColumn();
            if (ImGui::Selectable(p->displayName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                lib.SelectPreset(p->id);
            }

            // Column 1: Category
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(p->GetCategoryName().c_str());

            // Column 2: Model Hash
            ImGui::TableNextColumn();
            ImGui::Text("%u", p->modelHash);

            // Column 3: Source File
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(p->sourceFile.c_str());

            // Column 4: 3D Asset Status Pill
            ImGui::TableNextColumn();
            if (p->IsModelAvailable()) {
                ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "[AVAILABLE]");
            } else {
                ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.2f, 1.0f), "[MISSING_MODEL]");
            }

            // Column 5: Action Button
            ImGui::TableNextColumn();
            std::string btnId = "Select##" + p->id;
            if (ImGui::SmallButton(btnId.c_str())) {
                lib.SelectPreset(p->id);
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace SmartCity::ui

