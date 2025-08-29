module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module Image.Tilesets;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import GL.Canvas;
import GL.Painter;
import GL.Shader;
import GL.Texture;

extern "C++" auto LoadTextureFromFile(const wchar_t* file_name, bool bFlipY) noexcept -> std::expected<std::tuple<std::uint32_t, int, int>, std::string_view>;
extern "C++" auto LoadTextureArrayFromFile(const wchar_t* file_name, int target_height, bool bFlipY = false) noexcept -> std::expected<std::tuple<std::uint32_t, int, int, int>, std::string_view>;

export struct CTileUV final
{
	float m_x{}, m_y{};
	float m_flWidth{}, m_flHeight{};

	[[nodiscard]] inline constexpr auto TopLeft() const noexcept -> std::array<float, 2>
	{
		return { m_x, m_y };
	}

	[[nodiscard]] inline constexpr auto BottomRight() const noexcept -> std::array<float, 2>
	{
		return { m_x + m_flWidth, m_y + m_flHeight };
	}

	[[nodiscard]] inline constexpr auto TopRight() const noexcept -> std::array<float, 2>
	{
		return { m_x + m_flWidth, m_y };
	}

	[[nodiscard]] inline constexpr auto BottomLeft() const noexcept -> std::array<float, 2>
	{
		return { m_x, m_y + m_flHeight };
	}

	inline constexpr void Apply(std::span<std::span<float> const> square, int iDropCount = 0) const noexcept
	{
		if (square.size() < 4)
			return;

		for (auto&& [lhs, rhs] : std::views::zip(square[0] | std::views::drop(iDropCount), TopRight()))
			lhs = rhs;
		for (auto&& [lhs, rhs] : std::views::zip(square[1] | std::views::drop(iDropCount), BottomRight()))
			lhs = rhs;
		for (auto&& [lhs, rhs] : std::views::zip(square[2] | std::views::drop(iDropCount), BottomLeft()))
			lhs = rhs;
		for (auto&& [lhs, rhs] : std::views::zip(square[3] | std::views::drop(iDropCount), TopLeft()))
			lhs = rhs;
	}
};

export struct CTilesetImage final
{
	CGlTexture m_Texture{};	// Do not alloc new GL texture by default, keep it empty.
	int m_Width{};
	int m_Height{};
	int m_Layers{};	// how many layers in this texture array
	float m_LayerHeightV{};

	int m_Rows{};	// how many rows of tiles in this tileset
	int m_Columns{};	// how many columns of tiles in this tileset

	static constexpr auto TILE_WIDTH = 32;
	static constexpr auto TILE_HEIGHT = 32;

	// Default to flipping Y, as OpenGL's texture coordinate origin is bottom-left, while image files usually have top-left.
	// This must be aligned with other generated-textures. (Auto-tiles and Animated-tiles)
	CTilesetImage(const wchar_t* wcsFilename, bool bFlipY = false) noexcept
	{
		auto const res = LoadTextureArrayFromFile(wcsFilename, 256, bFlipY);

		auto const error = glGetError();
		if (error != GL_NO_ERROR) [[unlikely]]
		{
			std::filesystem::path const FilePath{ wcsFilename };
			std::println("[{}] OpenGL error after texture creation: {}", FilePath.filename().u8string(), error);
		}

		if (res.has_value())
		{
			GLuint textureId{};
			std::tie(textureId, m_Width, m_Height, m_Layers) = *res;

			m_LayerHeightV = 256.f / static_cast<float>(m_Height);
			m_Texture.Emplace(textureId);
		}

		if (m_Width % TILE_WIDTH != 0 || m_Height % TILE_HEIGHT != 0) [[unlikely]]
		{
			std::filesystem::path const FilePath{ wcsFilename };

			std::println(u8"Tileset image '{}' has invalid dimensions: {}x{} (must be multiple of {}x{})",
				FilePath.u8string(), m_Width, m_Height, TILE_WIDTH, TILE_HEIGHT
			);
		}

		m_Rows = m_Height / TILE_HEIGHT;
		m_Columns = m_Width / TILE_WIDTH;
	}

	CTilesetImage() noexcept = default;
	CTilesetImage(CTilesetImage const&) noexcept = default;
	CTilesetImage(CTilesetImage&&) noexcept = default;
	CTilesetImage& operator=(CTilesetImage const&) noexcept = default;
	CTilesetImage& operator=(CTilesetImage&&) noexcept = default;
	~CTilesetImage() noexcept = default;

	[[nodiscard]] constexpr auto UvOf(int x, int y) const noexcept -> CTileUV
	{
		if (x < 0 || x >= m_Columns || y < 0 || y >= m_Rows)
			return { 0.0f, 0.0f };

		auto const u = (float)(x * TILE_WIDTH) / (float)m_Width;
		auto const v = (float)(y * TILE_HEIGHT) / (float)m_Height;
		auto const flWidth = (float)TILE_WIDTH / (float)m_Width;
		auto const flHeight = (float)TILE_HEIGHT / (float)m_Height;

		return CTileUV{ .m_x{u}, .m_y{v}, .m_flWidth{flWidth}, .m_flHeight{flHeight} };
	}

	[[nodiscard]] constexpr auto GetOutputTextureId() const noexcept -> GLuint { return (GLuint)m_Texture; }

	[[nodiscard]] constexpr auto GetOutputTextureDimension() const noexcept -> std::pair<int, int> { return { m_Width, m_Height }; }

	[[nodiscard]] constexpr auto GetOutputTilesetDimension() const noexcept -> std::pair<int, int> { return { m_Columns, m_Rows }; }
};

// For indexing AUTOTILE_SHAPE_INFO below.
enum EAutotileShapes : std::uint8_t
{
	AT_Bordering_None,
	AT_Bordering_UL,
	AT_Bordering_UR,
	AT_Bordering_UL_UR,
	AT_Bordering_DR,
	AT_Bordering_UL_DR,
	AT_Bordering_UR_DR,
	AT_Bordering_UL_UR_DR,

	AT_Bordering_DL,
	AT_Bordering_UL_DL,
	AT_Bordering_UR_DL,
	AT_Bordering_UL_UR_DL,
	AT_Bordering_DL_DR,
	AT_Bordering_UL_DL_DR,
	AT_Bordering_UR_DL_DR,
	AT_Bordering_UL_UR_DL_DR,	// 'crossroad' shape, all 4 sides connected

	AT_Bordering_L,
	AT_Bordering_L_UR,
	AT_Bordering_L_DR,
	AT_Bordering_L_UR_DR,
	AT_Bordering_U,
	AT_Bordering_U_DR,
	AT_Bordering_U_DL,
	AT_Bordering_U_DR_DL,

	AT_Bordering_R,
	AT_Bordering_R_DL,
	AT_Bordering_R_UL,
	AT_Bordering_R_UL_DL,
	AT_Bordering_D,
	AT_Bordering_D_UL,
	AT_Bordering_D_UR,
	AT_Bordering_D_UL_DR,

	AT_Bordering_L_R,
	AT_Bordering_U_D,
	AT_Bordering_U_L,
	AT_Bordering_U_L_DR,
	AT_Bordering_U_R,
	AT_Bordering_U_R_DL,
	AT_Bordering_D_R,
	AT_Bordering_D_R_UL,

	AT_Bordering_D_L,
	AT_Bordering_D_L_UR,
	AT_Bordering_U_L_R,
	AT_Bordering_U_D_L,
	AT_Bordering_D_L_R,
	AT_Bordering_U_D_R,
	AT_Bordering_U_D_L_R,	// 'enclosed' shape, all 4 sides are adjacent to other tile types.
	AT_Bordering_UNUSED,

	AT_COUNT,
};

// arr[I] represents a full tile
// arr[I][J] represents a component of that tile
inline constexpr std::array AUTOTILE_SHAPE_INFO
{
	std::array{ std::array{2, 4}, std::array{3, 4}, std::array{2, 5}, std::array{3, 5}, },	// AT_Bordering_None,
	std::array{ std::array{4, 0}, std::array{3, 4}, std::array{2, 5}, std::array{3, 5}, },	// AT_Bordering_UL,
	std::array{ std::array{2, 4}, std::array{5, 0}, std::array{2, 5}, std::array{3, 5}, },	// AT_Bordering_UR,
	std::array{ std::array{4, 0}, std::array{5, 0}, std::array{2, 5}, std::array{3, 5}, },	// AT_Bordering_UL_UR,
	std::array{ std::array{2, 4}, std::array{3, 4}, std::array{2, 5}, std::array{5, 1}, },	// AT_Bordering_DR,
	std::array{ std::array{4, 0}, std::array{3, 4}, std::array{2, 5}, std::array{5, 1}, },	// AT_Bordering_UL_DR,
	std::array{ std::array{2, 4}, std::array{5, 0}, std::array{2, 5}, std::array{5, 1}, },	// AT_Bordering_UR_DR,
	std::array{ std::array{4, 0}, std::array{5, 0}, std::array{2, 5}, std::array{5, 1}, },	// AT_Bordering_UL_UR_DR,

	std::array{ std::array{2, 4}, std::array{3, 4}, std::array{4, 1}, std::array{3, 5}, },	// AT_Bordering_DL,
	std::array{ std::array{4, 0}, std::array{3, 4}, std::array{4, 1}, std::array{3, 5}, },	// AT_Bordering_UL_DL,
	std::array{ std::array{2, 4}, std::array{5, 0}, std::array{4, 1}, std::array{3, 5}, },	// AT_Bordering_UR_DL,
	std::array{ std::array{4, 0}, std::array{5, 0}, std::array{4, 1}, std::array{3, 5}, },	// AT_Bordering_UL_UR_DL,
	std::array{ std::array{2, 4}, std::array{3, 4}, std::array{4, 1}, std::array{5, 1}, },	// AT_Bordering_DL_DR,
	std::array{ std::array{4, 0}, std::array{3, 4}, std::array{4, 1}, std::array{5, 1}, },	// AT_Bordering_UL_DL_DR,
	std::array{ std::array{2, 4}, std::array{5, 0}, std::array{4, 1}, std::array{5, 1}, },	// AT_Bordering_UR_DL_DR,
	std::array{ std::array{4, 0}, std::array{5, 0}, std::array{4, 1}, std::array{5, 1}, },	// AT_Bordering_UL_UR_DL_DR,

	std::array{ std::array{0, 4}, std::array{1, 4}, std::array{0, 5}, std::array{1, 5}, },	// AT_Bordering_L,
	std::array{ std::array{0, 4}, std::array{5, 0}, std::array{0, 5}, std::array{1, 5}, },	// AT_Bordering_L_UR,
	std::array{ std::array{0, 4}, std::array{1, 4}, std::array{0, 5}, std::array{5, 1}, },	// AT_Bordering_L_DR,
	std::array{ std::array{0, 4}, std::array{5, 0}, std::array{0, 5}, std::array{5, 1}, },	// AT_Bordering_L_UR_DR,
	std::array{ std::array{2, 2}, std::array{3, 2}, std::array{2, 3}, std::array{3, 3}, },	// AT_Bordering_U,
	std::array{ std::array{2, 2}, std::array{3, 2}, std::array{2, 3}, std::array{5, 1}, },	// AT_Bordering_U_DR,
	std::array{ std::array{2, 2}, std::array{3, 2}, std::array{4, 1}, std::array{3, 3}, },	// AT_Bordering_U_DL,
	std::array{ std::array{2, 2}, std::array{3, 2}, std::array{4, 1}, std::array{5, 1}, },	// AT_Bordering_U_DR_DL,

	std::array{ std::array{4, 4}, std::array{5, 4}, std::array{4, 5}, std::array{5, 5}, },	// AT_Bordering_R,
	std::array{ std::array{4, 4}, std::array{5, 4}, std::array{4, 1}, std::array{5, 5}, },	// AT_Bordering_R_DL,
	std::array{ std::array{4, 0}, std::array{5, 4}, std::array{4, 5}, std::array{5, 5}, },	// AT_Bordering_R_UL,
	std::array{ std::array{4, 0}, std::array{5, 4}, std::array{4, 1}, std::array{5, 5}, },	// AT_Bordering_R_UL_DL,
	std::array{ std::array{2, 6}, std::array{3, 6}, std::array{2, 7}, std::array{3, 7}, },	// AT_Bordering_D,
	std::array{ std::array{4, 0}, std::array{3, 6}, std::array{2, 7}, std::array{3, 7}, },	// AT_Bordering_D_UL,
	std::array{ std::array{2, 6}, std::array{5, 0}, std::array{2, 7}, std::array{3, 7}, },	// AT_Bordering_D_UR,
	std::array{ std::array{4, 0}, std::array{5, 0}, std::array{2, 7}, std::array{3, 7}, },	// AT_Bordering_D_UL_DR,

	std::array{ std::array{0, 4}, std::array{5, 4}, std::array{0, 5}, std::array{5, 5}, },	// AT_Bordering_L_R,
	std::array{ std::array{2, 2}, std::array{3, 2}, std::array{2, 7}, std::array{3, 7}, },	// AT_Bordering_U_D,
	std::array{ std::array{0, 2}, std::array{1, 2}, std::array{0, 3}, std::array{1, 3}, },	// AT_Bordering_U_L,
	std::array{ std::array{0, 2}, std::array{1, 2}, std::array{0, 3}, std::array{5, 1}, },	// AT_Bordering_U_L_DR,
	std::array{ std::array{4, 2}, std::array{5, 2}, std::array{4, 3}, std::array{5, 3}, },	// AT_Bordering_U_R,
	std::array{ std::array{4, 2}, std::array{5, 2}, std::array{4, 1}, std::array{5, 3}, },	// AT_Bordering_U_R_DL,
	std::array{ std::array{4, 6}, std::array{5, 6}, std::array{4, 7}, std::array{5, 7}, },	// AT_Bordering_D_R,
	std::array{ std::array{4, 0}, std::array{5, 6}, std::array{4, 7}, std::array{5, 7}, },	// AT_Bordering_D_R_UL,

	std::array{ std::array{0, 6}, std::array{1, 6}, std::array{0, 7}, std::array{1, 7}, },	// AT_Bordering_D_L,
	std::array{ std::array{0, 6}, std::array{5, 0}, std::array{0, 7}, std::array{1, 7}, },	// AT_Bordering_D_L_UR,
	std::array{ std::array{0, 2}, std::array{5, 2}, std::array{0, 3}, std::array{5, 3}, },	// AT_Bordering_U_L_R,
	std::array{ std::array{0, 2}, std::array{1, 2}, std::array{0, 7}, std::array{1, 7}, },	// AT_Bordering_U_D_L,
	std::array{ std::array{0, 6}, std::array{5, 6}, std::array{0, 7}, std::array{5, 7}, },	// AT_Bordering_D_L_R,
	std::array{ std::array{4, 2}, std::array{5, 2}, std::array{4, 7}, std::array{5, 7}, },	// AT_Bordering_U_D_R,
	std::array{ std::array{0, 2}, std::array{5, 2}, std::array{0, 7}, std::array{5, 7}, },	// AT_Bordering_U_D_L_R,
	std::array{ std::array{0, 0}, std::array{1, 0}, std::array{0, 1}, std::array{1, 1}, },	// AT_Bordering_UNUSED,
};

export struct CAutotileImage final
{
	CGlTexture m_Texture{};	// Do not alloc new GL texture by default, keep it empty.

	static constexpr auto COMPONENT_WIDTH = CTilesetImage::TILE_WIDTH / 2;
	static constexpr auto COMPONENT_HEIGHT = CTilesetImage::TILE_HEIGHT / 2;

	static constexpr auto AUTOTILE_WIDTH = 96;
	static constexpr auto AUTOTILE_HEIGHT = 128;

	int m_Width{ AUTOTILE_WIDTH };
	int m_Height{ AUTOTILE_HEIGHT };

	// Output config
	static constexpr auto AUTOTILE_OUT_COLUMNS = 8;
	static constexpr auto AUTOTILE_OUT_ROWS = 6;

	static constexpr auto AUTOTILE_OUT_WIDTH = AUTOTILE_OUT_COLUMNS * CTilesetImage::TILE_WIDTH;
	static constexpr auto AUTOTILE_OUT_HEIGHT = AUTOTILE_OUT_ROWS * CTilesetImage::TILE_HEIGHT;

	static constexpr auto AUTOTILE_GL_TEXTURE_CHANNEL = 1;

	// Output canvas
	CGlCanvas m_Canvas{ AUTOTILE_OUT_WIDTH, AUTOTILE_OUT_HEIGHT };
	CGlPainter2D m_Painter{};	// data for the tile components

	static constexpr auto VERTEX_ELEM_COUNT = 5;		// Each vertex has 5 components: x, y, u, v, texid

	CAutotileImage(const wchar_t* wcsFilename, bool bFlipY = false) noexcept
	{
		auto const res = LoadTextureFromFile(wcsFilename, bFlipY);

		if (res.has_value())
		{
			GLuint textureId{};
			std::tie(textureId, m_Width, m_Height) = *res;

			m_Texture.Emplace(textureId);
		}

		if (m_Width % AUTOTILE_WIDTH != 0 || m_Height != AUTOTILE_HEIGHT) [[unlikely]]
		{
			std::filesystem::path const FilePath{ wcsFilename };

			std::println(u8"Tileset image '{}' has invalid dimensions: {}x{} (width must be multiple of {}, height must be exactly {})",
				FilePath.u8string(), m_Width, m_Height, AUTOTILE_WIDTH, AUTOTILE_HEIGHT
			);
		}

		// Doubling is because each tile was composed from 4 more small tiles, a.k.a. components.
		m_Painter.m_Vertices.reserve(static_cast<size_t>(AUTOTILE_OUT_COLUMNS * 2 * AUTOTILE_OUT_ROWS * 2) * 4u * VERTEX_ELEM_COUNT);
		m_Painter.m_Indices.reserve(static_cast<size_t>(AUTOTILE_OUT_COLUMNS * 2 * AUTOTILE_OUT_ROWS * 2) * 6u);

		auto const CompUF = (float)COMPONENT_WIDTH / (float)m_Width;
		auto const CompVF = (float)COMPONENT_HEIGHT / (float)m_Height;
		auto const CompWidthF = (float)COMPONENT_WIDTH / (float)AUTOTILE_OUT_WIDTH * 2.f;
		auto const CompHeightF = (float)COMPONENT_HEIGHT / (float)AUTOTILE_OUT_HEIGHT * 2.f;	// Because the scale is [-1, 1], so we need to double it.

		for (auto&& [iTileIndex, AutoTileComps] : std::views::enumerate(AUTOTILE_SHAPE_INFO))
		{
			for (auto&& [iCompIndex, CompUV_I] : std::views::enumerate(AutoTileComps))
			{
				// The coord of this tile in expanded texture of autotile is fixed, and CAN be generate through the algorithm below.

				// Each comp will output to a small square, which later built up to a full tile.
				auto const [flLeft, flTop] = GetOutputTilesetCoordf(iTileIndex, iCompIndex);
				auto const flRight = flLeft + CompWidthF;
				auto const flBottom = flTop - CompHeightF;	// negative Y is down in OpenGL

				auto const flLeftUV = (float)CompUV_I[0] * (float)COMPONENT_WIDTH / (float)m_Width;
				auto const flTopUV = (float)CompUV_I[1] * (float)COMPONENT_HEIGHT / (float)m_Height;
				auto const flRightUV = flLeftUV + CompUF;
				auto const flBottomUV = flTopUV + CompVF;	// For the texture, positive Y is down.

				std::array<float, VERTEX_ELEM_COUNT * 4u> const data{
					flLeft,  flTop,    flLeftUV,  flTopUV,    (float)AUTOTILE_GL_TEXTURE_CHANNEL,	// Top-Left
					flRight, flTop,    flRightUV, flTopUV,    (float)AUTOTILE_GL_TEXTURE_CHANNEL,	// Top-Right
					flLeft,  flBottom, flLeftUV,  flBottomUV, (float)AUTOTILE_GL_TEXTURE_CHANNEL,	// Bottom-Left
					flRight, flBottom, flRightUV, flBottomUV, (float)AUTOTILE_GL_TEXTURE_CHANNEL,	// Bottom-Right
				};

				auto const index_offset = (GLuint)m_Painter.m_Vertices.size() / VERTEX_ELEM_COUNT;

				std::array<GLuint, 6u> const indices{
					index_offset + 0, index_offset + 1, index_offset + 2, // First triangle
					index_offset + 1, index_offset + 2, index_offset + 3, // Second triangle
				};

				m_Painter.m_Vertices.append_range(data);
				m_Painter.m_Indices.append_range(indices);
			}
		}

		m_Painter.Commit(VERTEX_ELEM_COUNT);
	}

	CAutotileImage() noexcept = default;
	CAutotileImage(CAutotileImage const&) noexcept = delete;
	CAutotileImage(CAutotileImage&&) noexcept = default;
	CAutotileImage& operator=(CAutotileImage const&) noexcept = delete;
	CAutotileImage& operator=(CAutotileImage&&) noexcept = default;
	~CAutotileImage() noexcept = default;

	[[nodiscard]] static constexpr auto IsSizeAutotile(int iWidth, int iHeight) noexcept -> bool
	{
		return (iWidth % AUTOTILE_WIDTH == 0 && iHeight == AUTOTILE_HEIGHT);
	}

	static constexpr auto GetOutputTilesetCoordf(std::ptrdiff_t iTileIndex, std::ptrdiff_t iCompIndex) noexcept -> std::array<float, 2>
	{
		auto const iTileX = iTileIndex % AUTOTILE_OUT_COLUMNS;
		auto const iTileY = iTileIndex / AUTOTILE_OUT_COLUMNS;

		auto const iCompX = iCompIndex % 2;
		auto const iCompY = iCompIndex / 2;

		auto const flX = float(iTileX * CTilesetImage::TILE_WIDTH + iCompX * COMPONENT_WIDTH) / float(AUTOTILE_OUT_WIDTH);
		auto const flY = float(iTileY * CTilesetImage::TILE_HEIGHT + iCompY * COMPONENT_HEIGHT) / float(AUTOTILE_OUT_HEIGHT);

		// Mapping to range [-1, 1]
		return { flX * 2.f - 1.f, -(flY * 2.f - 1.f) };
	}

	void Render() const noexcept
	{
		if (!m_Texture) [[unlikely]]
		{
			std::println("No autotile image loaded.");
			return;
		}

		m_Canvas.Bind();

		glActiveTexture(GL_TEXTURE0 + AUTOTILE_GL_TEXTURE_CHANNEL);
		glBindTexture(GL_TEXTURE_2D, (GLuint)m_Texture);

		g_pGlobalShader->Use();

		static std::string szUniformName{
			std::format("ourTexture{}", AUTOTILE_GL_TEXTURE_CHANNEL)
		};
		glUniform1i(glGetUniformLocation(g_pGlobalShader->m_ProgramId, szUniformName.c_str()), AUTOTILE_GL_TEXTURE_CHANNEL);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		m_Painter.Paint();

		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		m_Canvas.Unbind();
	}

	[[nodiscard]] auto GetOutputTextureId() const noexcept -> GLuint { return m_Canvas.GetTextureId(); }

	[[nodiscard]] static constexpr auto GetOutputTextureDimension() noexcept -> std::pair<int, int> { return { AUTOTILE_OUT_WIDTH, AUTOTILE_OUT_HEIGHT }; }

	[[nodiscard]] static constexpr auto GetOutputTilesetDimension() noexcept -> std::pair<int, int> { return { AUTOTILE_OUT_COLUMNS, AUTOTILE_OUT_ROWS }; }
};

export struct CAnimatedTileImage final
{
	CGlTexture m_Texture{};	// Do not alloc new GL texture by default, keep it empty.

	static constexpr auto FRAME_WIDTH = CTilesetImage::TILE_WIDTH;
	static constexpr auto FRAME_HEIGHT = CTilesetImage::TILE_HEIGHT;

	static constexpr auto ANIMTILE_GL_TEXTURE_CHANNEL = 1;

	int m_FrameCount{ 1 };

	// Output canvas
	CGlCanvas m_Canvas{ FRAME_WIDTH, FRAME_HEIGHT };
	CGlPainter2D m_Painter{};

	static constexpr auto VERTEX_ELEM_COUNT = 5;		// Each vertex has 5 components: x, y, u, v, texid

	CAnimatedTileImage(const wchar_t* wcsFilename, bool bFlipY = false) noexcept
	{
		auto const res = LoadTextureFromFile(wcsFilename, bFlipY);
		int iWidth{}, iHeight{};

		if (res.has_value())
		{
			GLuint textureId{};
			std::tie(textureId, iWidth, iHeight) = *res;

			m_Texture.Emplace(textureId);
		}

		if (iWidth % FRAME_WIDTH != 0 || iHeight != FRAME_HEIGHT) [[unlikely]]
		{
			std::filesystem::path const FilePath{ wcsFilename };
			std::println(u8"Animated tile image '{}' has invalid dimensions: {}x{} (width must be multiple of {}, height must be exactly {})",
				FilePath.u8string(), iWidth, iHeight, FRAME_WIDTH, FRAME_HEIGHT
			);
		}

		m_FrameCount = iWidth / FRAME_WIDTH;

		// Since it's animated, all frames are rendered on the full canvas.

		static constexpr std::array fcoords{
			std::array{ -1.0f,  1.0f },	// Top-Left
			std::array{  1.0f,  1.0f },	// Top-Right
			std::array{ -1.0f, -1.0f },	// Bottom-Left
			std::array{  1.0f, -1.0f },	// Bottom-Right
		};

		// V coord is always the same, because the animated tile is required to be exactly one row.
		static constexpr float flTopV = 0.f;
		static constexpr float flBottomV = 1.f;

		for (int u = 0; u < iWidth; u += FRAME_WIDTH)
		{
			auto const flLeftU = (float)u / (float)iWidth;
			auto const flRightU = (float)(u + FRAME_WIDTH) / (float)iWidth;

			std::array<float, VERTEX_ELEM_COUNT * 4u> const data{
				fcoords[0][0], fcoords[0][1], flLeftU,  flTopV,    (float)ANIMTILE_GL_TEXTURE_CHANNEL,		// Top-Left
				fcoords[1][0], fcoords[1][1], flRightU, flTopV,    (float)ANIMTILE_GL_TEXTURE_CHANNEL,		// Top-Right
				fcoords[2][0], fcoords[2][1], flLeftU,  flBottomV, (float)ANIMTILE_GL_TEXTURE_CHANNEL,		// Bottom-Left
				fcoords[3][0], fcoords[3][1], flRightU, flBottomV, (float)ANIMTILE_GL_TEXTURE_CHANNEL,		// Bottom-Right
			};

			auto const index_offset = (GLuint)m_Painter.m_Vertices.size() / VERTEX_ELEM_COUNT;

			std::array<GLuint, 6u> const indices{
				index_offset + 0, index_offset + 1, index_offset + 2, // First triangle
				index_offset + 1, index_offset + 2, index_offset + 3, // Second triangle
			};

			m_Painter.m_Vertices.append_range(data);
			m_Painter.m_Indices.append_range(indices);
		}

		m_Painter.Commit(VERTEX_ELEM_COUNT);
	}

	CAnimatedTileImage() noexcept = default;
	CAnimatedTileImage(CAnimatedTileImage const&) noexcept = delete;
	CAnimatedTileImage(CAnimatedTileImage&& rhs) noexcept = default;
	CAnimatedTileImage& operator=(CAnimatedTileImage const&) noexcept = delete;
	CAnimatedTileImage& operator=(CAnimatedTileImage&& rhs) noexcept = default;
	~CAnimatedTileImage() noexcept = default;

	[[nodiscard]] static constexpr auto IsSizeAnimatedTile(int iWidth, int iHeight) noexcept -> bool
	{
		return (iWidth % FRAME_WIDTH == 0 && iHeight == FRAME_HEIGHT);
	}

	void Render(int iFrameIndex) const noexcept
	{
		if (!m_Texture || iFrameIndex < 0 || iFrameIndex >= m_FrameCount) [[unlikely]]
		{
			std::println("No animated tile image loaded ({}) or invalid frame index ({}).", (GLuint)m_Texture, iFrameIndex);
			return;
		}

		m_Canvas.Bind();

		glActiveTexture(GL_TEXTURE0 + ANIMTILE_GL_TEXTURE_CHANNEL);
		glBindTexture(GL_TEXTURE_2D, (GLuint)m_Texture);

		g_pGlobalShader->Use();

		static std::string szUniformName{
			std::format("ourTexture{}", ANIMTILE_GL_TEXTURE_CHANNEL)
		};
		glUniform1i(glGetUniformLocation(g_pGlobalShader->m_ProgramId, szUniformName.c_str()), ANIMTILE_GL_TEXTURE_CHANNEL);

		m_Painter.Paint(
			6,				// technicall "(GLsizei)m_Painter.m_Indices.size() / m_FrameCount", but we know it's always 6.
			iFrameIndex * 6	// Two triangles, 6 indices.
		);
		m_Canvas.Unbind();
	}

	[[nodiscard]] auto GetOutputTextureId() const noexcept -> GLuint { return m_Canvas.GetTextureId(); }

	[[nodiscard]] static constexpr auto GetOutputTextureDimension() noexcept -> std::pair<int, int> { return { FRAME_WIDTH, FRAME_HEIGHT }; }

	[[nodiscard]] static constexpr auto GetOutputTilesetDimension() noexcept -> std::pair<int, int> { return { FRAME_WIDTH / CTilesetImage::TILE_WIDTH, FRAME_HEIGHT / CTilesetImage::TILE_HEIGHT }; }
};
