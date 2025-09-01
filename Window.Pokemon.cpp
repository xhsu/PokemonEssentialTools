#include <imgui.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import Database.PBS;

static std::vector<decltype(Database::PBS::Species)::value_type const*> s_NationalPokeDex;

namespace Window
{
	void Pokemons() noexcept
	{
		if (!ImGui::Begin("Pokemons"))
		{
			ImGui::End();
			return;
		}

		if (s_NationalPokeDex.empty())
		{
			s_NationalPokeDex.reserve(Database::PBS::Species.size());
			s_NationalPokeDex.append_range(
				Database::PBS::Species
				| std::views::transform([](auto& a) static noexcept { return std::addressof(a); })
			);

			std::ranges::sort(
				s_NationalPokeDex,
				{},
				[](auto&& a) static noexcept { return a->second.begin()->second.m_NationalDex; }
			);
		}

		for (auto&& [idx, entry] : std::views::enumerate(s_NationalPokeDex))
		{
			ImGui::TextUnformatted(
				std::format(
					"#{:0>3} - {}",
					entry->second.begin()->second.m_NationalDex,
					entry->first
				).c_str()
			);
		}

		ImGui::End();
	}
}