#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import UtlString;

import Database.Raw.PBS;

#define PORT_SIMPLE(key)			m_##key{ Raw.m_##key }
#define PORT_ENUM(key, def, ...)	m_##key{ EnumDeserialize<__VA_ARGS__>(Raw.m_##key).value_or(def) }
#define PORT_CLASS(key, lib)		m_##key{ std::addressof(lib.at(Raw.m_##key)) }

template <auto FirstEnumValue, auto... RestEnumValues>
[[nodiscard]] constexpr auto EnumDeserialize(std::string_view sz) noexcept// -> std::optional<decltype(FirstEnumValue)>
{
	static_assert(std::conjunction_v<std::is_enum<decltype(FirstEnumValue)>, std::is_same<decltype(FirstEnumValue), decltype(RestEnumValues)>...>);

	static constexpr std::string_view FuncSigText{ __FUNCSIG__ };
	static constexpr auto pos0 = FuncSigText.find_first_of('<') + 1, pos1 = FuncSigText.find_first_of('>');
	static constexpr auto TemplateArgsText = FuncSigText.substr(pos0, pos1 - pos0);
	static constexpr auto TemplateArgsSplited = []()
	{
		std::array<std::string_view, sizeof...(RestEnumValues) + 1> res;
		size_t posLast = 0, posCur = 0, idx = 0;
		while ((posCur = TemplateArgsText.find_first_of(',', posLast)) != std::string_view::npos)
		{
			res[idx++] = TemplateArgsText.substr(posLast, posCur - posLast);
			posLast = posCur + 1;
		}
		res[idx] = TemplateArgsText.substr(posLast);
		return res;
	}();
	static constexpr std::array Candidates{ FirstEnumValue, RestEnumValues... };

	std::optional<decltype(FirstEnumValue)> result{ std::nullopt };

	for (auto&& [idx, text] : std::views::enumerate(TemplateArgsSplited))
	{
		auto const scope_resolution_pos = text.find("::");
		auto const identifier = scope_resolution_pos == std::string_view::npos ? text : text.substr(scope_resolution_pos + 2);

		if (identifier == sz)
		{
			result = Candidates[idx];
			break;
		}
	}

	return result;
}

namespace Database::PBS
{
	template <typename T>
	[[nodiscard]] auto BuildFromRaw(auto&& Source) noexcept -> std::map<std::string_view, T, sv_less_t>
	{
		std::map<std::string_view, T, sv_less_t> ret{};

		for (auto&& [NameId, RawData] : Source)
		{
			auto&& [it, bNew] = ret.try_emplace(
				NameId,
				RawData
			);

			if (!bNew) [[unlikely]]
				std::println("[{}] Duplicated id name '{}', the later is ignored.", __FUNCTION__, NameId);
		}

		return ret;
	}



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

		CPokemonType(::PokemonType const& Raw) noexcept
			: PORT_SIMPLE(Name), PORT_SIMPLE(IconPosition), PORT_SIMPLE(IsSpecialType), PORT_SIMPLE(IsPseudoType), PORT_SIMPLE(Flags),
			m_Raw{ &Raw }
		{

		}

		CPokemonType(CPokemonType const&) noexcept = delete;
		CPokemonType(CPokemonType&&) noexcept = delete;
		CPokemonType& operator=(CPokemonType const&) = delete;
		CPokemonType& operator=(CPokemonType&&) = delete;
		~CPokemonType() noexcept = default;
	};

	[[nodiscard]] auto BuildPokemonTypes() noexcept -> decltype(BuildFromRaw<CPokemonType>(::PBS::Types))
	{
		auto ret = BuildFromRaw<CPokemonType>(::PBS::Types);

		// Link references.

		for (auto&& [NameId, Type] : ret)
		{
			auto const fnLinker =
				[&](std::vector<struct CPokemonType const*> CPokemonType::* lhs, std::vector<std::string> PokemonType::* rhs) noexcept
				{
					for (auto&& TypeName : Type.m_Raw->*rhs)
					{
						try
						{
							(Type.*lhs).push_back(&ret.at(TypeName));
						}
						catch (...)
						{
							std::println("[BuildPokemonTypes] Assumed type '{}' referenced in type '{}' but has no definition.", TypeName, NameId);
						}
					}
				};

			fnLinker(&CPokemonType::m_Weaknesses, &PokemonType::m_Weaknesses);
			fnLinker(&CPokemonType::m_Resistances, &PokemonType::m_Resistances);
			fnLinker(&CPokemonType::m_Immunities, &PokemonType::m_Immunities);
		}

		return ret;
	}

	inline decltype(BuildPokemonTypes()) Types;



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

		CPokemonMove(::PokemonMove const& Raw) noexcept : PORT_SIMPLE(Name), PORT_CLASS(Type, Types),
			PORT_ENUM(Category, EMoveCategory::Status,
				EMoveCategory::Physical, EMoveCategory::Special, EMoveCategory::Status),
			PORT_SIMPLE(Power), PORT_SIMPLE(Accuracy), PORT_SIMPLE(TotalPP),
			PORT_SIMPLE(Priority), PORT_SIMPLE(EffectChance),
			PORT_ENUM(Target, EMoveTarget::None,
				EMoveTarget::None, EMoveTarget::User, EMoveTarget::NearAlly, EMoveTarget::UserOrNearAlly, EMoveTarget::AllAllies, EMoveTarget::UserAndAllies,
				EMoveTarget::NearFoe, EMoveTarget::RandomNearFoe, EMoveTarget::AllNearFoes, EMoveTarget::Foe, EMoveTarget::AllFoes,
				EMoveTarget::NearOther, EMoveTarget::AllNearOthers, EMoveTarget::Other, EMoveTarget::AllBattlers,
				EMoveTarget::UserSide, EMoveTarget::FoeSide, EMoveTarget::BothSides),
			PORT_SIMPLE(FunctionCode), PORT_SIMPLE(Flags), PORT_SIMPLE(Description)
		{
		}

		CPokemonMove(CPokemonMove const&) noexcept = default;
		CPokemonMove(CPokemonMove&&) noexcept = default;
		CPokemonMove& operator=(CPokemonMove const&) noexcept = default;
		CPokemonMove& operator=(CPokemonMove&&) noexcept = default;
		~CPokemonMove() noexcept = default;
	};

	inline decltype(BuildFromRaw<CPokemonMove>(::PBS::Moves)) Moves;



	struct CPokemonAbility final
	{
		std::string_view m_Name{ "Unnamed" };
		std::string_view m_Description{ "???" };
		std::span<std::string const> m_Flags{};

		CPokemonAbility(::PokemonAbility const& Raw) noexcept
			: PORT_SIMPLE(Name), PORT_SIMPLE(Description), PORT_SIMPLE(Flags)
		{
		}

		constexpr CPokemonAbility() noexcept = default;
		constexpr CPokemonAbility(CPokemonAbility const&) noexcept = default;
		constexpr CPokemonAbility(CPokemonAbility&&) noexcept = default;
		constexpr CPokemonAbility& operator=(CPokemonAbility const&) noexcept = default;
		constexpr CPokemonAbility& operator=(CPokemonAbility&&) noexcept = default;
		constexpr ~CPokemonAbility() noexcept = default;
	};

	inline decltype(BuildFromRaw<CPokemonAbility>(::PBS::Abilities)) Abilities;



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
		CPokemonItem(::PokemonItem const& Raw) noexcept
			: PORT_SIMPLE(Name), PORT_SIMPLE(NamePlural), PORT_SIMPLE(PortionName), PORT_SIMPLE(PortionNamePlural),
			PORT_SIMPLE(Pocket), PORT_SIMPLE(BPPrice), PORT_SIMPLE(Price), PORT_SIMPLE(SellPrice),
			PORT_ENUM(FieldUse, EFieldUse::None,
				EFieldUse::None, EFieldUse::OnPokemon, EFieldUse::Direct, EFieldUse::TR, EFieldUse::TM, EFieldUse::HM),
			PORT_ENUM(BattleUse, EBattleUse::None,
				EBattleUse::None, EBattleUse::OnPokemon, EBattleUse::OnMove, EBattleUse::OnBattler, EBattleUse::OnFoe, EBattleUse::Direct),
			PORT_SIMPLE(Flags), PORT_SIMPLE(Consumable), PORT_SIMPLE(ShowQuantity), PORT_CLASS(Move, Moves), PORT_SIMPLE(Description)
		{

		}

		constexpr CPokemonItem() noexcept = default;
		constexpr CPokemonItem(CPokemonItem const&) noexcept = default;
		constexpr CPokemonItem(CPokemonItem&&) noexcept = default;
		constexpr CPokemonItem& operator=(CPokemonItem const&) noexcept = default;
		constexpr CPokemonItem& operator=(CPokemonItem&&) noexcept = default;
		constexpr ~CPokemonItem() noexcept = default;
	};

	inline decltype(BuildFromRaw<CPokemonItem>(::PBS::Items)) Items;



	struct CPokemonEvolution final
	{
		struct CPokemonSpecies const* m_Species{};
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

		std::vector<PokemonAbility const*> m_Abilities{};
		std::vector<PokemonAbility const*> m_HiddenAbilities{};

		std::vector<std::pair<std::int_fast16_t, PokemonMove const*>> m_Moves{};
		std::vector<PokemonMove const*> m_TutorMoves{};
		std::vector<PokemonMove const*> m_EggMoves{};

		std::span<std::string const> m_EggGroups{};	// "Undiscovered"
		PokemonItem const* m_Incense{};

		std::vector<PokemonSpecies const*> m_Offspring{};

		float m_Height{ .1f }, m_Weight{ .1f };
		std::string_view m_Color{ "Red" };
		std::string_view m_Shape{ "Head" };
		std::string_view m_Habitat{ "None" };
		std::string_view m_Category{ "???" };
		std::string_view m_Pokedex{ "???" };

		std::span<std::string const> m_Flags{};

		PokemonItem const* m_WildItemCommon{};
		PokemonItem const* m_WildItemUncommon{};
		PokemonItem const* m_WildItemRare{};

		std::vector<CPokemonEvolution> m_Evolutions{};
	};

	void Build() noexcept
	{
		Types = BuildPokemonTypes();
		Moves = BuildFromRaw<CPokemonMove>(::PBS::Moves);
		Abilities = BuildFromRaw<CPokemonAbility>(::PBS::Abilities);
		Items = BuildFromRaw<CPokemonItem>(::PBS::Items);
	}
}
