module;

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Image.Query;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import Database.RX;
import Image.Tilesets;
import Image.Resources;

namespace Utils::Query
{
	export auto tileset_image_by_id(int id) noexcept -> CTilesetImage const*
	{
		if (auto it = std::ranges::find(Database::RX::Tilesets, id, &Database::RX::Tileset::m_id);
			it != Database::RX::Tilesets.end())
		{
			auto& tileset = *it;
			[[maybe_unused]] auto& AllTilesets = PokemonEssentials::Resources::Tilesets;

			if (auto it2 = PokemonEssentials::Resources::Tilesets.find(tileset.m_tileset_name);
				it2 != PokemonEssentials::Resources::Tilesets.end())
			{
				return &it2->second;
			}
		}

		return nullptr;
	}
}
