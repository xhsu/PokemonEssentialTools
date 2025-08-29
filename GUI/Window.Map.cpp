#include <imgui.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import Database.RX;
import GL.GameMap;
import Game.Map;
import Image.Query;
import Image.Tilesets;


static std::optional<CMap> s_MapOnDisplay = std::nullopt;

static void DrawTree(Database::RX::MapInfo const& info, int bitsFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes) noexcept
{
	char szDisplayName[256]{};
	std::snprintf(szDisplayName, sizeof(szDisplayName), "%s##%d", info.m_name.c_str(), (int)(std::intptr_t)(&info));

	if (info.m_rgpChildren.empty())
	{
		ImGui::TreeNodeEx(szDisplayName, bitsFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::IsItemClicked())
			s_MapOnDisplay.emplace(&info);
	}
	else
	{
		auto const bOpen = ImGui::TreeNodeEx(szDisplayName, bitsFlags | ImGuiTreeNodeFlags_OpenOnArrow);
		auto const bClicked = ImGui::IsItemClicked();

		// Must handle click before other potential child widgets are drawn.
		if (bClicked)
		{
			// Get the item rectangle
			auto const itemMin = ImGui::GetItemRectMin();
			auto const& mousePos = ImGui::GetIO().MousePos;

			// Approximate arrow area (typically on the left side)
			auto const arrowWidth = ImGui::GetFrameHeight(); // Arrow is roughly square

			if (mousePos.x - itemMin.x < arrowWidth)
			{
				// Does nothing when user trying to expand/collapse the tree node.
			}
			else
			{
				s_MapOnDisplay.emplace(&info);
			}
		}

		if (bOpen)
		{
			for (auto&& child : info.m_rgpChildren)
				DrawTree(*child, bitsFlags);

			ImGui::TreePop();
		}
	}
}

namespace Window
{
	void MapList() noexcept
	{
		if (ImGui::Begin("Map List"))
		{
			for (auto&& info :
				Database::RX::MapMetaInfos
				| std::views::values
				| std::views::filter([](auto const& mi) static noexcept { return mi.m_parent_id == 0; })	// Show only root maps.
				)
			{
				DrawTree(info);
			}
		}
		ImGui::End();
	}

	void MapDisplay() noexcept
	{
		if (ImGui::Begin("Map Display", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (s_MapOnDisplay)
			{
				if (ImGui::Button("Export Map"))
				{
					auto const szPath =
						std::format("Map{0:0>3}_{1}.png", s_MapOnDisplay->m_pMapDatum->m_id, s_MapOnDisplay->m_Name);	// #UPDATE_AT_CPP26 formatting fs::path

					if (s_MapOnDisplay->m_GameMap.m_Canvas.SaveToFile(szPath))
						std::println("Map exported to '{}'.", szPath);
					else
						std::println("Failed to export map to '{}'.", szPath);
				}

				static float s_flZoom = 1.f;
				ImGui::SliderFloat("Zoom", &s_flZoom, 0.1f, 4.f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::SeparatorText("Meta Info");

				ImGui::Text("Size: %dx%d", s_MapOnDisplay->m_pMapDatum->m_width, s_MapOnDisplay->m_pMapDatum->m_height);
				ImGui::Text("Resolution: %dx%d", s_MapOnDisplay->m_GameMap.m_Canvas.m_Width, s_MapOnDisplay->m_GameMap.m_Canvas.m_Height);
				ImGui::Text("Referenced Tileset: %s", s_MapOnDisplay->m_pTileset->m_Name.data());

				ImGui::SeparatorText("Preview");

				// Remember that the texture rendered by OpenGL is upside down.
				ImGui::Image(
					s_MapOnDisplay->GetTextureId(),
					{ (float)s_MapOnDisplay->m_GameMap.m_Canvas.m_Width * s_flZoom, (float)s_MapOnDisplay->m_GameMap.m_Canvas.m_Height * s_flZoom },
					{ 0, 1 },
					{ 1, 0 }
				);
			}
			else
			{
				ImGui::TextUnformatted("Select a map from 'Map List' window.");
			}
		}
		ImGui::End();
	}
}
