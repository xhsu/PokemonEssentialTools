#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

// Image.cpp
extern auto GetTextureDimensions(const wchar_t* file_name) noexcept -> std::pair<int, int>;
//

import UtlString;
import Image.Tilesets;
import Image.Resources;
import Game.Path;

namespace PokemonEssentials::Resources
{
	void LoadAutotiles() noexcept
	{
		static std::filesystem::path const AutotilePath{
			PokemonEssentials::GamePath / LR"(Graphics\Autotiles\)"
		};

		if (!std::filesystem::exists(AutotilePath) || !std::filesystem::is_directory(AutotilePath))
		{
			std::println("Autotile path '{}' does not exist or is not a directory!", AutotilePath.u8string());
			return;
		}

		for (auto&& entry : std::filesystem::recursive_directory_iterator(AutotilePath))
		{
			if (!entry.is_regular_file())
				continue;

			auto&& [iWidth, iHeight] = GetTextureDimensions(entry.path().c_str());
			if (iWidth <= 0 || iHeight <= 0)
			{
				std::println("Invalid image dimensions for '{}', skipping...", entry.path().filename().u8string());
				continue;
			}

			if (CAnimatedTileImage::IsSizeAnimatedTile(iWidth, iHeight))
				Resources::AnimatedTiles.try_emplace(entry.path().stem().u8string(), entry.path().c_str());
			else if (CAutotileImage::IsSizeAutotile(iWidth, iHeight))
				Resources::AutoTiles.try_emplace(entry.path().stem().u8string(), entry.path().c_str());
		}
	}
}
