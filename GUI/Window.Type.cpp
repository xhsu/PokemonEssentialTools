#include <imgui.h>
#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif


import Database.Raw.PBS;
import GL.Canvas;
import GL.GameMap;
import Game.Map;
import Game.Path;
import Image.Tilesets;

extern auto LoadTextureFromFile(const wchar_t* file_name, bool bFlipY) noexcept -> std::expected<std::tuple<std::uint32_t, int, int>, std::string_view>;


struct Imgui_ID_RAII
{
	Imgui_ID_RAII(auto&& a) noexcept
	{
		ImGui::PushID(std::forward<decltype(a)>(a));
	}

	~Imgui_ID_RAII() noexcept
	{
		ImGui::PopID();
	}
};

namespace Window
{
	extern void TypeInfo() noexcept;
}

void Window::TypeInfo() noexcept
{
	static std::filesystem::path const TypeIconsFile{
		PokemonEssentials::GamePath / LR"(Graphics\UI\types.png)"
	};

	static auto const TypeIcons{
		LoadTextureFromFile(TypeIconsFile.c_str(), false)
	};

	if (!TypeIcons)
	{
		if (ImGui::Begin("Type Chart"))
			ImGui::Text("Failed to load type icons: %s", TypeIcons.error().data());
		ImGui::End();

		return;
	}

	static auto const TypeIconTexId{ std::get<0>(*TypeIcons) };
	static auto const TypeIconTotalWidth{ (float)std::get<1>(*TypeIcons) };
	static auto const TypeIconTotalHeight{ (float)std::get<2>(*TypeIcons) };
	static auto const TypeIconPerHeight{ (float)(TypeIconTotalHeight / PBS::Types.size()) };

	static std::vector const TypeInUse{ std::from_range,
		PBS::Types
		| std::views::values
		| std::views::filter(std::not_fn(&PokemonType::m_IsPseudoType))
		| std::views::transform([](auto& a) static noexcept { return std::addressof(a); })
	};

	static auto const rgfl{
		[]
		{
			std::vector<std::string_view> ret{};
			ret.resize(TypeInUse.size() * TypeInUse.size(), "neutral");

			std::mdspan<decltype(ret)::value_type, std::dextents<size_t, 2>> const TypeChart{
				ret.data(), TypeInUse.size(), TypeInUse.size()
			};

			for (int i = 0; i < std::ssize(TypeInUse); ++i)
			{
				for (int j = 0; j < std::ssize(TypeInUse); ++j)
				{
					if (std::ranges::contains(TypeInUse[i]->Weaknesses(), TypeInUse[j]))	// [i] is weak to [j]
						TypeChart[i, j] = "weak";
					if (std::ranges::contains(TypeInUse[i]->Immunities(), TypeInUse[j]))	// [i] is immu to [j]
						TypeChart[i, j] = "immune";
					if (std::ranges::contains(TypeInUse[i]->Resistances(), TypeInUse[j]))	// [i] is resi to [j]
						TypeChart[i, j] = "resisting";
				}
			}

			return ret;
		}()
	};
	static std::mdspan<decltype(rgfl)::value_type const, std::dextents<size_t, 2>> const TypeChart{
		rgfl.data(), TypeInUse.size(), TypeInUse.size()
	};

	if (ImGui::Begin("Type Chart"))
	{
		if (ImGui::BeginTable("table1", (int)TypeInUse.size() + 1, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
		{
			ImGui::TableSetupScrollFreeze(1, 1);
			ImGui::TableNextRow();

			for (auto&& [idx, pType] : std::views::enumerate(TypeInUse))
			{
				ImGui::TableSetColumnIndex((int)idx + 1);
				ImGui::Image(
					TypeIconTexId,
					{ TypeIconTotalWidth, TypeIconPerHeight },
					{ 0, (pType->m_IconPosition * TypeIconPerHeight) / TypeIconTotalHeight },
					{ 1, ((pType->m_IconPosition + 1) * TypeIconPerHeight) / TypeIconTotalHeight }
				);

				if (ImGui::BeginItemTooltip())
				{
					ImGui::TextUnformatted("[Attacker]");

					auto const fnPrintRelation =
						[&](const char* szSepText, std::string_view szRelText) noexcept
						{
							bool bAny{};

							ImGui::SeparatorText(szSepText);
							for (int i = 0; i < TypeChart.extent(1); ++i)
							{
								if (TypeChart[i, idx] == szRelText)
								{
									ImGui::TextUnformatted(TypeInUse[i]->m_Name.data());
									bAny = true;
								}
							}
							if (!bAny)
								ImGui::TextUnformatted("(NONE)");
						};

					fnPrintRelation("Effective", "weak");
					fnPrintRelation("Uneffective", "resisting");
					fnPrintRelation("Deals no damage", "immune");

					ImGui::EndTooltip();
				}
			}

			for (int row = 0; row < std::ssize(TypeInUse); row++)
			{
				[[maybe_unused]] Imgui_ID_RAII RAII_1(TypeInUse[row]);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Image(
					TypeIconTexId,
					{ TypeIconTotalWidth, TypeIconPerHeight },
					{ 0, (TypeInUse[row]->m_IconPosition * TypeIconPerHeight) / TypeIconTotalHeight },
					{ 1, ((TypeInUse[row]->m_IconPosition + 1) * TypeIconPerHeight) / TypeIconTotalHeight }
				);

				if (ImGui::BeginItemTooltip())
				{
					ImGui::TextUnformatted("[Defender]");

					auto const fnPrintRelation =
						[&](const char* szSepText, std::string_view szRelText) noexcept
						{
							bool bAny{};

							ImGui::SeparatorText(szSepText);
							for (int j = 0; j < TypeChart.extent(0); ++j)
							{
								if (TypeChart[row, j] == szRelText)
								{
									ImGui::TextUnformatted(TypeInUse[j]->m_Name.data());
									bAny = true;
								}
							}
							if (!bAny)
								ImGui::TextUnformatted("(NONE)");
						};

					fnPrintRelation("Weak to", "weak");
					fnPrintRelation("Resists", "resisting");
					fnPrintRelation("Immune to", "immune");

					ImGui::EndTooltip();
				}

				for (int column = 0; column < std::ssize(TypeInUse); column++)
				{
					[[maybe_unused]] Imgui_ID_RAII RAII_2(TypeInUse[column]);
					ImGui::TableSetColumnIndex(column + 1);

					std::string_view const& TypeMatch = TypeChart[row, column];
					if (TypeMatch != "neutral")
					{
						ImGui::TextUnformatted(TypeMatch.data());

						if (ImGui::BeginItemTooltip())
						{
							ImGui::TextUnformatted(
								std::format("{} is {} to {}", TypeInUse[row]->m_Name, TypeMatch, TypeInUse[column]->m_Name).c_str()
							);
							ImGui::EndTooltip();
						}
					}

					//if (TypeMatch == "weak")
					//	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0xFF'00'FF'00);
					//else if (TypeMatch == "resisting")
					//	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 0xFF'00'00'FF);
				}
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}
