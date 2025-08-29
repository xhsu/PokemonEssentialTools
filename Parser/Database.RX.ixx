module;

#include <assert.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Database.RX;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import Ruby.Deserializer;

// #UPDATE_AT_CPP26 reflection
#define FIND_IN_OBJ(x) m_##x{ Dumped.Find<decltype(this->m_##x)>("@" #x) }

namespace Database::RX
{
	export struct Tileset
	{
		std::int32_t m_id{};
		std::string m_name{};	// display name
		std::string m_tileset_name{};	// actual file name, without extension
		std::vector<std::string> m_autotile_names{};
		std::string m_panorama_name{};
		std::int16_t m_panorama_hue{};
		std::string m_fog_name{};
		std::int16_t m_fog_hue{};
		std::uint8_t m_fog_opacity{}, m_fog_blend_type{};
		std::int32_t m_fog_zoom{};
		std::int32_t m_fog_sx{}, m_fog_sy{};
		std::string m_battleback_name{};

		static constexpr auto AUTOTILE_COUNT = 8;	// RPG Maker XP has 8 autotiles, 7 regular and 1 null autotile.
		static constexpr auto AUTOTILE_TILE = 48;	// Each autotile composed of 48 different shapes of tile to fit in the map.
		static constexpr auto AUTOTILE_SIZE = AUTOTILE_TILE * AUTOTILE_COUNT;	// 48 * 8 = 384

		// RPG Maker XP tile index comes with a offset of 384. It's a null autotile (48) plus 7 regular autotiles. Therefore 48 * 8 = 384.
		Ruby::Deserializer::Table m_passages{}, m_priorities{}, m_terrain_tags{};

		Tileset(Ruby::Deserializer::Object const& Dumped) noexcept
			: FIND_IN_OBJ(id), FIND_IN_OBJ(name), FIND_IN_OBJ(tileset_name), FIND_IN_OBJ(autotile_names),
			FIND_IN_OBJ(panorama_name), FIND_IN_OBJ(panorama_hue),
			FIND_IN_OBJ(fog_name), FIND_IN_OBJ(fog_hue), FIND_IN_OBJ(fog_opacity), FIND_IN_OBJ(fog_blend_type), FIND_IN_OBJ(fog_zoom), FIND_IN_OBJ(fog_sx), FIND_IN_OBJ(fog_sy),
			FIND_IN_OBJ(battleback_name), FIND_IN_OBJ(passages), FIND_IN_OBJ(priorities), FIND_IN_OBJ(terrain_tags)
		{

		}

		constexpr Tileset() noexcept = default;
		constexpr Tileset(Tileset const&) noexcept = default;
		constexpr Tileset(Tileset&&) noexcept = default;
		constexpr Tileset& operator=(Tileset const&) noexcept = default;
		constexpr Tileset& operator=(Tileset&&) noexcept = default;
		constexpr ~Tileset() noexcept = default;
	};

	export struct MapInfo
	{
		std::string m_name{};
		std::uint16_t m_parent_id{};	// 0 means no parent
		std::int16_t m_order{};
		bool m_expanded{};
		std::uint8_t m_scroll_x{}, m_scroll_y{};	// unused

		struct MapDatum* m_pMapDatum{};	// don't serialize this, it's not in original object.
		std::vector<MapInfo const*> m_rgpChildren{};	// don't serialize this, it's not in original object.

		MapInfo(Ruby::Deserializer::Object const& Dumped) noexcept
			: FIND_IN_OBJ(name), FIND_IN_OBJ(parent_id), FIND_IN_OBJ(order), FIND_IN_OBJ(expanded),
			FIND_IN_OBJ(scroll_x), FIND_IN_OBJ(scroll_y)
		{

		}

		constexpr MapInfo() noexcept = default;
		constexpr MapInfo(MapInfo const&) noexcept = default;
		constexpr MapInfo(MapInfo&&) noexcept = default;
		constexpr MapInfo& operator=(MapInfo const&) noexcept = default;
		constexpr MapInfo& operator=(MapInfo&&) noexcept = default;
		constexpr ~MapInfo() noexcept = default;
	};

	export struct MapDatum
	{
		std::int32_t m_tileset_id{};
		std::int32_t m_width{}, m_height{};
		bool m_autoplay_bgm{};
		// bgm - #TODO
		bool m_autoplay_bgs{};
		// bgs - #TODO
		//std::vector<> m_encounter_list{};	// - UNUSED in pokemon essentials
		//std::int32_t m_encounter_step{};	// - UNUSED in pokemon essentials
		Ruby::Deserializer::Table m_data{};

		//std::map<int, RxData::Event> m_events;	// - #TODO RPGMakerXP events

		std::int32_t m_id{};	// don't serialize this, it's not in original object.

		MapDatum(Ruby::Deserializer::Object const& Dumped) noexcept
			: FIND_IN_OBJ(tileset_id), FIND_IN_OBJ(width), FIND_IN_OBJ(height),
			FIND_IN_OBJ(autoplay_bgm), FIND_IN_OBJ(autoplay_bgs), FIND_IN_OBJ(data)
		{
		}

		constexpr MapDatum() noexcept = default;
		constexpr MapDatum(MapDatum const&) noexcept = default;
		constexpr MapDatum(MapDatum&&) noexcept = default;
		constexpr MapDatum& operator=(MapDatum const&) noexcept = default;
		constexpr MapDatum& operator=(MapDatum&&) noexcept = default;
		constexpr ~MapDatum() noexcept = default;
	};
}

static [[nodiscard]] inline auto ReadTileset(std::filesystem::path const& GameRootPath) noexcept
{
	std::ifstream file;
	file.open(GameRootPath / L"Data/Tilesets.rxdata", std::ios::binary);

	// Verify Version
	char version[2]{};
	file.read(version, 2);
	assert(version[0] == 4);
	assert(version[1] == 8);

	Ruby::Deserializer::Reader reader(file);
	auto result = reader.parse();	// LUNA: if we decouple this, the array store in std::any will be empty! why?
	file.close();

	std::vector<Database::RX::Tileset> res{};
	auto& tilesets = **std::any_cast<std::vector<std::any>*>(&result);
	for (auto&& obj : tilesets)
	{
		if (obj.type() == typeid(Ruby::Deserializer::Object*))
		{
			auto& obj2 = **std::any_cast<Ruby::Deserializer::Object*>(&obj);
			res.emplace_back(obj2);
		}
	}

	return res;
}

static [[nodiscard]] inline auto ReadMapInfo(std::filesystem::path const& GameRootPath) noexcept
{
	std::ifstream file;
	file.open(GameRootPath / L"Data/MapInfos.rxdata", std::ios::binary);

	// Verify Version
	char version[2]{};
	file.read(version, 2);
	assert(version[0] == 4);
	assert(version[1] == 8);

	Ruby::Deserializer::Reader reader(file);
	auto result = reader.parse();	// LUNA: if we decouple this, the array store in std::any will be empty! why?
	file.close();

	std::map<std::int32_t, Database::RX::MapInfo, std::less<>> res{};
	auto& mapinfos = **std::any_cast<std::map<std::int32_t, std::any, std::less<>>*>(&result);
	for (auto&& [id, obj] : mapinfos)
	{
		if (obj.type() == typeid(Ruby::Deserializer::Object*))
		{
			auto& obj2 = **std::any_cast<Ruby::Deserializer::Object*>(&obj);
			res.try_emplace(
				id,
				obj2
			);
		}
	}

	// Organize into tree structure.
	for (auto&& [id, info] : res)
	{
		if (info.m_parent_id != 0)
		{
			auto const it = res.find(info.m_parent_id);
			if (it != res.end())
				it->second.m_rgpChildren.push_back(&info);
		}
	}

	return res;
}

namespace Database::RX
{
	export inline decltype(ReadTileset({})) Tilesets;
	export inline decltype(ReadMapInfo({})) MapMetaInfos;
}

static [[nodiscard]] inline auto ReadMapData(std::filesystem::path const& GameRootPath) noexcept
{
	std::vector<Database::RX::MapDatum> ret{};
	ret.reserve(Database::RX::MapMetaInfos.size() + 1);	// Keep address stable.

	auto const DATA_FOLDER = (GameRootPath / L"Data/").u8string();

	for (auto&& index : Database::RX::MapMetaInfos | std::views::keys)
	{
		auto const szPath = std::format("{0}Map{1:0>3}.rxdata", DATA_FOLDER, index);	// #UPDATE_AT_CPP26 formatting fs::path
		auto const Path = std::filesystem::path{ szPath };

		if (!std::filesystem::exists(Path))
		{
			std::println("Map file 'Map{:0>3}.rxdata' does not exist, skipping...", index);
			continue;
		}

		std::ifstream file{ Path.c_str(), std::ios::binary };

		// Verify Version
		char version[2]{};
		file.read(version, 2);
		assert(version[0] == 4);
		assert(version[1] == 8);

		Ruby::Deserializer::Reader reader{ file };
		auto result = reader.parse();	// LUNA: if we decouple this, the array store in std::any will be empty! why?
		file.close();

		if (result.type() == typeid(Ruby::Deserializer::Object*))
		{
			auto& obj2 = **std::any_cast<Ruby::Deserializer::Object*>(&result);
			auto& MapDat = ret.emplace_back(obj2);

			// Supplimental data
			MapDat.m_id = index;

			Database::RX::MapMetaInfos.at(index).m_pMapDatum = &MapDat;
		}
		else
		{
			std::println("Map file 'Map{:0>3}.rxdata' does not contain a valid MapData object, skipping...", index);
		}
	}

	return ret;
}

namespace Database::RX
{
	export inline decltype(ReadMapData({})) MapData;
}

namespace Database::RX
{
	export void Load(std::filesystem::path const& GameRootPath) noexcept
	{
		Tilesets = ReadTileset(GameRootPath);
		MapMetaInfos = ReadMapInfo(GameRootPath);
		MapData = ReadMapData(GameRootPath);
	}
}
