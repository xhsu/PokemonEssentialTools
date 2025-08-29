#include <assert.h>

#include <gl/glew.h>
#include "stb_image.h"

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

// Function to load a tall texture into a texture array
static auto LoadTextureArrayFromMemory(const void* data, long data_size, int target_height, bool bFlipY = false) noexcept
	-> std::expected<std::tuple<std::uint32_t, int, int, int>, std::string_view>
{
	stbi_set_flip_vertically_on_load(bFlipY);

	// Load the source image
	int image_width = 0;
	int image_height = 0;
	auto const image_data = stbi_load_from_memory(
		(const unsigned char*)data,
		data_size,
		&image_width,
		&image_height,
		nullptr,
		4
	);

	if (image_data == nullptr)
		return std::unexpected("Failed to load image from memory");

	// Calculate how many layers we need
	auto const layers_needed = (image_height + target_height - 1) / target_height; // Ceiling division

	decltype(glGetError()) iErrorCode{};

	// Create a OpenGL texture array
	GLuint texture_array;
	glGenTextures(1, &texture_array);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// Setup filtering parameters
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// Allocate the texture array storage
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, image_width, target_height, layers_needed, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// Upload each slice
	for (int layer = 0; layer < layers_needed; ++layer)
	{
		auto const src_y_offset = layer * target_height;
		auto const actual_height = std::min(target_height, image_height - src_y_offset);
		
		if (actual_height <= 0) break;

		// Calculate the source data pointer for this layer
		auto const layer_data = image_data + (src_y_offset * image_width * 4);

		// Upload this layer
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 
					   0, 0, layer,                    // x, y, z offsets
					   image_width, actual_height, 1, // width, height, depth
					   GL_RGBA, GL_UNSIGNED_BYTE, layer_data);

		iErrorCode = glGetError();
		assert(iErrorCode == GL_NO_ERROR);

		// If this layer is smaller than target_height, we can fill the rest with transparent pixels
		if (actual_height < target_height)
		{
			// Create a buffer of transparent pixels
			std::vector<unsigned char> transparent_pixels(image_width * (target_height - actual_height) * 4, 0);
			
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
						   0, actual_height, layer,                    // x, y, z offsets
						   image_width, target_height - actual_height, 1, // width, height, depth
						   GL_RGBA, GL_UNSIGNED_BYTE, transparent_pixels.data());

			iErrorCode = glGetError();
			assert(iErrorCode == GL_NO_ERROR);
		}
	}

	stbi_image_free(image_data);

	return std::tuple{ texture_array, image_width, image_height, layers_needed };
}

// Wrapper function for loading from file
auto LoadTextureArrayFromFile(const wchar_t* file_name, int target_height, bool bFlipY = false) noexcept -> std::expected<std::tuple<std::uint32_t, int, int, int>, std::string_view>
{
	auto f = _wfopen(file_name, L"rb");
	if (f == nullptr)
		return std::unexpected("Cannot open file");

	fseek(f, 0, SEEK_END);
	auto const file_size = ftell(f);
	if (file_size < 0)
		return std::unexpected("Cannot retrieve file size");

	fseek(f, 0, SEEK_SET);
	auto file_data = std::make_unique<std::byte[]>((size_t)file_size);
	fread(file_data.get(), 1, (size_t)file_size, f);
	fclose(f);

	return LoadTextureArrayFromMemory(file_data.get(), file_size, target_height, bFlipY);
}

// Simple helper function to load an image into a OpenGL texture with common settings
static auto LoadTextureFromMemory(const void* data, long data_size, bool bFlipY = false) noexcept
	-> std::expected<std::tuple<std::uint32_t, int, int>, std::string_view>
{
	// GL Y is different from image Y for the most of time.
	// GL coord(0, 0) is the center, while image coord(0, 0) is the top-left corner.
	stbi_set_flip_vertically_on_load(bFlipY);

	// Load from file
	int image_width = 0;
	int image_height = 0;
	auto const image_data = stbi_load_from_memory(
		(const unsigned char*)data,
		data_size,
		&image_width,
		&image_height,
		nullptr,
		4
	);

	if (image_data == nullptr)
		return std::unexpected("Fail to load to memory");

	decltype(glGetError()) iErrorCode{};

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// 为当前绑定的纹理对象设置环绕、过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	iErrorCode = glGetError();
	assert(iErrorCode == GL_NO_ERROR);

	// Upload pixels into texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	iErrorCode = glGetError();
	//assert(iErrorCode == GL_NO_ERROR);

	return std::tuple{ image_texture, image_width, image_height, };
}

// Open and read a file, then forward to LoadTextureFromMemory()
auto LoadTextureFromFile(const wchar_t* file_name, bool bFlipY) noexcept -> std::expected<std::tuple<std::uint32_t, int, int>, std::string_view>
{
	auto f = _wfopen(file_name, L"rb");
	if (f == nullptr)
		return std::unexpected("Cannot open file");

	fseek(f, 0, SEEK_END);
	auto const file_size = ftell(f);
	if (file_size < 0)
		return std::unexpected("Cannot retrieve file size");

	fseek(f, 0, SEEK_SET);
	auto file_data = std::make_unique<std::byte[]>((size_t)file_size);
	fread(file_data.get(), 1, (size_t)file_size, f);
	fclose(f);

	return LoadTextureFromMemory(file_data.get(), file_size, bFlipY);
}

auto GetTextureDimensions(const wchar_t* file_name) noexcept -> std::pair<int, int>
{
	std::ifstream file{ file_name, std::ios::binary };

	if (!file.is_open()) [[unlikely]]
	{
		std::filesystem::path const FilePath{ file_name };
		std::cerr << "Error: Could not open file " << FilePath.u8string() << std::endl;
		return { 0, 0 };
	}

	static constexpr unsigned char PNG_SIGNATURE[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	unsigned char signature[8]{};
	file.read(reinterpret_cast<char*>(signature), sizeof(signature));

	if (std::memcmp(signature, PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) != 0)	[[unlikely]]
	{
		std::cerr << "Error: Not a valid PNG file." << std::endl;
		return { 0, 0 };
	}

	file.seekg(16, std::ios::beg);	// PNG width and height are stored at offset 16

	unsigned char buf[8]{};
	file.read(reinterpret_cast<char*>(&buf), 8);

	std::uint32_t iWidth = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
	std::uint32_t iHeight = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7] << 0);

	return { std::bit_cast<int>(iWidth), std::bit_cast<int>(iHeight) };
}
