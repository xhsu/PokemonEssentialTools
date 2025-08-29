#ifndef __INTELLISENSE__
import std.compat;
#else
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

import UtlString;
import Ruby.Deserializer;


std::any Ruby::Deserializer::Reader::parse()
{
	char const cTypeSymbol = (char)m_file->get();
	if (m_file->eof())
		throw std::runtime_error("Unexpected EOF");

#define ios_failure(x) throw std::runtime_error(x)

	auto read_fixnum = [&]() -> std::int32_t
		{
			std::uint32_t x;
			std::int8_t c = m_file->get();

			if (c == 0)
				return 0;
			if (c > 0)
			{
				if (4 < c && c < 128)
					return c - 5;
				if (c > sizeof(std::int32_t))
					ios_failure("Fixnum too big: " + std::to_string(c));

				x = 0;
				for (int i = 0; i < 4; i++)
					x = (i < c ? m_file->get() << 24 : 0) | (x >> 8);
			}
			else
			{
				if (-129 < c && c < -4)
					return c + 5;
				c = -c;
				if (c > sizeof(std::int32_t))
					ios_failure("Fixnum too big: " + std::to_string(c));

				x = -1;
				static constexpr std::uint32_t mask = ~(0xff << 24);
				for (int i = 0; i < 4; i++)
					x = (i < c ? m_file->get() << 24 : 0xff) | ((x >> 8) & mask);
			}

			return std::bit_cast<std::int32_t>(x);
		};

	switch (cTypeSymbol)
	{
	case '@':    // Link
	{
		std::int32_t index = read_fixnum();
		auto it = m_object_cache.begin();
		std::ranges::advance(it, index, m_object_cache.end());
		return *it;	// copy
	}
	case 'I':    // Instance Variables https://docs.ruby-lang.org/en/3.2/marshal_rdoc.html
	{
		auto name = parse();
		if (name.type() != typeid(std::string*))
			ios_failure(std::string("Unsupported IVar Type: ") + name.type().name());

		auto length = read_fixnum();

		auto& strref_any = m_object_cache.emplace_back(std::move(name));
		auto& strref = **std::any_cast<std::string*>(&strref_any);

		for (std::int32_t i = 0; i < length; i++)
		{
			std::string key;
			{
				auto akey = parse();
				if (akey.type() != typeid(std::vector<std::uint8_t>*))
					ios_failure(std::string("IVar key not symbol: ") + akey.type().name());

				auto vec = std::any_cast<std::vector<std::uint8_t> *>(akey);
				key = std::string(vec->begin(), vec->end());
			}

			auto value = parse();

			// #TODO: Do something with the character encoding
			//std::println("I : key: '{}', value: '{}'", key, value.type().name());
		}

		return &strref;
	}
	case 'e':    // Extended
		ios_failure("Not yet Implemented: Extended");
	case 'C':    // UClass
		ios_failure("Not yet Implemented: UClass");
	case '0':    // Nil
		return nullptr;
	case 'T':    // True
		return true;
	case 'F':    // False
		return false;
	case 'i':    // Fixnum
		return read_fixnum();
	case 'f':    // Float
	{
		auto str = std::string(read_fixnum(), '\0');
		m_file->read(str.data(), str.length());

		double v;
		if (str.compare("nan") == 0)
			v = std::numeric_limits<double>::quiet_NaN();
		if (str.compare("inf") == 0)
			v = std::numeric_limits<double>::infinity();
		if (str.compare("-inf") == 0)
			v = -std::numeric_limits<double>::infinity();
		else
			v = UTIL_StrToNum<double>(str);

		m_object_cache.push_back(v);
		return v;
	}
	case 'l':    // Bignum
	{
		int  sign = m_file->get();
		auto len = read_fixnum();

		//char data[len * 2];
		auto data = std::make_unique<char[]>(len * 2);
		m_file->read(data.get(), len * 2);

		// #TODO

		ios_failure("Not yet Implemented: Bignum");
	}
	case '"':    // String
	{
		auto const len = read_fixnum();
		auto str = std::string(len, '\0');
		m_file->read(str.data(), len);

		auto& strref_any = m_object_cache.emplace_back(std::move(str));
		auto& strref = *std::any_cast<std::string>(&strref_any);
		return &strref;
	}
	case '/':    // RegExp
		ios_failure("Not yet Implemented: RegExp");
	case '[':    // Array
	{
		std::vector<std::any> arr;
		std::int32_t len = read_fixnum();
		arr.reserve(len);

		auto& arrref_any = m_object_cache.emplace_back(std::move(arr));
		auto& arrref = *std::any_cast<decltype(arr)>(&arrref_any);

		for (long i = 0; i < len; i++)
			arrref.push_back(parse());

		return &arrref;
	}
	case '{':    // Hash
	{
		using RubyMap = std::map<std::int32_t, std::any, std::less<>>;

		auto const length = read_fixnum();

		auto& mapref_any = m_object_cache.emplace_back(std::make_any<RubyMap>());
		auto& mapref = *std::any_cast<RubyMap>(&mapref_any);

		for (std::int32_t i = 0; i < length; i++)
		{
			auto key = parse();
			if (key.type() != typeid(std::int32_t))
				ios_failure(std::string("Hash key not Fixnum: ") + key.type().name());

			auto value = parse();

			mapref.try_emplace(
				std::any_cast<std::int32_t>(key),
				std::move(value)
			);
		}

		return &mapref;
	}
	case '}':    // HashDef
		ios_failure("Not yet Implemented: HashDef");
	case 'S':    // Struct
		ios_failure("Not yet Implemented: Struct");
	case 'u':    // UserDef
	{
		std::string_view name{};
		auto pname = parse();
		if (pname.type() == typeid(std::vector<std::uint8_t>*))
		{
			auto& name_strref =
				*std::any_cast<std::vector<std::uint8_t>*>(std::move(pname));

			name = std::string_view{
				(char const*)name_strref.data(),
				name_strref.size()
			};
		}
		else
			ios_failure(std::string("UserDef name not Symbol"));

		auto size = read_fixnum();

		if (name.compare("Color") == 0)
		{
			auto& color_any = m_object_cache.emplace_back(std::make_any<Color>());
			auto& color = *std::any_cast<Color>(&color_any);

			m_file->read((char*)&color.red, sizeof(double));
			m_file->read((char*)&color.green, sizeof(double));
			m_file->read((char*)&color.blue, sizeof(double));
			m_file->read((char*)&color.alpha, sizeof(double));

			return &color;
		}
		else if (name.compare("Table") == 0)
		{
			auto& table_any = m_object_cache.emplace_back(std::make_any<Table>());
			auto& table = *std::any_cast<Table>(&table_any);

			m_file->seekg(4, std::ios::cur);
			m_file->read((char*)&table.x_size, 4);
			m_file->read((char*)&table.y_size, 4);
			m_file->read((char*)&table.z_size, 4);

			std::int32_t size;
			m_file->read((char*)&size, sizeof(std::int32_t));

			table.data.resize(size);
			for (std::int32_t i = 0; i < size; i++)
				m_file->read((char*)&table.data[i], sizeof(std::int16_t));

			return &table;
		}
		else if (name.compare("Tone") == 0)
		{
			auto& tone_any = m_object_cache.emplace_back(std::make_any<Tone>());
			auto& tone = *std::any_cast<Tone>(&tone_any);

			m_file->read((char*)&tone.red, sizeof(double));
			m_file->read((char*)&tone.green, sizeof(double));
			m_file->read((char*)&tone.blue, sizeof(double));
			m_file->read((char*)&tone.grey, sizeof(double));

			return &tone;
		}
		else
			ios_failure(std::format("Unsupported user defined class: {}", name));
	}
	case 'U':    // User Marshal
		ios_failure("Not yet Implemented: User Marshal");
	case 'o':    // Object
	{
		Object o;
		auto name = parse();
		if (name.type() == typeid(std::vector<std::uint8_t>*))
		{
			auto vec = std::any_cast<std::vector<std::uint8_t> *>(name);
			o.m_Name.append_range(*vec);
		}
		else
			ios_failure(std::string("Object name not Symbol"));

		[[maybe_unused]] auto const length = read_fixnum();
		//o.m_List.reserve(length);	// maybe in std::flat_map?

		auto& objref_any = m_object_cache.emplace_back(o);
		auto& objref = *std::any_cast<Object>(&objref_any);

		for (std::int32_t i = 0; i < length; i++)
		{
			std::string key;
			{
				auto akey = parse();
				if (akey.type() != typeid(std::vector<std::uint8_t>*))
					ios_failure(std::string("Object key not symbol: ") + akey.type().name());

				auto vec = std::any_cast<std::vector<std::uint8_t> *>(akey);
				key = std::string(vec->begin(), vec->end());
			}

			if (key[0] != '@')
				ios_failure("Object Key not instance variable name");

			auto value = parse();

			objref.m_List.insert(std::pair{ std::move(key), std::move(value) });
		}

		return &objref;
	}
	case 'd':    // Data
		ios_failure("Not yet Implemented: Data");
	case 'M':    // Module Old
	case 'c':    // Class
	case 'm':    // Module
		ios_failure("Not yet Implemented: Module/Class");
	case ':':    // Symbol
	{
		std::vector<std::uint8_t> arr;
		std::int32_t len = read_fixnum();

		arr.resize(len);
		m_file->read((char*)arr.data(), len);

		auto& arrref = m_symbol_cache.emplace_back(std::move(arr));
		return &arrref;
	}
	case ';':    // Symlink
	{
		std::int32_t index = read_fixnum();
		auto it = m_symbol_cache.begin();
		std::ranges::advance(it, index, m_symbol_cache.end());
		return &(*it);	// ref
	}
	default:
		ios_failure("Unknown Value: " + std::to_string(cTypeSymbol));
	}

	return nullptr;
}
