#include <GLFW/glfw3.h>
#include <imgui.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import Image.Resources;
import Image.Tilesets;

namespace Window
{
	extern void AutoTiles() noexcept;
}

static void* s_pSelectedAutoTile = nullptr;

void Window::AutoTiles() noexcept
{
	if (ImGui::Begin("AutoTiles"))
	{
		ImGui::SeparatorText("Auto-Fitting Tiles");
		for (auto&& [szName, AutoTile] : PokemonEssentials::Resources::AutoTiles)
		{
			if (ImGui::Selectable(szName.c_str(), s_pSelectedAutoTile == &AutoTile))
				s_pSelectedAutoTile = &AutoTile;

			if (ImGui::BeginItemTooltip())
			{
				AutoTile.Render();

				ImGui::Image(
					AutoTile.m_Canvas.GetTextureId(),
					{ (float)AutoTile.m_Canvas.m_Width * 2.f, (float)AutoTile.m_Canvas.m_Height * 2.f },
					{ 0, 1 },
					{ 1, 0 }
				);

				ImGui::EndTooltip();
			}
		}

		ImGui::SeparatorText("Animated Tiles");
		for (auto&& [szName, AnimatedTile] : PokemonEssentials::Resources::AnimatedTiles)
		{
			if (ImGui::Selectable(szName.c_str(), s_pSelectedAutoTile == &AnimatedTile))
				s_pSelectedAutoTile = &AnimatedTile;

			if (ImGui::BeginItemTooltip())
			{
				AnimatedTile.Render((int)(glfwGetTime() * 10) % AnimatedTile.m_FrameCount);

				ImGui::Image(
					AnimatedTile.m_Canvas.GetTextureId(),
					{ (float)AnimatedTile.m_Canvas.m_Width * 2.f, (float)AnimatedTile.m_Canvas.m_Height * 2.f },
					{ 0, 1 },
					{ 1, 0 }
				);

				ImGui::EndTooltip();
			}
		}
	}
	ImGui::End();
}
