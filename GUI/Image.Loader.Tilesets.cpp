#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import UtlString;
import Image.Tilesets;
import Image.Resources;
import Game.Path;


// Image.cpp
extern auto GetTextureDimensions(const wchar_t* file_name) noexcept -> std::pair<int, int>;
//


namespace PokemonEssentials::Resources
{
	void LoadTilesets() noexcept
	{
		static std::filesystem::path const TilesetsPath{
			PokemonEssentials::GamePath / LR"(Graphics\Tilesets\)"
		};

		if (!std::filesystem::exists(TilesetsPath) || !std::filesystem::is_directory(TilesetsPath))
		{
			std::println("Tileset path '{}' does not exist or is not a directory!", TilesetsPath.u8string());
			return;
		}

		for (auto&& entry : std::filesystem::recursive_directory_iterator(TilesetsPath))
		{
			if (!entry.is_regular_file())
				continue;

			auto&& [iWidth, iHeight] = GetTextureDimensions(entry.path().c_str());
			if (iWidth <= 0 || iHeight <= 0)
			{
				std::println("Invalid image dimensions for '{}', skipping...", entry.path().filename().u8string());
				continue;
			}
			else if (iWidth % CTilesetImage::TILE_WIDTH != 0 || iHeight % CTilesetImage::TILE_HEIGHT != 0)
			{
				std::println("Image '{}' is not a multiple of tile size {}x{}, skipping...",
					entry.path().filename().u8string(), CTilesetImage::TILE_WIDTH, CTilesetImage::TILE_HEIGHT);
				continue;
			}

			Resources::Tilesets.try_emplace(
				entry.path().stem().u8string(),
				entry.path().c_str()
			);
		}
	}
}
