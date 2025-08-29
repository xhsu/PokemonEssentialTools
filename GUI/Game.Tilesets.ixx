module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Game.Tilesets;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import UtlString;
import Database.RX;
import Game.Path;
import Image.Query;
import Image.Resources;
import Image.Tilesets;


export struct CTileset final
{
	std::string_view m_Name{};
	Database::RX::Tileset const* m_pTilesetDatabase{};
	std::array<std::variant<CTilesetImage const*, CAutotileImage const*, CAnimatedTileImage const*, std::nullptr_t>, 8> m_rgpTilesetTextures{
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
	};

	CTileset(Database::RX::Tileset const* pTilesetDatabase) noexcept
		: m_Name{ pTilesetDatabase->m_name }, m_pTilesetDatabase{ pTilesetDatabase }
	{
		m_rgpTilesetTextures[0] = Utils::Query::tileset_image_by_id(pTilesetDatabase->m_id);	// Index 0 is always the main tileset image.

		for (auto&& [iIndex, szAutoTileName] : std::views::enumerate(pTilesetDatabase->m_autotile_names))
		{
			if (auto it = PokemonEssentials::Resources::AnimatedTiles.find(szAutoTileName);
				it != PokemonEssentials::Resources::AnimatedTiles.end())
			{
				m_rgpTilesetTextures[iIndex + 1] = &it->second;
			}
			else if (auto it2 = PokemonEssentials::Resources::AutoTiles.find(szAutoTileName);
				it2 != PokemonEssentials::Resources::AutoTiles.end())
			{
				m_rgpTilesetTextures[iIndex + 1] = &it2->second;
			}
			else
			{
				m_rgpTilesetTextures[iIndex + 1] = nullptr;	// No autotile found, set to nullptr.
			}
		}
	}

	CTileset(CTileset const&) noexcept = delete;
	CTileset(CTileset&&) noexcept = default;
	CTileset& operator=(CTileset const&) noexcept = delete;
	CTileset& operator=(CTileset&&) noexcept = default;
	~CTileset() noexcept = default;

	[[nodiscard]] auto GetMainTilesetObject() const noexcept -> CTilesetImage const*
	{
		if (auto ptr = std::get_if<CTilesetImage const*>(&m_rgpTilesetTextures[0]); ptr && *ptr) [[likely]]
			return *ptr;

		return nullptr;
	}

	[[nodiscard]] auto GetTextureIdAt(int iIndex) const noexcept -> GLuint
	{
		if (iIndex < 0 || iIndex >= std::ssize(m_rgpTilesetTextures))
			return 0;

		return std::visit(
			[]<typename T>(T&& pTexture) static noexcept -> GLuint
			{
				if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>)
				{
					return 0;
				}
				else
				{
					if (pTexture)
						return pTexture->GetOutputTextureId();

					return 0;
				}
			}, m_rgpTilesetTextures[iIndex]
		);
	}

	[[nodiscard]] auto GetTilePageIndex(int iTileIndex) const noexcept -> int
	{
		if (iTileIndex >= Database::RX::Tileset::AUTOTILE_SIZE || iTileIndex < Database::RX::Tileset::AUTOTILE_TILE)
			return 0;	// Main tileset image.

		// The first 48 tiles are unused, for unknown reason.
		// So we don't need to offset the result, as this quot starts from 1 and up to 7.
		return iTileIndex / Database::RX::Tileset::AUTOTILE_TILE;
	}

	[[nodiscard]] auto GetTextureDimensionsAt(int iIndex) const noexcept -> std::pair<int, int>
	{
		if (iIndex < 0 || iIndex >= std::ssize(m_rgpTilesetTextures))
			return { 0, 0 };

		return std::visit(
			[]<typename T>(T&& pTexture) static noexcept -> std::pair<int, int>
			{
				if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>)
				{
					return { 0, 0 };
				}
				else
				{
					if (pTexture)
						return pTexture->GetOutputTextureDimension();

					return { 0, 0 };
				}
			}, m_rgpTilesetTextures[iIndex]
		);
	}

	[[nodiscard]] auto GetTilesetDimensionsAt(int iIndex) const noexcept -> std::pair<int, int>
	{
		if (iIndex < 0 || iIndex >= std::ssize(m_rgpTilesetTextures))
			return { 0, 0 };

		return std::visit(
			[]<typename T>(T && pTexture) static noexcept -> std::pair<int, int>
			{
				if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>)
				{
					return { 0, 0 };
				}
				else
				{
					if (pTexture)
						return pTexture->GetOutputTilesetDimension();

					return { 0, 0 };
				}
			}, m_rgpTilesetTextures[iIndex]
		);
	}

	[[nodiscard]] static constexpr auto GetNormalizedTileIndex(int iTileIndex) noexcept -> decltype(iTileIndex)
	{
		if (iTileIndex < Database::RX::Tileset::AUTOTILE_TILE)
			return 0;	// Unknown tile.

		if (iTileIndex >= Database::RX::Tileset::AUTOTILE_SIZE)
			return iTileIndex - Database::RX::Tileset::AUTOTILE_SIZE;	// Main tileset image.

		return iTileIndex % Database::RX::Tileset::AUTOTILE_TILE;	// Auto-tile or animated tile image.
	}

	void RenderAll() const noexcept
	{
		for (auto&& pTexture : m_rgpTilesetTextures)
		{
			std::visit(
				[]<typename T>(T&& pTexture) static noexcept
				{
					if constexpr (requires { pTexture->Render(); })
					{
						if (pTexture)
							pTexture->Render();
					}
					else if constexpr (requires { pTexture->Render(0); })
					{
						if (pTexture)
							pTexture->Render(0);	// Render the first frame.
					}
					else
					{
						// Do nothing.
					}
				}, pTexture
			);
		}
	}
};

namespace Game
{
	export inline std::map<std::int32_t, CTileset, std::less<>> Tilesets{};

	export void CompileTilesets() noexcept
	{
		Tilesets.clear();

		for (auto&& TilesetMetaData : Database::RX::Tilesets)
		{
			Tilesets.try_emplace(TilesetMetaData.m_id, &TilesetMetaData);
		}
	}

} // namespace Game
