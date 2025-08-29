module;

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Game.Path;

#ifndef __INTELLISENSE__
import std.compat;
#endif

namespace PokemonEssentials
{
	export inline std::filesystem::path GamePath{ LR"(C:\Users\Hydrogen\Documents\GitHub\Pokemon Essentials\)" };

	export bool AssignGamePath(std::filesystem::path NewPath) noexcept
	{
		if (!std::filesystem::is_directory(NewPath / L"Data/"))
		{
			std::println("'Data' folder no found!");
			return false;
		}

		if (!std::filesystem::is_directory(NewPath / L"PBS/"))
		{
			std::println("'PBS' folder no found!");
			return false;
		}

		if (!std::filesystem::is_directory(NewPath / L"Graphics/"))
		{
			std::println("'Graphics' folder no found!");
			return false;
		}

		GamePath = std::move(NewPath);
		std::println("Game path set to '{}'", GamePath.u8string());
		return true;
	}
}
