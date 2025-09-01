module;

#include <assert.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Database.PBS;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import UtlString;
import Database.Raw.PBS;

export namespace Database::PBS
{
	struct CPokemonType final
	{
		std::string_view m_Name{ "Unnamed" };
		std::uint8_t m_IconPosition{ 0 };
		bool m_IsSpecialType{ false };
		bool m_IsPseudoType{ false };
		std::span<std::string const> m_Flags{};
		std::vector<struct CPokemonType const*> m_Weaknesses{};
		std::vector<struct CPokemonType const*> m_Resistances{};
		std::vector<struct CPokemonType const*> m_Immunities{};

		// Extra

		::PokemonType const* m_Raw{};

		CPokemonType(::PokemonType const& Raw) noexcept;

		CPokemonType(CPokemonType const&) noexcept = delete;
		CPokemonType(CPokemonType&&) noexcept = delete;
		CPokemonType& operator=(CPokemonType const&) = delete;
		CPokemonType& operator=(CPokemonType&&) = delete;
		~CPokemonType() noexcept = default;
	};

	inline std::map<std::string_view, CPokemonType, sv_less_t> Types;

	struct CPokemonMove final
	{
		std::string_view m_Name{ "Unnamed" };
		CPokemonType const* m_Type{ nullptr };

		EMoveCategory m_Category{ EMoveCategory::Status };

		std::uint16_t m_Power{ 0 };
		std::uint16_t m_Accuracy{ 100 };
		std::uint8_t m_TotalPP{ 5 };
		std::int8_t m_Priority{ 0 };
		std::uint8_t m_EffectChance{ 0 };

		EMoveTarget m_Target{ EMoveTarget::None };

		std::string_view m_FunctionCode{ "None" };
		std::span<std::string const> m_Flags{};
		std::string_view m_Description{ "???" };

		CPokemonMove(::PokemonMove const& Raw) noexcept;

		CPokemonMove(CPokemonMove const&) noexcept = default;
		CPokemonMove(CPokemonMove&&) noexcept = default;
		CPokemonMove& operator=(CPokemonMove const&) noexcept = default;
		CPokemonMove& operator=(CPokemonMove&&) noexcept = default;
		~CPokemonMove() noexcept = default;
	};

	inline std::map<std::string_view, CPokemonMove, sv_less_t> Moves;

	struct CPokemonAbility final
	{
		std::string_view m_Name{ "Unnamed" };
		std::string_view m_Description{ "???" };
		std::span<std::string const> m_Flags{};

		CPokemonAbility(::PokemonAbility const& Raw) noexcept;

		constexpr CPokemonAbility() noexcept = default;
		constexpr CPokemonAbility(CPokemonAbility const&) noexcept = default;
		constexpr CPokemonAbility(CPokemonAbility&&) noexcept = default;
		constexpr CPokemonAbility& operator=(CPokemonAbility const&) noexcept = default;
		constexpr CPokemonAbility& operator=(CPokemonAbility&&) noexcept = default;
		constexpr ~CPokemonAbility() noexcept = default;
	};

	inline std::map<std::string_view, CPokemonAbility, sv_less_t> Abilities;

	struct CPokemonItem final
	{
		std::string_view m_Name{ "Unnamed" };
		std::string_view m_NamePlural{ "Unnamed" };
		std::string_view m_PortionName{};
		std::string_view m_PortionNamePlural{};

		std::uint8_t m_Pocket{ 1 };
		std::uint8_t m_BPPrice{ 1 };
		std::uint16_t m_Price{ 0 };
		std::uint16_t m_SellPrice{ (std::uint16_t)(this->m_Price / 2) };

		EFieldUse m_FieldUse{ EFieldUse::None };
		EBattleUse m_BattleUse{ EBattleUse::None };

		std::span<std::string const> m_Flags{};

		bool m_Consumable{ /*false if a Key Item, TM or HM, and true otherwise*/ GetDefault() };
		bool m_ShowQuantity{ /*false if a Key Item, TM or HM, and true otherwise*/ GetDefault() };

		CPokemonMove const* m_Move{ nullptr };

		std::string_view m_Description{ "???" };

	private:
		/* false if a Key Item, TM or HM, and true otherwise */
		[[nodiscard]] constexpr bool GetDefault() const noexcept
		{
			return !(std::ranges::contains(this->m_Flags, "KeyItem")
				|| this->m_FieldUse == EFieldUse::TM || this->m_FieldUse == EFieldUse::HM);
		}

	public:
		CPokemonItem(::PokemonItem const& Raw) noexcept;

		constexpr CPokemonItem() noexcept = default;
		constexpr CPokemonItem(CPokemonItem const&) noexcept = default;
		constexpr CPokemonItem(CPokemonItem&&) noexcept = default;
		constexpr CPokemonItem& operator=(CPokemonItem const&) noexcept = default;
		constexpr CPokemonItem& operator=(CPokemonItem&&) noexcept = default;
		constexpr ~CPokemonItem() noexcept = default;
	};

	inline std::map<std::string_view, CPokemonItem, sv_less_t> Items;

	struct CPokemonEvolution final
	{
		std::string_view m_Species{};
		std::string_view m_Method{};
		std::string_view m_Parameter{};
	};

	struct CPokemonSpecies final
	{
		std::string_view m_Name{ "Unnamed" };
		std::string_view m_FormName{ "" };
		std::vector<CPokemonType const*> m_Types{ std::addressof(Types.find("NORMAL")->second) };

		std::span<std::uint8_t const, 6> m_BaseStats;	// 1, 1, 1, 1, 1, 1 
		std::uint16_t m_BaseExp{ 100 };
		std::uint8_t m_CatchRate{ 255 };
		std::uint8_t m_Happiness{ 70 };
		std::uint8_t m_Generation{ 0 };
		// 8 bit space.
		std::uint32_t m_HatchSteps{ 1 }; // Total steps, not traditional hatching cycles in official games.

		EGenderRatio m_GenderRatio{ EGenderRatio::Female50Percent };

		EGrowthRate m_GrowthRate{ EGrowthRate::Medium };

		std::span<std::pair<std::string, std::int_fast16_t> const> m_EVs{};

		std::vector<CPokemonAbility const*> m_Abilities{};
		std::vector<CPokemonAbility const*> m_HiddenAbilities{};

		std::vector<std::pair<std::int_fast16_t, CPokemonMove const*>> m_Moves{};
		std::vector<CPokemonMove const*> m_TutorMoves{};
		std::vector<CPokemonMove const*> m_EggMoves{};

		std::span<std::string const> m_EggGroups{};	// "Undiscovered"
		CPokemonItem const* m_Incense{};

		std::span<std::string const> m_Offspring{};	// Keep the string, just check them in dict when needed

		float m_Height{ .1f }, m_Weight{ .1f };
		std::string_view m_Color{ "Red" };
		std::string_view m_Shape{ "Head" };
		std::string_view m_Habitat{ "None" };
		std::string_view m_Category{ "???" };
		std::string_view m_Pokedex{ "???" };

		std::span<std::string const> m_Flags{};

		CPokemonItem const* m_WildItemCommon{};
		CPokemonItem const* m_WildItemUncommon{};
		CPokemonItem const* m_WildItemRare{};

		std::vector<CPokemonEvolution> m_Evolutions{};

		decltype(::PokemonSpecies::m_NationalDex) m_NationalDex{};

		// Extra

		::PokemonSpecies const* m_Raw{};

		CPokemonSpecies(::PokemonSpecies const& Raw) noexcept;

		CPokemonSpecies(CPokemonSpecies const&) noexcept = delete;
		CPokemonSpecies(CPokemonSpecies&&) noexcept = delete;
		CPokemonSpecies& operator=(CPokemonSpecies const&) = delete;
		CPokemonSpecies& operator=(CPokemonSpecies&&) noexcept = delete;
		~CPokemonSpecies() noexcept = default;
	};

	inline std::map<
		std::string_view,
		std::map<int, CPokemonSpecies, std::less<>>,
		sv_less_t
	> Species;

	extern "C++" void Build() noexcept;
}
