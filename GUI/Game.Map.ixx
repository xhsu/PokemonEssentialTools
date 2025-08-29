module;

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Game.Map;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import Database.RX;
import GL.GameMap;
import Game.Tilesets;
import Image.Query;
import Image.Tilesets;


export struct CMap final
{
	std::string_view m_Name{};
	Database::RX::MapDatum const* m_pMapDatum{};
	CTileset const* m_pTileset{};	// Tileset image for this map, only one tileset is used for each map.
	CGlGameMap m_GameMap;

	CMap(Database::RX::MapInfo const* pMapMetaInfo) noexcept
		: m_Name{ pMapMetaInfo->m_name }, m_pMapDatum{ pMapMetaInfo->m_pMapDatum },
		m_pTileset{ &Game::Tilesets.at(m_pMapDatum->m_tileset_id) },
		m_GameMap{ m_pMapDatum->m_width * CTilesetImage::TILE_WIDTH, m_pMapDatum->m_height * CTilesetImage::TILE_HEIGHT, m_pTileset }
	{
		for (int x = 0; x < m_pMapDatum->m_width; ++x)
		{
			for (int y = 0; y < m_pMapDatum->m_height; ++y)
			{
				for (int layer = 0; layer < CGlGameMap::TOTAL_LAYERS; ++layer)
				{
					auto iTileIndex =
						m_pMapDatum->m_data.data[x + y * m_pMapDatum->m_width + layer * m_pMapDatum->m_width * m_pMapDatum->m_height];

					// Index falling in first 48 are unknown tiles. No idea what they are, can't find any documentation about them.
					if (iTileIndex >= Database::RX::Tileset::AUTOTILE_TILE)
						m_GameMap.AddTile(x, y, iTileIndex);
				}
			}
		}

		m_GameMap.CommitVerticesChanges();
		m_pTileset->RenderAll();	// Or we just get pitch black.
		m_GameMap.Render();
	}

	CMap(CMap const&) noexcept = delete;
	CMap(CMap&&) noexcept = default;
	CMap& operator=(CMap const&) noexcept = delete;
	CMap& operator=(CMap&&) noexcept = default;
	~CMap() noexcept = default;

	[[nodiscard]] inline auto GetTextureId() const noexcept -> GLuint
	{
		return m_GameMap.m_Canvas.GetTextureId();
	}
};
