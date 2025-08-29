module;

#include <gl/glew.h>

#include "stb_image_write.h"

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module GL.Canvas;

#ifndef __INTELLISENSE__
import std.compat;
#endif

import GL.Texture;

export struct CGlCanvas final
{
	GLuint m_fbo{};
	CGlTexture m_Texture{ std::in_place };
	int m_Width{};
	int m_Height{};

	mutable std::array<GLint, 4> m_LastViewport{};

	CGlCanvas(int iWidth, int iHeight) noexcept : m_Texture{ std::in_place }, m_Width(iWidth), m_Height(iHeight)
	{
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

		// Texture already generated in m_Texture's constructor
		glBindTexture(GL_TEXTURE_2D, (GLuint)m_Texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)m_Texture, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::println("Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the framebuffer
	}

	CGlCanvas(CGlCanvas const&) noexcept = delete;
	CGlCanvas(CGlCanvas&& rhs) noexcept
		: m_fbo{ rhs.m_fbo }, m_Texture{ std::move(rhs.m_Texture) }, m_Width{ rhs.m_Width }, m_Height{ rhs.m_Height }
	{
		rhs.m_fbo = 0;
	}
	CGlCanvas& operator=(CGlCanvas const&) noexcept = delete;
	CGlCanvas& operator=(CGlCanvas&& rhs) noexcept
	{
		if (this != &rhs)
		{
			if (m_fbo != 0)
			{
				glDeleteFramebuffers(1, &m_fbo);
				m_fbo = 0;
			}

			m_fbo = rhs.m_fbo;
			m_Texture = std::move(rhs.m_Texture);
			m_Width = rhs.m_Width;
			m_Height = rhs.m_Height;

			rhs.m_fbo = 0;
		}
		return *this;
	}

	~CGlCanvas() noexcept
	{
		if (m_fbo != 0)
		{
			glDeleteFramebuffers(1, &m_fbo);
			m_fbo = 0;
		}
	}

	void Bind(bool bClearTexture = true) const noexcept
	{
		glGetIntegerv(GL_VIEWPORT, m_LastViewport.data());

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glViewport(0, 0, m_Width, m_Height); // Set viewport to texture dimensions

		if (bClearTexture)
		{
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set the 'clear' color. Transparent by default.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear texture with desinated clear color.
		}
	}

	void Unbind() const noexcept
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer
		glViewport(m_LastViewport[0], m_LastViewport[1], m_LastViewport[2], m_LastViewport[3]); // Reset viewport to window dimensions
	}

	bool SaveToFile(const std::filesystem::path& filePath) const noexcept
	{
		// Bind the framebuffer to read from it
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		
		// Allocate memory for pixel data (RGBA format)
		std::vector<unsigned char> pixels(m_Width * m_Height * 4);
		
		// Read pixels from framebuffer
		glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		
		// Check for OpenGL errors
		auto const error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::println("OpenGL error when reading pixels: 0x{:X}", error);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}
		
		// OpenGL reads pixels from bottom-left, but images are typically top-left
		// So we need to flip the image vertically
		std::vector<unsigned char> flippedPixels(pixels.size());
		for (int y = 0; y < m_Height; ++y)
		{
			std::memcpy(
				flippedPixels.data() + y * m_Width * 4,
				pixels.data() + (m_Height - 1 - y) * m_Width * 4,
				m_Width * 4
			);
		}
		
		// Save to PNG file
		auto const result = stbi_write_png(
			filePath.u8string().c_str(),
			m_Width, 
			m_Height, 
			4, // 4 components (RGBA)
			flippedPixels.data(), 
			m_Width * 4 // stride
		);
		
		// Restore framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		if (result == 0)
		{
			std::println("Failed to write PNG file: {}", filePath.u8string());
			return false;
		}
		
		std::println("Successfully saved PNG: {}", filePath.u8string());
		return true;
	}

	[[nodiscard]] inline auto GetTextureId() const noexcept -> GLuint { return (GLuint)m_Texture; }
};
