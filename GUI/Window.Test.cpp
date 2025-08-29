#include <imgui.h>
#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import GL.Painter;
import GL.Canvas;
import GL.Shader;

extern auto LoadTextureFromFile(const wchar_t* file_name, bool bFlipY) noexcept -> std::expected<std::tuple<std::uint32_t, int, int>, std::string_view>;
extern auto LoadTextureArrayFromFile(const wchar_t* file_name, int target_height, bool bFlipY) noexcept -> std::expected<std::tuple<std::uint32_t, int, int, int>, std::string_view>;

#pragma region Test Shader with sampler2DArray

inline constexpr char VERTEX_SHADER_SAMP2DARR[] = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aTextureLayer;

out vec2 TexCoord;
out float TextureLayer;

void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	TexCoord = aTexCoord;
	TextureLayer = aTextureLayer;
}
)";

inline constexpr char FRAGMENT_SHADER_SAMP2DARR[] = R"(
#version 330 core

uniform sampler2DArray ourTextureArray;

in vec2 TexCoord;
in float TextureLayer;

out vec4 FragColor;

void main()
{
	FragColor = texture(ourTextureArray, vec3(TexCoord, TextureLayer));
}
)";

#pragma endregion Test Shader with sampler2DArray

#pragma region Test Shader with sampler2DArray and Global UV Mapping

inline constexpr char VERTEX_SHADER_GLOBAL_UV[] = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;  // This is now the global UV (0.0 to 1.0 for entire original texture)

out vec2 GlobalTexCoord;

void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	GlobalTexCoord = aTexCoord;
}
)";

inline constexpr char FRAGMENT_SHADER_GLOBAL_UV[] = R"(
#version 330 core

uniform sampler2DArray ourTextureArray;
uniform float layerHeight;  // Height of each layer in UV space (e.g., 256/29216 = 0.00876...)

in vec2 GlobalTexCoord;

out vec4 FragColor;

void main()
{
	// Get the texture array dimensions directly
	ivec3 textureArraySize = textureSize(ourTextureArray, 0);
	int totalLayers = textureArraySize.z;  // z component contains the number of layers

	// Convert global V coordinate to layer index and local V coordinate
	// Calculate which layer this UV falls into
	float layerFloat = GlobalTexCoord.y / layerHeight;
	int layerIndex = int(floor(layerFloat));

	// Clamp layer index to valid range
	layerIndex = clamp(layerIndex, 0, totalLayers - 1);

	// Calculate local V coordinate within the layer (0.0 to 1.0)
	float localV = fract(layerFloat);

	// U coordinate remains the same
	float localU = GlobalTexCoord.x;

	// Sample from the texture array
	vec3 texCoord = vec3(localU, localV, float(layerIndex));
	FragColor = texture(ourTextureArray, texCoord);
}
)";

#pragma endregion

namespace TestWindow
{
	inline std::optional<CGlPainter2D> m_TestPainter = std::nullopt;
	inline std::optional<CGlCanvas> m_TestCanvas = std::nullopt;
	inline std::optional<CGlShader> m_TestShader = std::nullopt;
	inline GLuint m_TextureArrayId = 0;
	inline int m_LayerCount = 0;
	inline float m_LayerHeightV = 0;	// Height of each layer in V coordinate (0.0 to 1.0)

	void Initialize() noexcept
	{
		// Load texture array
		auto result = LoadTextureArrayFromFile(LR"(G:\GameTools\Image\Ecchi Version Showcase 12-23-2023\Graphics\Tilesets\pkmnh_overworld_2x.png)", 256, false);
		if (!result.has_value()) {
			std::println("Failed to load texture array");
			return;
		}

		auto&& [iTexArrId, iWidth, iHeight, iLayerCount] = *result;
		m_TextureArrayId = iTexArrId;
		m_LayerCount = iLayerCount;

		m_LayerHeightV = 256.f / static_cast<float>(iHeight);

		auto const iTotalCols = iWidth / 32;
		auto const iTotalRows = iHeight / 32;

		// Initialize components
		m_TestPainter.emplace();
		m_TestCanvas.emplace(512, 512);
		m_TestShader.emplace(VERTEX_SHADER_GLOBAL_UV, FRAGMENT_SHADER_GLOBAL_UV);

		// Create vertices for multiple quads, each using different texture layers
		// Format: x, y, u, v, layer
		std::vector<float> vertices;
		std::vector<GLuint> indices;

		// Create a grid of quads, each showing a different layer
		const int grid_cols = 3;
		const int grid_rows = 3;
		
		// Tile size and gap configuration
		constexpr int tile_size = 64;      // Size of each tile in pixels
		constexpr int gap_size = 16;       // Gap between tiles in pixels
		constexpr int total_tile_step = tile_size + gap_size;  // Total step size including gap

		int iTileIndex = 84;

		for (int row = 0; row < grid_rows; ++row) {
			for (int col = 0; col < grid_cols; ++col) {
				int layer_index = row * grid_cols + col;

				auto const iTileX = iTileIndex % iTotalCols;
				auto const iTileY = iTileIndex / iTotalCols;

				++iTileIndex;

				// Calculate UV coordinates for the tile in the texture
				auto const flLeftU = (float)(iTileX * 32) / (float)iWidth;
				auto const flRightU = (float)((iTileX + 1) * 32) / (float)iWidth;
				auto const flTopV = (float)(iTileY * 32) / (float)iHeight;
				auto const flBottomV = (float)((iTileY + 1) * 32) / (float)iHeight;

				// Calculate screen position with gaps
				// Start from an offset to center the grid
				constexpr float start_offset_x = gap_size / 2.0f;
				constexpr float start_offset_y = gap_size / 2.0f;
				
				auto const flLeft = (start_offset_x + (float)(col * total_tile_step)) / m_TestCanvas->m_Width * 2.f - 1.f;
				auto const flTop = -((start_offset_y + (float)(row * total_tile_step)) / m_TestCanvas->m_Height * 2.f - 1.f);	// Watch out this negation!
				auto const flRight = (start_offset_x + (float)(col * total_tile_step + tile_size)) / m_TestCanvas->m_Width * 2.f - 1.f;
				auto const flBottom = -((start_offset_y + (float)(row * total_tile_step + tile_size)) / m_TestCanvas->m_Height * 2.f - 1.f);

				GLuint base_index = static_cast<GLuint>(vertices.size() / 5);

				// Add vertices (x, y, u, v, layer)
				std::array<float, 20> quad_vertices{
					flRight, flTop,    flRightU, flTopV, static_cast<float>(layer_index), // Top-right
					flRight, flBottom, flRightU, flBottomV, static_cast<float>(layer_index), // Bottom-right
					flLeft,  flBottom, flLeftU, flBottomV, static_cast<float>(layer_index), // Bottom-left
					flLeft,  flTop,    flLeftU, flTopV, static_cast<float>(layer_index), // Top-left
				};

				// Add indices for two triangles
				std::array<GLuint, 6> quad_indices{
					base_index + 0, base_index + 1, base_index + 3, // First triangle
					base_index + 1, base_index + 2, base_index + 3  // Second triangle
				};

				vertices.insert(vertices.end(), quad_vertices.begin(), quad_vertices.end());
				indices.insert(indices.end(), quad_indices.begin(), quad_indices.end());
			}
		}

		m_TestPainter->m_Vertices.assign(vertices.begin(), vertices.end());
		m_TestPainter->m_Indices.assign(indices.begin(), indices.end());
		m_TestPainter->Commit(5); // 5 components per vertex: x, y, u, v, layer
	}

	void Render() noexcept
	{
		if (!m_TestCanvas || !m_TestShader || !m_TestPainter || m_TextureArrayId == 0) {
			return;
		}

		// Bind canvas and shader
		m_TestCanvas->Bind();
		m_TestShader->Use();

		// Bind texture array to texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_TextureArrayId);

		// Set uniforms
		glUniform1i(glGetUniformLocation(m_TestShader->m_ProgramId, "ourTextureArray"), 0);

		// Calculate layer height in UV space
		glUniform1f(glGetUniformLocation(m_TestShader->m_ProgramId, "layerHeight"), m_LayerHeightV);

		// Clear canvas
		glClear(GL_COLOR_BUFFER_BIT);

		// Render all quads
		m_TestPainter->Paint();

		m_TestCanvas->Unbind();
	}

	void ShowWindow() noexcept
	{
		if (ImGui::Begin("Texture Array Test"))
		{
			if (ImGui::Button("Initialize"))
				Initialize();

			if (m_TestCanvas) {
				ImGui::Text("Layers loaded: %d", m_LayerCount);
				ImGui::Text("Canvas texture ID: %u", m_TestCanvas->GetTextureId());
	
				// Render the test
				Render();
				
				// Display the canvas texture in ImGui
				ImVec2 canvas_size((float)m_TestCanvas->m_Width, (float)m_TestCanvas->m_Height);
				ImGui::Image((ImTextureID)m_TestCanvas->GetTextureId(),
							canvas_size, ImVec2(0, 1), ImVec2(1, 0)); // Flip Y for OpenGL
			}
		}
		ImGui::End();
	}

	void Cleanup() noexcept
	{
		if (m_TextureArrayId != 0) {
			glDeleteTextures(1, &m_TextureArrayId);
			m_TextureArrayId = 0;
		}
		m_TestPainter.reset();
		m_TestCanvas.reset();
		m_TestShader.reset();
	}
}