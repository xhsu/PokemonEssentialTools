module;

#include <stdio.h>

export module UtlFile;

import std;

export auto UTIL_LoadFile(std::filesystem::path const& Path) noexcept -> std::pair<std::unique_ptr<char[]>, size_t>
{
	if (auto f = _wfopen(Path.c_str(), L"rb"); f != nullptr)
	{
		std::fseek(f, 0, SEEK_END);
		auto const iLength = (size_t)std::ftell(f);

		auto buf = std::make_unique<char[]>(iLength + 1);
		std::fseek(f, 0, SEEK_SET);

		std::fread(buf.get(), sizeof(char), iLength, f);
		buf[iLength] = '\0';

		std::fclose(f);
		return { std::move(buf), iLength };
	}

	return { nullptr, 0 };
}
