module;

#ifdef __INTELLISENSE__
#endif

export module Database.Raw.PBS;

#ifndef __INTELLISENSE__
import std.compat;
#else
import std;
#endif

import UtlFile;
import UtlString;


struct IniBlock
{
	std::string m_Id{};
	std::string m_Comment{};
	std::map<std::string, std::string, sv_less_t> m_KeyValues{};

	template <typename R = std::ranges::empty_view<decltype(IniBlock::m_KeyValues)::mapped_type>>
	auto EjectArr(auto&& a, R&& def = {}, const char* delim = ", \t") && noexcept -> std::vector<decltype(IniBlock::m_KeyValues)::mapped_type>
	{
		if (auto it = m_KeyValues.find(std::forward<decltype(a)>(a)); it != m_KeyValues.end())
		{
			return
				UTIL_Split(it->second, delim)
				| std::views::transform([](auto&&... a) static noexcept { return std::string{ std::forward<decltype(a)>(a)... }; })
				| std::ranges::to<std::vector>();
		}

		return {};
	}

	auto EjectStr(auto&& a, auto&& def) && noexcept -> std::string
	{
		if (auto it = m_KeyValues.find(std::forward<decltype(a)>(a)); it != m_KeyValues.end())
		{
			if (!it->second.empty())
				return std::move(it->second);
		}

		return { std::forward<decltype(def)>(def) };
	}

	template <typename T>
	auto EjectNum(auto&& a, T def = {}) && noexcept -> T
	{
		auto const sz = std::move(*this).EjectStr(std::forward<decltype(a)>(a), "");
		return UTIL_StrToNum<T>(sz, def);
	}

	bool EjectBool(auto&& a, bool def = false) && noexcept
	{
		auto const sz = std::move(*this).EjectStr(std::forward<decltype(a)>(a), "");
		if (sz.empty())
			return def;

		static constexpr std::string_view szTrue = "true";
		static constexpr auto fnToLower = [](auto c) static noexcept { return std::tolower(c); };

		return std::ranges::equal(sz, szTrue, {}, fnToLower);
	}

	template <typename T, size_t N>
	auto EjectVec(auto&& a, std::array<T, N> def = {}) && noexcept
	{
		if (auto it = m_KeyValues.find(std::forward<decltype(a)>(a)); it != m_KeyValues.end())
		{
			auto const vec =
				UTIL_Split(it->second, ", \t")
				| std::views::transform([](auto&&... a) static noexcept { return UTIL_StrToNum<T>(std::forward<decltype(a)>(a)...); })
				| std::ranges::to<std::vector>();

			for (auto&& [lhs, rhs] : std::views::zip(def, vec))
			{
				lhs = rhs;
			}
		}

		return def;
	}

	template <int iSize>
	auto EjectTuple(std::string_view szKey, auto&& fnVisitor) && noexcept -> std::expected<void, std::string_view>
	{
		auto RawDat = std::move(*this).EjectStr(szKey, "");
		if (!RawDat.empty())
		{
			if (auto const iCommaCount = std::ranges::count(RawDat, ','); (iCommaCount + 1) % iSize != 0) [[unlikely]]
				std::abort();

			auto chunks =
				RawDat
				| std::views::split(std::string_view{ "," })
				| std::views::transform([](auto&& a) static noexcept { return UTIL_Trim(std::string_view{ a }); })
				| std::ranges::to<std::vector>()
				| std::views::chunk(iSize)
				;

			for (auto&& arr : chunks)
				fnVisitor(std::forward<decltype(arr)>(arr));

			return {};
		}

		return std::unexpected("Key not found");
	}
};

struct IniFile
{
	std::vector<IniBlock> m_Entries{};

	static IniFile Make(std::string_view input) noexcept
	{
		IniFile ret{};
		auto rgszLines = UTIL_Split(input, "\r\n");

		for (auto&& szLine : rgszLines)
		{
			if (auto pos = szLine.find_first_of('#'); pos != szLine.npos)
				szLine.remove_suffix(szLine.length() - pos);

			szLine = UTIL_Trim(szLine);

			if (szLine.empty())
				continue;

			if (szLine.starts_with('[') && szLine.ends_with(']'))
				ret.m_Entries.emplace_back(std::string{ szLine.substr(1, szLine.length() - 2) });

			if (auto pos = szLine.find_first_of('='); pos != szLine.npos)
			{
				auto const lhs = UTIL_Trim(szLine.substr(0, pos));
				auto const rhs = UTIL_Trim(szLine.substr(pos + 1));
				auto& Block = ret.m_Entries.back();

				if (!Block.m_KeyValues.contains(lhs))
				{
					Block.m_KeyValues.try_emplace(
						std::string{ lhs },
						std::string{ rhs }
					);
				}
			}
		}

		return ret;
	}

	template <typename T>
	static auto Factory(std::string_view input) noexcept -> std::map<std::string, T, sv_less_t>
	{
		auto Config = IniFile::Make(input);
		auto ret =
			Config.m_Entries
			| std::views::as_rvalue
			| std::views::transform([](auto&& a) static noexcept { return std::pair{ std::move(a.m_Id), T{ std::forward<decltype(a)>(a) } }; })
			| std::ranges::to<std::map<std::string, T, sv_less_t>>();

		return ret;
	}
};

#define READ_STR(key, def) m_##key{ std::move(src).EjectStr(#key, def) }
#define READ_NUM(key, def) m_##key{ std::move(src).EjectNum<decltype(m_##key)>(#key, def) }
#define READ_ARR(key) m_##key{ std::move(src).EjectArr(#key) }
#define READ_ARRDEF(key, ...) m_##key{ std::move(src).EjectArr(#key, std::array{ __VA_ARGS__ }) }
#define READ_BOOL(key, def) m_##key{ std::move(src).EjectBool(#key, def) }
#define READ_VEC(key, ...) m_##key{ std::move(src).EjectVec(#key, decltype(m_##key){ __VA_ARGS__ }) }

#pragma region Pokemon

export struct PokemonType
{
	std::string m_Name{ "Unnamed" };
	std::uint8_t m_IconPosition{ 0 };
	bool m_IsSpecialType{ false };
	bool m_IsPseudoType{ false };
	std::vector<std::string> m_Flags{};
	std::vector<std::string> m_Weaknesses{};
	std::vector<std::string> m_Resistances{};
	std::vector<std::string> m_Immunities{};

	PokemonType(IniBlock&& src) noexcept
		: READ_STR(Name, "Unnamed"), READ_NUM(IconPosition, 0),
		READ_BOOL(IsSpecialType, false), READ_BOOL(IsPseudoType, false), READ_ARR(Flags),
		READ_ARR(Weaknesses), READ_ARR(Resistances), READ_ARR(Immunities)
	{

	}

	constexpr PokemonType() noexcept = default;
	constexpr PokemonType(PokemonType const&) noexcept = default;
	constexpr PokemonType(PokemonType&&) noexcept = default;
	constexpr PokemonType& operator=(PokemonType const&) noexcept = default;
	constexpr PokemonType& operator=(PokemonType&&) noexcept = default;
	constexpr ~PokemonType() noexcept = default;

	[[nodiscard]] auto Weaknesses() const noexcept -> std::generator<PokemonType*>;
	[[nodiscard]] auto Resistances() const noexcept -> std::generator<PokemonType*>;
	[[nodiscard]] auto Immunities() const noexcept -> std::generator<PokemonType*>;
};

namespace PBS { export inline decltype(IniFile::Factory<PokemonType>({})) Types; }

auto PokemonType::Weaknesses() const noexcept -> std::generator<PokemonType*>
{
	for (auto&& TypeId : m_Weaknesses)
	{
		if (auto it = PBS::Types.find(TypeId); it != PBS::Types.cend())
			co_yield std::addressof(it->second);
	}
}

auto PokemonType::Resistances() const noexcept -> std::generator<PokemonType*>
{
	for (auto&& TypeId : m_Resistances)
	{
		if (auto it = PBS::Types.find(TypeId); it != PBS::Types.cend())
			co_yield std::addressof(it->second);
	}
}

auto PokemonType::Immunities() const noexcept -> std::generator<PokemonType*>
{
	for (auto&& TypeId : m_Immunities)
	{
		if (auto it = PBS::Types.find(TypeId); it != PBS::Types.cend())
			co_yield std::addressof(it->second);
	}
}

export enum struct EMoveCategory : std::uint8_t { Physical, Special, Status, };
export enum struct EMoveTarget : std::uint8_t
{
	None,
	User,
	NearAlly,
	UserOrNearAlly,
	AllAllies,
	UserAndAllies,
	NearFoe,
	RandomNearFoe,
	AllNearFoes,
	Foe,
	AllFoes,
	NearOther,
	AllNearOthers,
	Other,
	AllBattlers,
	UserSide,
	FoeSide,
	BothSides,
};

export struct PokemonMove
{
	std::string m_Name{ "Unnamed" };
	std::string m_Type{ "NONE" };

	// EMoveCategory
	std::string m_Category{ "Status" };

	std::uint16_t m_Power{ 0 };
	std::uint16_t m_Accuracy{ 100 };
	std::uint8_t m_TotalPP{ 5 };
	std::int8_t m_Priority{ 0 };
	std::uint8_t m_EffectChance{ 0 };

	// EMoveTarget
	std::string m_Target{ "None" };

	std::string m_FunctionCode{ "None" };
	std::vector<std::string> m_Flags{};
	std::string m_Description{ "???" };

	PokemonMove(IniBlock&& src) noexcept : READ_STR(Name, "Unnamed"), READ_STR(Type, "NONE"), READ_STR(Category, "Status"),
		READ_NUM(Power, 0), READ_NUM(Accuracy, 100), READ_NUM(TotalPP, 5),
		READ_NUM(Priority, 0), READ_NUM(EffectChance, 0), READ_STR(Target, "None"), READ_STR(FunctionCode, "None"),
		READ_ARR(Flags), READ_STR(Description, "???")
	{

	}

	constexpr PokemonMove() noexcept = default;
	constexpr PokemonMove(PokemonMove const&) noexcept = default;
	constexpr PokemonMove(PokemonMove&&) noexcept = default;
	constexpr PokemonMove& operator=(PokemonMove const&) noexcept = default;
	constexpr PokemonMove& operator=(PokemonMove&&) noexcept = default;
	constexpr ~PokemonMove() noexcept = default;

	[[nodiscard]] auto Type() const noexcept -> PokemonType*
	{
		if (auto it = PBS::Types.find(m_Type); it != PBS::Types.cend())
			return std::addressof(it->second);

		return nullptr;
	}
};

namespace PBS { export inline decltype(IniFile::Factory<PokemonMove>({})) Moves; }

export struct PokemonAbility
{
	std::string m_Name{ "Unnamed" };
	std::string m_Description{ "???" };
	std::vector<std::string> m_Flags{};

	PokemonAbility(IniBlock&& src) noexcept
		: m_Name{ std::move(src).EjectStr("Name", "Unnamed") }, m_Description{ std::move(src).EjectStr("Description", "???") },
		m_Flags{ std::move(src).EjectArr("Flags") }
	{

	}

	constexpr PokemonAbility() noexcept = default;
	constexpr PokemonAbility(PokemonAbility const&) noexcept = default;
	constexpr PokemonAbility(PokemonAbility&&) noexcept = default;
	constexpr PokemonAbility& operator=(PokemonAbility const&) noexcept = default;
	constexpr PokemonAbility& operator=(PokemonAbility&&) noexcept = default;
	constexpr ~PokemonAbility() noexcept = default;
};

namespace PBS { export inline decltype(IniFile::Factory<PokemonAbility>({})) Abilities; }

export enum struct EFieldUse : std::uint8_t { None = 0, OnPokemon = 1, Direct, TR, TM, HM, };
export enum struct EBattleUse : std::uint8_t { None = 0, OnPokemon = 1, OnMove, OnBattler, OnFoe, Direct, };

export struct PokemonItem
{
	std::string m_Name{ "Unnamed" };
	std::string m_NamePlural{ "Unnamed" };
	std::string m_PortionName{};
	std::string m_PortionNamePlural{};

	std::uint8_t m_Pocket{ 1 };
	std::uint8_t m_BPPrice{ 1 };
	std::uint16_t m_Price{ 0 };
	std::uint16_t m_SellPrice{ (std::uint16_t)(this->m_Price / 2) };

	// EFieldUse
	std::string m_FieldUse{};

	// EBattleUse
	std::string m_BattleUse{};

	std::vector<std::string> m_Flags{};

	bool m_Consumable{/*false if a Key Item, TM or HM, and true otherwise*/ };
	bool m_ShowQuantity{/*false if a Key Item, TM or HM, and true otherwise*/ };

	std::string m_Move{};
	[[nodiscard]] auto Move() const noexcept -> PokemonMove*
	{
		if (auto it = PBS::Moves.find(m_Move); it != PBS::Moves.cend())
			return std::addressof(it->second);

		return nullptr;
	}

	std::string m_Description{ "???" };

private:
	[[nodiscard]] constexpr bool GetDefault() const noexcept
	{
		return !(std::ranges::contains(this->m_Flags, "KeyItem")
			|| this->m_FieldUse == "TM" || this->m_FieldUse == "HM");
	}

public:
	PokemonItem(IniBlock&& src) noexcept
		: READ_STR(Name, "Unnamed"), READ_STR(NamePlural, "Unnamed"), READ_STR(PortionName, ""), READ_STR(PortionNamePlural, ""),
		READ_NUM(Pocket, 1), READ_NUM(BPPrice, 1), READ_NUM(Price, 0), READ_NUM(SellPrice, ((std::uint16_t)(this->m_Price / 2))),
		READ_STR(FieldUse, ""), READ_STR(BattleUse, ""), READ_ARR(Flags),
		READ_BOOL(Consumable, this->GetDefault()), READ_BOOL(ShowQuantity, this->GetDefault()),
		READ_STR(Move, ""), READ_STR(Description, "???")
	{

	}

	constexpr PokemonItem() noexcept = default;
	constexpr PokemonItem(PokemonItem const&) noexcept = default;
	constexpr PokemonItem(PokemonItem&&) noexcept = default;
	constexpr PokemonItem& operator=(PokemonItem const&) noexcept = default;
	constexpr PokemonItem& operator=(PokemonItem&&) noexcept = default;
	constexpr ~PokemonItem() noexcept = default;
};

namespace PBS { export inline decltype(IniFile::Factory<PokemonItem>({})) Items; }

export enum struct EGenderRatio : std::uint8_t
{
	AlwaysMale,
	FemaleOneEighth,
	Female25Percent,
	Female50Percent,
	Female75Percent,
	FemaleSevenEighths,
	AlwaysFemale,
	Genderless,
};

export enum struct EGrowthRate : std::uint8_t
{
	Fast,
	Medium,
	MediumFast = Medium,
	Slow,
	Parabolic,
	MediumSlow = Parabolic,
	Erratic,
	Fluctuating,
};

export struct PokemonSpecies
{
	std::string m_Name{ "Unnamed" };
	std::string m_FormName{ "" };
	std::vector<std::string> m_Types{ "NORMAL" };
	[[nodiscard]] auto Types() const noexcept -> std::generator<PokemonType*>
	{
		for (auto&& TypeId : m_Types)
		{
			if (auto it = PBS::Types.find(TypeId); it != PBS::Types.cend())
				co_yield std::addressof(it->second);
		}
	}

	std::array<std::uint8_t, 6> m_BaseStats{ 1, 1, 1, 1, 1, 1 };
	std::uint16_t m_BaseExp{ 100 };
	std::uint8_t m_CatchRate{ 255 };
	std::uint8_t m_Happiness{ 70 };
	std::uint8_t m_Generation{ 0 };
	// 8 bit space.
	std::uint32_t m_HatchSteps{ 1 }; // Total steps, not traditional hatching cycles in official games.

	// EGenderRatio
	std::string m_GenderRatio{ "Female50Percent" };

	// EGrowthRate
	std::string m_GrowthRate{ "Medium" };

	std::vector<std::pair<std::string, std::int_fast16_t>> m_EVs{};

	std::vector<std::string> m_Abilities{};
	[[nodiscard]] auto Ability1() const noexcept -> PokemonAbility*
	{
		if (!m_Abilities.empty())
		{
			if (auto it = PBS::Abilities.find(m_Abilities[0]); it != PBS::Abilities.cend())
				return std::addressof(it->second);
		}
		return nullptr;
	}
	[[nodiscard]] auto Ability2() const noexcept -> PokemonAbility*
	{
		if (m_Abilities.size() > 1)
		{
			if (auto it = PBS::Abilities.find(m_Abilities[1]); it != PBS::Abilities.cend())
				return std::addressof(it->second);
		}
		return nullptr;
	}
	[[nodiscard]] auto Abilities() const noexcept -> std::generator<PokemonAbility*>
	{
		for (auto&& AbilityId : m_Abilities)
		{
			if (auto it = PBS::Abilities.find(AbilityId); it != PBS::Abilities.cend())
				co_yield std::addressof(it->second);
		}
	}
	std::vector<std::string> m_HiddenAbilities{};
	[[nodiscard]] auto HiddenAbility() const noexcept -> PokemonAbility*
	{
		if (!m_HiddenAbilities.empty())
		{
			if (auto it = PBS::Abilities.find(m_HiddenAbilities[0]); it != PBS::Abilities.cend())
				return std::addressof(it->second);
		}
		return nullptr;
	}
	[[nodiscard]] auto HiddenAbilities() const noexcept -> std::generator<PokemonAbility*>
	{
		for (auto&& AbilityId : m_HiddenAbilities)
		{
			if (auto it = PBS::Abilities.find(AbilityId); it != PBS::Abilities.cend())
				co_yield std::addressof(it->second);
		}
	}

	std::vector<std::pair<std::int_fast16_t, std::string>> m_Moves{};
	[[nodiscard]] auto Moves() const noexcept -> std::generator<PokemonMove*>
	{
		for (auto&& MoveId : m_Moves | std::views::elements<1>)
		{
			if (auto it = PBS::Moves.find(MoveId); it != PBS::Moves.cend())
				co_yield std::addressof(it->second);
		}
	}
	std::vector<std::string> m_TutorMoves{};
	[[nodiscard]] auto TutorMoves() const noexcept -> std::generator<PokemonMove*>
	{
		for (auto&& MoveId : m_TutorMoves)
		{
			if (auto it = PBS::Moves.find(MoveId); it != PBS::Moves.cend())
				co_yield std::addressof(it->second);
		}
	}
	std::vector<std::string> m_EggMoves{};
	[[nodiscard]] auto EggMoves() const noexcept -> std::generator<PokemonMove*>
	{
		for (auto&& MoveId : m_EggMoves)
		{
			if (auto it = PBS::Moves.find(MoveId); it != PBS::Moves.cend())
				co_yield std::addressof(it->second);
		}
	}

	std::vector<std::string> m_EggGroups{ "Undiscovered" };
	std::string m_Incense{};
	[[nodiscard]] auto Incense() const noexcept -> PokemonItem*
	{
		if (!m_Incense.empty())
		{
			if (auto it = PBS::Items.find(m_Incense); it != PBS::Items.cend())
				return std::addressof(it->second);
		}
		return nullptr;
	}
	std::vector<std::string> m_Offspring{};
	[[nodiscard]] auto Offspring() const noexcept -> std::generator<PokemonSpecies*>;

	float m_Height{ .1f }, m_Weight{ .1f };
	std::string m_Color{ "Red" };
	std::string m_Shape{ "Head" };
	std::string m_Habitat{ "None" };
	std::string m_Category{ "???" };
	std::string m_Pokedex{ "???" };

	std::vector<std::string> m_Flags{};

	std::string m_WildItemCommon{};
	std::string m_WildItemUncommon{};
	std::string m_WildItemRare{};
	[[nodiscard]] auto WildItems() const noexcept -> std::tuple<PokemonItem*, PokemonItem*, PokemonItem*>
	{
		PokemonItem* pCommon = nullptr, * pUncommon = nullptr, * pRare = nullptr;
		if (!m_WildItemCommon.empty())
		{
			if (auto it = PBS::Items.find(m_WildItemCommon); it != PBS::Items.cend())
				pCommon = std::addressof(it->second);
		}
		if (!m_WildItemUncommon.empty())
		{
			if (auto it = PBS::Items.find(m_WildItemUncommon); it != PBS::Items.cend())
				pUncommon = std::addressof(it->second);
		}
		if (!m_WildItemRare.empty())
		{
			if (auto it = PBS::Items.find(m_WildItemRare); it != PBS::Items.cend())
				pRare = std::addressof(it->second);
		}
		return { pCommon, pUncommon, pRare };
	}

	struct PokemonEvolution
	{
		std::string m_Species{};
		std::string m_Method{};
		std::string m_Parameter{};
	};
	std::vector<PokemonEvolution> m_Evolutions{};
	[[nodiscard]] auto Evolutions() const noexcept -> std::generator<PokemonSpecies*>;

	PokemonSpecies(IniBlock&& src) noexcept
		: READ_STR(Name, "Unnamed"), READ_STR(FormName, ""), READ_ARRDEF(Types, "NORMAL"), READ_VEC(BaseStats, 1, 1, 1, 1, 1, 1),
		READ_NUM(BaseExp, 100), READ_NUM(CatchRate, 255), READ_NUM(Happiness, 70), READ_NUM(HatchSteps, 1), READ_NUM(Generation, 0),
		READ_STR(GenderRatio, "Female50Percent"), READ_STR(GrowthRate, "Medium"), READ_ARR(Abilities), READ_ARR(HiddenAbilities),
		READ_ARR(TutorMoves), READ_ARR(EggMoves), READ_ARRDEF(EggGroups, "Undiscovered"), READ_STR(Incense, ""), READ_ARR(Offspring),
		READ_NUM(Height, .1f), READ_NUM(Weight, .1f), READ_STR(Color, "Red"), READ_STR(Shape, "Head"), READ_STR(Habitat, "None"), READ_STR(Category, "???"), READ_STR(Pokedex, "???"),
		READ_ARR(Flags), READ_STR(WildItemCommon, ""), READ_STR(WildItemUncommon, ""), READ_STR(WildItemRare, "")
	{
		std::ignore = std::move(src).EjectTuple<2>(
			"EVs",
			[&](auto&& arr) noexcept
			{
				m_EVs.emplace_back(
					std::string{ arr[0] },
					UTIL_StrToNum<decltype(m_EVs)::value_type::second_type>(arr[1])
				);
			}
		);

		std::ignore = std::move(src).EjectTuple<2>(
			"Moves",
			[&](auto&& arr) noexcept
			{
				m_Moves.emplace_back(
					UTIL_StrToNum<decltype(m_Moves)::value_type::first_type>(arr[0]),
					std::string{ arr[1] }
				);
			}
		);

		std::ignore = std::move(src).EjectTuple<3>(
			"Evolutions",
			[&](std::span<std::string_view const> arr) noexcept
			{
				m_Evolutions.emplace_back(
					std::string{ arr[0] },
					std::string{ arr[1] },
					std::string{ arr[2] }
				);
			}
		);
	}

#define READ_STR_B(key) m_##key{ std::move(src).EjectStr(#key, base.m_##key) }
#define READ_NUM_B(key) m_##key{ std::move(src).EjectNum<decltype(m_##key)>(#key, base.m_##key) }
#define READ_ARR_B(key) m_##key{ std::move(src).EjectArr(#key, decltype(m_##key){ base.m_##key }) }
#define READ_BOOL_B(key) m_##key{ std::move(src).EjectBool(#key, base.m_##key) }
#define READ_VEC_B(key) m_##key{ std::move(src).EjectVec(#key, decltype(m_##key){ base.m_##key }) }

	// For forms
	PokemonSpecies(PokemonSpecies const& base, IniBlock&& src) noexcept
		: READ_STR_B(Name), READ_STR_B(FormName), READ_ARR_B(Types), READ_VEC_B(BaseStats),
		READ_NUM_B(BaseExp), READ_NUM_B(CatchRate), READ_NUM_B(Happiness), READ_NUM_B(HatchSteps), READ_NUM_B(Generation),
		READ_STR_B(GenderRatio), READ_STR_B(GrowthRate), READ_ARR_B(Abilities), READ_ARR_B(HiddenAbilities),
		READ_ARR_B(TutorMoves), READ_ARR_B(EggMoves), READ_ARR_B(EggGroups), READ_STR_B(Incense), READ_ARR_B(Offspring),
		READ_NUM_B(Height), READ_NUM_B(Weight), READ_STR_B(Color), READ_STR_B(Shape), READ_STR_B(Habitat), READ_STR_B(Category), READ_STR_B(Pokedex),
		READ_ARR_B(Flags), READ_STR_B(WildItemCommon), READ_STR_B(WildItemUncommon), READ_STR_B(WildItemRare)
	{
		std::ignore = std::move(src).EjectTuple<2>(
			"EVs",
			[&](auto&& arr) noexcept
			{
				m_EVs.emplace_back(
					std::string{ arr[0] },
					UTIL_StrToNum<decltype(m_EVs)::value_type::second_type>(arr[1])
				);
			}
		).or_else(
			[&](auto&& err) noexcept -> std::expected<void, std::string_view> { m_EVs = base.m_EVs; return std::unexpected(std::move(err)); }
		);

		std::ignore = std::move(src).EjectTuple<2>(
			"Moves",
			[&](auto&& arr) noexcept
			{
				m_Moves.emplace_back(
					UTIL_StrToNum<decltype(m_Moves)::value_type::first_type>(arr[0]),
					std::string{ arr[1] }
				);
			}
		).or_else(
			[&](auto&& err) noexcept -> std::expected<void, std::string_view> { m_Moves = base.m_Moves; return std::unexpected(std::move(err)); }
		);

		std::ignore = std::move(src).EjectTuple<3>(
			"Evolutions",
			[&](std::span<std::string_view const> arr) noexcept
			{
				m_Evolutions.emplace_back(
					std::string{ arr[0] },
					std::string{ arr[1] },
					std::string{ arr[2] }
				);
			}
		).or_else(
			[&](auto&& err) noexcept -> std::expected<void, std::string_view> { m_Evolutions = base.m_Evolutions; return std::unexpected(std::move(err)); }
		);
	}

#undef READ_STR_B
#undef READ_NUM_B
#undef READ_ARR_B
#undef READ_BOOL_B
#undef READ_VEC_B

	constexpr PokemonSpecies() noexcept = default;
	constexpr PokemonSpecies(PokemonSpecies const&) noexcept = default;
	constexpr PokemonSpecies(PokemonSpecies&&) noexcept = default;
	constexpr PokemonSpecies& operator=(PokemonSpecies const&) noexcept = default;
	constexpr PokemonSpecies& operator=(PokemonSpecies&&) noexcept = default;
	constexpr virtual ~PokemonSpecies() noexcept = default;
};

namespace PBS { export inline decltype(IniFile::Factory<PokemonSpecies>({})) Species; }

auto PokemonSpecies::Offspring() const noexcept -> std::generator<PokemonSpecies*>
{
	for (auto&& SpeciesId : m_Offspring)
	{
		if (auto it = PBS::Species.find(SpeciesId); it != PBS::Species.cend())
			co_yield std::addressof(it->second);
	}
}

auto PokemonSpecies::Evolutions() const noexcept -> std::generator<PokemonSpecies*>
{
	for (auto&& evo : m_Evolutions)
	{
		if (auto it = PBS::Species.find(evo.m_Species); it != PBS::Species.cend())
			co_yield std::addressof(it->second);
	}
}

#pragma endregion Pokemon

export struct PokemonBerryPlants
{

};

namespace PBS
{
	namespace detail
	{
		template <typename T, size_t N>
		void Load(std::filesystem::path const& PbsFolder, wchar_t const (&fileName)[N], decltype(IniFile::Factory<T>({}))* output) noexcept
		{
			auto const File = PbsFolder / fileName;
			auto [buf, iBufLen] = UTIL_LoadFile(File);
			auto bufView = std::string_view{ buf.get(), iBufLen };

			if (bufView.size() > 3 && std::memcmp(bufView.data(), "\xEF\xBB\xBF", 3) == 0)
				bufView.remove_prefix(3);

			*output = IniFile::Factory<T>(bufView);
		}

		auto LoadForms(std::filesystem::path const& PbsFolder) noexcept
		{
			auto const File = PbsFolder / L"pokemon_forms.txt";
			auto [buf, iBufLen] = UTIL_LoadFile(File);
			auto bufView = std::string_view{ buf.get(), iBufLen };

			if (bufView.size() > 3 && std::memcmp(bufView.data(), "\xEF\xBB\xBF", 3) == 0)
				bufView.remove_prefix(3);

			auto Config = IniFile::Make(bufView);

			// #UPDATE_AT_CPP23 flat_map
			std::map<
				std::string,
				std::map<int, PokemonSpecies, std::less<>>,
				sv_less_t
			> ret{};

			// Copy the original species as form id == 0.
			for (auto&& [id, base] : PBS::Species)
			{
				ret[id].try_emplace(0, base);
			}

			for (auto&& IniEntry : Config.m_Entries)
			{
				std::string_view const sz{ IniEntry.m_Id };
				if (auto const pos = sz.find_first_of(','); pos != sz.npos) [[likely]]
				{
					auto const baseId = sz.substr(0, pos);
					auto const formId = UTIL_StrToNum<int>(sz.substr(pos + 1), 0);

					if (formId == 0) [[unlikely]]
					{
						std::println("[pokemon_forms.txt] Form ID 0 is reserved for base forms. Entry '{}' ignored.", sz);
						continue;
					}

					if (auto const it = ret.find(baseId); it != ret.end())
					{
						auto&& [itForm, bNew] = it->second.try_emplace(
							formId,
							// Invoking form constructor
							it->second.at(0), std::move(IniEntry)
						);

						if (!bNew)
						{
							std::println("[pokemon_forms.txt] Duplicate form ID detected: Entry '{}' ignored.", sz);
							continue;
						}
					}
				}
				else
				{
					std::println("[pokemon_forms.txt] Bad form ID format: Expected '[BASE,ID]' but '{}' received.", sz);
					continue;
				}
			}

			return ret;
		}
	}

	export inline decltype(detail::LoadForms({})) Forms;

	export void Load(std::filesystem::path const& GameRootFolder) noexcept
	{
		auto const PbsFolder = GameRootFolder / L"PBS/";

		detail::Load<PokemonType>(PbsFolder, L"types.txt", &::PBS::Types);
		detail::Load<PokemonMove>(PbsFolder, L"moves.txt", &::PBS::Moves);
		detail::Load<PokemonItem>(PbsFolder, L"items.txt", &::PBS::Items);
		detail::Load<PokemonAbility>(PbsFolder, L"abilities.txt", &::PBS::Abilities);
		detail::Load<PokemonSpecies>(PbsFolder, L"pokemon.txt", &::PBS::Species);
		Forms = detail::LoadForms(PbsFolder);	// This one is different...
	}
}
