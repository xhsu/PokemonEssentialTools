module;

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Image.Resources;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import UtlString;
import Image.Tilesets;

namespace PokemonEssentials::Resources
{
	export inline std::map<std::string, CAnimatedTileImage, sv_less_t> AnimatedTiles;
	export inline std::map<std::string, CAutotileImage, sv_less_t> AutoTiles;

	export inline std::map<std::string, CTilesetImage, sv_less_t> Tilesets;
}
