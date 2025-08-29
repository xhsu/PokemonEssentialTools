module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module GL.Shader;

#ifndef __INTELLISENSE__
import std.compat;
#endif

export struct CGlShader final
{
	GLuint m_ProgramId{};

	CGlShader() noexcept = default;
	CGlShader(CGlShader const&) noexcept = delete;
	CGlShader(CGlShader&& rhs) noexcept : m_ProgramId(rhs.m_ProgramId)
	{
		rhs.m_ProgramId = 0;
	}
	CGlShader& operator=(CGlShader const&) noexcept = delete;
	CGlShader& operator=(CGlShader&& rhs) noexcept
	{
		if (this != &rhs)
		{
			if (m_ProgramId != 0)
			{
				glDeleteProgram(m_ProgramId);
				m_ProgramId = 0;
			}
			m_ProgramId = rhs.m_ProgramId;
			rhs.m_ProgramId = 0;
		}
		return *this;
	}
	~CGlShader() noexcept
	{
		if (m_ProgramId != 0)
			glDeleteProgram(m_ProgramId);
	}

	CGlShader(const char* szVertexShaderSource, const char* szFragmentShaderSource) noexcept
	{
		// Create the shaders
		GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		GLint Result = GL_FALSE;
		int InfoLogLength;

		// Compile Vertex Shader
		glShaderSource(VertexShaderID, 1, &szVertexShaderSource, nullptr);
		glCompileShader(VertexShaderID);

		// Check Vertex Shader
		glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0)
		{
			std::string szErrorMessage(InfoLogLength + 1, '\0');
			glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, szErrorMessage.data());
			std::print("Vertex Shader Error:\n>>> {}\n", szErrorMessage);
		}

		// Compile Fragment Shader
		glShaderSource(FragmentShaderID, 1, &szFragmentShaderSource, nullptr);
		glCompileShader(FragmentShaderID);

		// Check Fragment Shader
		glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0)
		{
			std::string szErrorMessage(InfoLogLength + 1, '\0');
			glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, szErrorMessage.data());
			std::print("Fragment Shader Error:\n>>> {}\n", szErrorMessage);
		}

		// Link the program
		m_ProgramId = glCreateProgram();
		glAttachShader(m_ProgramId, VertexShaderID);
		glAttachShader(m_ProgramId, FragmentShaderID);
		glLinkProgram(m_ProgramId);

		// Check the program
		glGetProgramiv(m_ProgramId, GL_LINK_STATUS, &Result);
		glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0)
		{
			std::string szErrorMessage(InfoLogLength + 1, '\0');
			glGetProgramInfoLog(m_ProgramId, InfoLogLength, nullptr, szErrorMessage.data());
			std::print("Shader Program Error:\n>>> {}\n", szErrorMessage);
		}

		glDetachShader(m_ProgramId, VertexShaderID);
		glDetachShader(m_ProgramId, FragmentShaderID);

		glDeleteShader(VertexShaderID);
		glDeleteShader(FragmentShaderID);
	}

	inline void Use() const noexcept
	{
		if (m_ProgramId != 0) [[likely]]
			glUseProgram(m_ProgramId);
		else
			std::println("Shader program is not initialized.");
	}
};

export inline std::optional<CGlShader> g_pGlobalShader = std::nullopt;

#pragma region Default Shader

inline constexpr char VERTEX_SHADER[] = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aTextureId;

out vec2 TexCoord;
flat out int TextureId;

void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	TexCoord = aTexCoord;
	TextureId = int(aTextureId);
}
)";

inline constexpr char FRAGMENT_SHADER[] = R"(
#version 330 core

uniform sampler2DArray ourTexture0;
uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
uniform sampler2D ourTexture3;
uniform sampler2D ourTexture4;
uniform sampler2D ourTexture5;
uniform sampler2D ourTexture6;
uniform sampler2D ourTexture7;

uniform float layerHeight;  // Height of each layer in UV space (e.g., 256/29216 = 0.00876...)

in vec2 TexCoord;
flat in int TextureId;

out vec4 FragColor;

void main()
{
	switch (TextureId)
	{
		case 0:
		default:
		{
			// Get the texture array dimensions directly
			ivec3 textureArraySize = textureSize(ourTexture0, 0);
			int totalLayers = textureArraySize.z;  // z component contains the number of layers

			// Convert global V coordinate to layer index and local V coordinate
			// Calculate which layer this UV falls into
			float layerFloat = -TexCoord.y / layerHeight;	// Negate Y because TexCoord.y is from top to bottom, and consistent with other GL-generated textures
			int layerIndex = int(floor(layerFloat));

			// Clamp layer index to valid range
			layerIndex = clamp(layerIndex, 0, totalLayers - 1);

			// Calculate local V coordinate within the layer (0.0 to 1.0)
			float localV = fract(layerFloat);

			// U coordinate remains the same
			float localU = TexCoord.x;

			// Sample from the texture array
			vec3 LocalTexCoord = vec3(localU, localV, float(layerIndex));
			FragColor = texture(ourTexture0, LocalTexCoord);
			break;
		}
		case 1:
			FragColor = texture(ourTexture1, TexCoord);
			break;
		case 2:
			FragColor = texture(ourTexture2, TexCoord);
			break;
		case 3:
			FragColor = texture(ourTexture3, TexCoord);
			break;
		case 4:
			FragColor = texture(ourTexture4, TexCoord);
			break;
		case 5:
			FragColor = texture(ourTexture5, TexCoord);
			break;
		case 6:
			FragColor = texture(ourTexture6, TexCoord);
			break;
		case 7:
			FragColor = texture(ourTexture7, TexCoord);
			break;
	}
}
)";

#pragma endregion Default Shader

export inline void CompileBuiltinShader() noexcept
{
	g_pGlobalShader.emplace(VERTEX_SHADER, FRAGMENT_SHADER);
}
