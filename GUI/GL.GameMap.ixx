module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module GL.GameMap;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import GL.Canvas;
import GL.Painter;
import GL.Shader;
import GL.Texture;
import Image.Tilesets;
import Game.Tilesets;


export struct CGlGameMap final
{
	CGlCanvas m_Canvas;
	CTileset const* m_pTileset;	// tileset image for this map. Only one tileset is used for each map.

	int m_Rows{};	// how many rows of tiles in this map
	int m_Columns{};	// how many columns of tiles in this map

	CGlPainter2D m_Painter;	// data for the tiles, painting the map.

	static constexpr auto CELL_WIDTH = CTilesetImage::TILE_WIDTH;
	static constexpr auto CELL_HEIGHT = CTilesetImage::TILE_HEIGHT;

	static constexpr auto VERTEX_ELEM_COUNT = 5;	// Each vertex has 5 components: x, y, u, v, texid

	static constexpr auto TOTAL_LAYERS = 3;	// RPG Maker XP has 3 layers of tiles. Events is excluded for now.

	CGlGameMap(int iWidth, int iHeight, CTileset const* pTileset) noexcept
		: m_Canvas(iWidth, iHeight), m_Rows(iHeight / CELL_HEIGHT), m_Columns(iWidth / CELL_WIDTH),
		m_pTileset(pTileset)
	{
		if (m_Rows * CELL_HEIGHT != iHeight || m_Columns * CELL_WIDTH != iWidth) [[unlikely]]
		{
			std::println("Map size {}x{} is not a multiple of tile size {}x{}",
				iWidth, iHeight, CELL_WIDTH, CELL_HEIGHT
			);
		}

		// Vertices data struct:
		// x, y, u, v

		// Each cell in the map is represented by 4 vertices (2 triangles).
		// Each vertex has 4 components: x, y, u, v
		// Hence cell count * 4 vertices * 4 components per vertex

		m_Painter.m_Vertices.reserve(static_cast<size_t>(m_Rows * m_Columns) * 4u * VERTEX_ELEM_COUNT * TOTAL_LAYERS);
		m_Painter.m_Indices.reserve(static_cast<size_t>(m_Rows * m_Columns) * 6u * TOTAL_LAYERS);
	}

	CGlGameMap(CGlGameMap const&) noexcept = delete;
	CGlGameMap(CGlGameMap&&) noexcept = default;
	CGlGameMap& operator=(CGlGameMap const&) noexcept = delete;
	CGlGameMap& operator=(CGlGameMap&&) noexcept = default;
	~CGlGameMap() noexcept = default;

	void Render() const noexcept
	{
		if (m_pTileset == nullptr) [[unlikely]]
		{
			std::println("No tileset image loaded for this map.");
			return;
		}

		m_Canvas.Bind();

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		g_pGlobalShader->Use();

		for (int i = 1; i < (int)m_pTileset->m_rgpTilesetTextures.size(); ++i)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, m_pTileset->GetTextureIdAt(i));
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_pTileset->GetTextureIdAt(0));

		for (int i = 0; i < (int)m_pTileset->m_rgpTilesetTextures.size(); ++i)
		{
			auto const sz = std::format("ourTexture{}", i);
			glUniform1i(glGetUniformLocation(g_pGlobalShader->m_ProgramId, sz.c_str()), i);
		}

		if (auto const pMainTilesetObject = m_pTileset->GetMainTilesetObject(); pMainTilesetObject != nullptr) [[likely]]
			glUniform1f(glGetUniformLocation(g_pGlobalShader->m_ProgramId, "layerHeight"), pMainTilesetObject->m_LayerHeightV);

		m_Painter.Paint();

		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		m_Canvas.Unbind();
	}

	// RPG Maker XP tile index comes with a offset of 384. It's a null autotile (48) plus 7 regular autotiles. Therefore 48 * 8 = 384.

	void AddTile(int x, int y, int iTileIndex) noexcept
	{
		if (x >= m_Columns || y >= m_Rows || iTileIndex < 0) [[unlikely]]
		{
			std::println("Invalid tile assignment: x={}, y={}, iTileIndex={}", x, y, iTileIndex);
			return;
		}

		auto const iTilesetPageIndex = m_pTileset->GetTilePageIndex(iTileIndex);

		// Now that we determind which tileset image to use, we need to normalize the tile index to fit in that image.
		iTileIndex = m_pTileset->GetNormalizedTileIndex(iTileIndex);

		auto const [iTilesetWidth, iTilesetHeight] = m_pTileset->GetTextureDimensionsAt(iTilesetPageIndex);
		auto const [iTilesetCols, iTilesetRows] = m_pTileset->GetTilesetDimensionsAt(iTilesetPageIndex);

		auto const iTileX = iTileIndex % iTilesetCols;
		auto const iTileY = iTileIndex / iTilesetCols;

		auto const flLeft = (float)(x * CELL_WIDTH) / m_Canvas.m_Width * 2.f - 1.f;
		auto const flTop = -((float)(y * CELL_HEIGHT) / m_Canvas.m_Height * 2.f - 1.f);	// Watch out this negation!
		auto const flRight = (float)((x + 1) * CELL_WIDTH) / m_Canvas.m_Width * 2.f - 1.f;
		auto const flBottom = -((float)((y + 1) * CELL_HEIGHT) / m_Canvas.m_Height * 2.f - 1.f);

		auto const flLeftUV = (float)(iTileX * CTilesetImage::TILE_WIDTH) / (float)iTilesetWidth;
		auto const flTopUV = -(float)(iTileY * CTilesetImage::TILE_HEIGHT) / (float)iTilesetHeight;
		auto const flRightUV = (float)((iTileX + 1) * CTilesetImage::TILE_WIDTH) / (float)iTilesetWidth;
		auto const flBottomUV = -(float)((iTileY + 1) * CTilesetImage::TILE_HEIGHT) / (float)iTilesetHeight;

		std::array const rgflVertexData{
			flLeft,  flTop,    flLeftUV,  flTopUV,    (float)iTilesetPageIndex,	// Top-Left
			flRight, flTop,    flRightUV, flTopUV,    (float)iTilesetPageIndex,	// Top-Right
			flLeft,  flBottom, flLeftUV,  flBottomUV, (float)iTilesetPageIndex,	// Bottom-Left
			flRight, flBottom, flRightUV, flBottomUV, (float)iTilesetPageIndex,	// Bottom-Right
		};

		auto const index_offset = (GLuint)m_Painter.m_Vertices.size() / VERTEX_ELEM_COUNT;

		std::array<GLuint, 6u> const indices{
			index_offset + 0, index_offset + 1, index_offset + 2, // First triangle
			index_offset + 1, index_offset + 2, index_offset + 3, // Second triangle
		};

		m_Painter.m_Vertices.append_range(rgflVertexData);
		m_Painter.m_Indices.append_range(indices);
	}

	inline void CommitVerticesChanges() const noexcept
	{
		//glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		//glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(decltype(m_Vertices)::value_type), m_Vertices.data(), GL_STATIC_DRAW);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_Painter.Commit(VERTEX_ELEM_COUNT);
	}
};
