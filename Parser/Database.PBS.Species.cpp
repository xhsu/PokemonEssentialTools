#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import UtlString;

import Database.PBS;
import Database.Raw.PBS;

#define PORT_SIMPLE(key)			m_##key{ Raw.m_##key }
#define PORT_ENUM(key, def, ...)	m_##key{ EnumDeserialize<__VA_ARGS__>(Raw.m_##key).value_or(def) }
#define PORT_CLASS(key, lib)		m_##key{ std::addressof(lib.at(Raw.m_##key)) }
#define PORT_COLL(key, lib, Class) \
			m_##key{																															\
				std::from_range,																												\
				Raw.m_##key																														\
				| std::views::transform(																										\
					[](auto&& a) static noexcept																								\
					{																															\
						auto it = lib.find(a);																									\
																																				\
						if (it != lib.cend())																									\
							return std::addressof(it->second);																					\
						else																													\
						{																														\
							std::println("[{}::`ctor] Assumed class '{}' referenced but cannot linked.", typeid(Class).name(), a);				\
							return (decltype(std::addressof(it->second)))nullptr;																\
						}																														\
					}																															\
				)																																\
			}
#define PORT_OPTIONAL(key, lib) 	m_##key{ Raw.m_##key.empty() ? nullptr : std::addressof(lib.at(Raw.m_##key)) }
#define PORT_LOC_OWNED(key)			m_##key{ std::from_range, Raw.m_##key | std::views::transform([](auto&& a) static noexcept { return decltype(m_##key)::value_type{std::forward<decltype(a)>(a) }; }) }



// #UPDATE_AT_CPP26 reflecion
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

	// Pokemon Types

	CPokemonType::CPokemonType(::PokemonType const& Raw) noexcept
		: PORT_SIMPLE(Name), PORT_SIMPLE(IconPosition), PORT_SIMPLE(IsSpecialType), PORT_SIMPLE(IsPseudoType), PORT_SIMPLE(Flags),
		m_Raw{ &Raw }
	{

	}

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

	// Pokemon Moves

	CPokemonMove::CPokemonMove(::PokemonMove const& Raw) noexcept : PORT_SIMPLE(Name), PORT_CLASS(Type, Types),
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

	// Pokemon Abilities

	CPokemonAbility::CPokemonAbility(::PokemonAbility const& Raw) noexcept
		: PORT_SIMPLE(Name), PORT_SIMPLE(Description), PORT_SIMPLE(Flags)
	{
	}

	// Pokemon Items

	CPokemonItem::CPokemonItem(::PokemonItem const& Raw) noexcept
		: PORT_SIMPLE(Name), PORT_SIMPLE(NamePlural), PORT_SIMPLE(PortionName), PORT_SIMPLE(PortionNamePlural),
		PORT_SIMPLE(Pocket), PORT_SIMPLE(BPPrice), PORT_SIMPLE(Price), PORT_SIMPLE(SellPrice),
		PORT_ENUM(FieldUse, EFieldUse::None,
			EFieldUse::None, EFieldUse::OnPokemon, EFieldUse::Direct, EFieldUse::TR, EFieldUse::TM, EFieldUse::HM),
		PORT_ENUM(BattleUse, EBattleUse::None,
			EBattleUse::None, EBattleUse::OnPokemon, EBattleUse::OnMove, EBattleUse::OnBattler, EBattleUse::OnFoe, EBattleUse::Direct),
		PORT_SIMPLE(Flags), PORT_SIMPLE(Consumable), PORT_SIMPLE(ShowQuantity), PORT_OPTIONAL(Move, Moves), PORT_SIMPLE(Description)
	{
	}

	// Pokemon Species/Forms

	CPokemonSpecies::CPokemonSpecies(::PokemonSpecies const& Raw) noexcept
		: PORT_SIMPLE(Name), PORT_SIMPLE(FormName),
		PORT_COLL(Types, Types, CPokemonSpecies),
		PORT_SIMPLE(BaseStats), PORT_SIMPLE(BaseExp), PORT_SIMPLE(CatchRate), PORT_SIMPLE(Happiness), PORT_SIMPLE(Generation), PORT_SIMPLE(HatchSteps),
		PORT_ENUM(GenderRatio, EGenderRatio::Female50Percent,
			EGenderRatio::AlwaysMale, EGenderRatio::FemaleOneEighth, EGenderRatio::Female25Percent, EGenderRatio::Female50Percent,
			EGenderRatio::Female75Percent, EGenderRatio::FemaleSevenEighths, EGenderRatio::AlwaysFemale, EGenderRatio::Genderless),
		PORT_ENUM(GrowthRate, EGrowthRate::Medium,
			EGrowthRate::Fast, EGrowthRate::Medium, EGrowthRate::Slow, EGrowthRate::MediumFast,
			EGrowthRate::Parabolic, EGrowthRate::MediumSlow, EGrowthRate::Erratic, EGrowthRate::Fluctuating),
		PORT_SIMPLE(EVs),
		PORT_COLL(Abilities, Abilities, CPokemonSpecies), PORT_COLL(HiddenAbilities, Abilities, CPokemonSpecies),
		PORT_COLL(TutorMoves, Moves, CPokemonSpecies), PORT_COLL(EggMoves, Moves, CPokemonSpecies),
		PORT_SIMPLE(EggGroups), PORT_OPTIONAL(Incense, Items), PORT_SIMPLE(Offspring),
		PORT_SIMPLE(Height), PORT_SIMPLE(Weight), PORT_SIMPLE(Color), PORT_SIMPLE(Shape), PORT_SIMPLE(Habitat), PORT_SIMPLE(Category), PORT_SIMPLE(Pokedex),
		PORT_SIMPLE(Flags), PORT_OPTIONAL(WildItemCommon, Items), PORT_OPTIONAL(WildItemUncommon, Items), PORT_OPTIONAL(WildItemRare, Items), PORT_SIMPLE(NationalDex),
		m_Raw{ &Raw }
	{
		// Convert Moves array to level-move pairs with pointers
		for (auto&& [Level, MoveName] : Raw.m_Moves)
		{
			try
			{
				m_Moves.emplace_back(Level, &Moves.at(MoveName));
			}
			catch (...)
			{
				std::println("[CPokemonSpecies] Assumed move '{}' referenced in species '{}' but has no definition.", MoveName, Raw.m_Name);
			}
		}
	}

	[[nodiscard]] auto BuildPokemonSpecies() noexcept
	{
		//auto ret = BuildFromRaw<CPokemonSpecies>(::PBS::Species);

		std::map<
			std::string_view,
			std::map<int, CPokemonSpecies, std::less<>>,
			sv_less_t
		> ret{};

		for (auto&& [NameId, FormsMap] : ::PBS::Forms)
		{
			auto&& [it, bNew] = ret.try_emplace(NameId);

			if (!bNew) [[unlikely]]
				std::println("[{}] Duplicated id name '{}', the later is ignored.", __FUNCTION__, NameId);
			else
			{
				auto& FormsMapOut = it->second;

				for (auto&& [iId, Form] : FormsMap)
				{
					auto&& [it2, bNew2] = FormsMapOut.try_emplace(
						iId,
						Form
					);
				}
			}
		}

		// Link references.

		for (auto&& [NameId, FormMap] : ret)
		{
			for (auto&& [iId, Spec] : FormMap)
			{
				// Evolutions

				for (auto&& Evo : Spec.m_Raw->m_Evolutions)
				{
					try
					{
						Spec.m_Evolutions.push_back(
							CPokemonEvolution{
								.m_Species{ Evo.m_Species },
								.m_Method{ Evo.m_Method },
								.m_Parameter{ Evo.m_Parameter },
							}
						);
					}
					catch (...)
					{
						std::println("[BuildPokemonSpecies] Assumed species '{}' referenced in evolution of species '{}' but has no definition.", Evo.m_Species, NameId);
					}
				}
			}
		}

		return ret;
	}

	void Build() noexcept
	{
		Types = BuildPokemonTypes();
		Moves = BuildFromRaw<CPokemonMove>(::PBS::Moves);
		Abilities = BuildFromRaw<CPokemonAbility>(::PBS::Abilities);
		Items = BuildFromRaw<CPokemonItem>(::PBS::Items);
		Species = BuildPokemonSpecies();
	}
}
