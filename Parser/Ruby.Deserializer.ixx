module;

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Ruby.Deserializer;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import UtlString;

template <typename... Tys>
auto any_of_cast(std::any const* AnyObj, auto def) noexcept -> decltype(def)
{
	std::tuple<std::add_pointer_t<Tys const>...> const casting_result{
		std::any_cast<Tys>(AnyObj)...
	};

	// #UPDATE_AT_CPP26 template for

	auto fn = [&]<size_t I>() noexcept -> bool
	{
		if (auto ptr = std::get<I>(casting_result); ptr != nullptr)
		{
			if constexpr (requires { def = static_cast<decltype(def)>(*ptr); })
				def = static_cast<decltype(def)>(*ptr);
			else if constexpr (requires { def = decltype(def){ *ptr }; })
				def = decltype(def){ *ptr };
			else
				static_assert(false, "Can't convert to type of def");

			return true;
		}

		return false;
	};

	[&] <size_t... I>(std::index_sequence<I...>) noexcept {
		(fn.template operator()<I>() || ...);
	}(std::index_sequence_for<Tys...>{});

	return def;
}

namespace Ruby::Deserializer
{
	export struct Color
	{
		double red{};
		double green{};
		double blue{};
		double alpha{};
	};

	export struct Object
	{
		std::string m_Name;

		std::map<std::string, std::any, sv_less_t> m_List;

		template <typename T>
		auto Find(std::string_view key, T def = {}) const noexcept -> T
		{
			if (auto it = m_List.find(key); it != m_List.cend())
			{
				if constexpr (requires { typename T::value_type; } && !std::is_same_v<std::string, std::remove_cvref_t<T>>)
				{
					if (auto ptr = std::any_cast<std::vector<std::any>*>(&it->second); ptr != nullptr && *ptr != nullptr)
					{
						std::vector<std::any>& arrref = **ptr;
						std::vector<typename T::value_type> ret{};

						for (auto&& elem : arrref)
						{
							if (auto elemptr = std::any_cast<typename T::value_type>(&elem); elemptr != nullptr)
								ret.emplace_back(*elemptr);
							else if (auto elemptr = std::any_cast<typename T::value_type*>(&elem); elemptr != nullptr && *elemptr != nullptr)
								ret.emplace_back(**elemptr);
						}

						if (ret.empty())
							ret.append_range(std::move(def));

						return ret;
					}
				}
				else if constexpr (std::integral<T>)
				{
					using namespace std;
					return any_of_cast<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t>(&it->second, def);
				}
				else
				{
					if (auto ptr = std::any_cast<T>(&it->second); ptr != nullptr)
						return *ptr;
					else if (auto ptr = std::any_cast<T*>(&it->second); ptr != nullptr && *ptr != nullptr)
						return **ptr;
				}
			}

			return std::move(def);
		}

		void Print() const noexcept
		{
			for (auto&& [szName, AnyObj] : m_List)
			{
				if (auto ptr = std::any_cast<int>(&AnyObj); ptr != nullptr)
					std::println("{}: '{}' -> '{}'", szName, *ptr, AnyObj.type().name());
				else if (auto ptr = std::any_cast<float>(&AnyObj); ptr != nullptr)
					std::println("{}: '{}' -> '{}'", szName, *ptr, AnyObj.type().name());
				else if (auto ptr = std::any_cast<std::string*>(&AnyObj); ptr != nullptr)
					std::println("{}: '{}' -> '{}'", szName, *ptr == nullptr ? "nullptr" : **ptr, AnyObj.type().name());
				else
					std::println("{}: 'unknown' -> '{}'", szName, AnyObj.type().name());
			}
		}
	};

	export struct Table
	{
		std::int32_t x_size{};
		std::int32_t y_size{};
		std::int32_t z_size{};

		std::vector<std::int16_t> data;
	};

	export struct Tone
	{
		double red{};
		double green{};
		double blue{};
		double grey{};
	};

	export class Reader
	{
	public:
		Reader(std::ifstream& file) noexcept : m_file{ &file } {}

		std::any parse();

	private:
		std::ifstream* m_file;

		std::deque<std::any> m_object_cache;
		std::deque<std::vector<std::uint8_t>> m_symbol_cache;
	};
}
