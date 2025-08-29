// Parser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
Naming convention.

Window.<Name> - Theres no doubt that this one is for ImGui window.
Game.<Name> - The objects that required for myself to compile, and is core logic to the game.
GL.<Name> - OpenGL related objects, I expected them to be reusable in other projects.
Image.<Name> - Image resource management, feel like it should be renamed to Resource.<Name> later??
Gui.<Name> - ImGui extensions. Should be resuable.

Tech debt:
Fucking namespaces. They are a mess.
The issue with RxData and PBS, and the compiled or reassambled class - they need to be distinguished.

Ruby.Deserialize - Handle .rxdata deserialization and its related objects.
Pbs.<Name> - Raw data directly read from PBS txt files.
Database.RX.<Name> - Deserialized and reorganized data from ruby. (or .rxdata) - NO GL INVOLVED!
Database.PBS.<Name> - Reorganized data from PBS txt files. - NO GL INVOLVED!

*/

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import Database.RX;
import Database.Raw.PBS;

import Game.Path;

namespace Database::PBS
{
	extern void Build() noexcept;
}

int main(int argc, char* argv[]) noexcept
{
	std::jthread thread_LoadRxData{ [] static noexcept { Database::RX::Load(PokemonEssentials::GamePath); } };
	PBS::Load(PokemonEssentials::GamePath);
	thread_LoadRxData.join();

	Database::PBS::Build();

	std::vector const TypeInUse{ std::from_range,
		PBS::Types
		| std::views::values
		| std::views::filter(std::not_fn(&PokemonType::m_IsPseudoType))
		| std::views::transform([](auto& a) static noexcept { return std::addressof(a); })
	};

	auto const iMaxLength =
		std::ranges::max(TypeInUse, {}, [](PokemonType* ty) { return ty->m_Name.length(); })->m_Name.length();

	static constexpr auto TRUNC = 4;
	std::print("|{0:^{1}}", "", iMaxLength + 2);

	std::vector<std::string_view> rgfl{};
	rgfl.resize(TypeInUse.size() * TypeInUse.size(), "1");

	std::mdspan<std::string_view, std::dextents<size_t, 2>> const TypeChart{ rgfl.data(), TypeInUse.size(), TypeInUse.size() };

	for (int i = 0; i < std::ssize(TypeInUse); ++i)
	{
		for (int j = 0; j < std::ssize(TypeInUse); ++j)
		{
			if (std::ranges::contains(TypeInUse[i]->Weaknesses(), TypeInUse[j]))	// [i] is weak to [j]
				TypeChart[i, j] = "2";
			if (std::ranges::contains(TypeInUse[i]->Immunities(), TypeInUse[j]))	// [i] is immu to [j]
				TypeChart[i, j] = "0";
			if (std::ranges::contains(TypeInUse[i]->Resistances(), TypeInUse[j]))	// [i] is resi to [j]
				TypeChart[i, j] = u8"½";
		}
	}

	for (auto&& ptr : TypeInUse)
	{
		std::string_view szDisplay{ ptr->m_Name };
		if (szDisplay.length() > TRUNC)
			szDisplay.remove_suffix(szDisplay.size() - TRUNC);

		std::print("| {0:^{1}} ", szDisplay, TRUNC);
	}
	std::print("|\n");

	for (auto&& [idx, ptr] : std::views::enumerate(TypeInUse))
	{
		std::print("| {0:^{1}} ", ptr->m_Name, iMaxLength);
		for (size_t j = 0; j < TypeChart.extent(1); ++j)
		{
			if (TypeChart[idx, j] == "1")
				std::print("| {0:^{1}} ", "", TRUNC);
			else
				std::print("| {0:^{1}} ", TypeChart[idx, j], TRUNC);
		}
		std::print("|\n");
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
