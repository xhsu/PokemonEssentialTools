module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

#define HYDROGENIUM_GL_TEXTURE_WRAPPER_MACRO 2025'08'27L

export module GL.Texture;

#ifndef __INTELLISENSE__
import std.compat;
#endif

export inline constexpr auto HYDROGENIUM_GL_TEXTURE_WRAPPER = HYDROGENIUM_GL_TEXTURE_WRAPPER_MACRO;

export struct CGlTexture final
{
public:
	GLuint m_TextureId{};

private:
	std::shared_ptr<std::uint_fast32_t> m_RefCount{ nullptr };

public:
	CGlTexture(std::in_place_t) noexcept
		: m_RefCount{ std::make_shared<decltype(m_RefCount)::element_type>(1)}
	{
		glGenTextures(1, &m_TextureId);
	}

	// Take over ownship of an existing texture.
	explicit CGlTexture(GLuint textureId) noexcept
		: m_TextureId{ textureId }, m_RefCount{ std::make_shared<decltype(m_RefCount)::element_type>(1) }
	{
	}

	CGlTexture() noexcept = default;

	CGlTexture(const CGlTexture& rhs) noexcept
		: m_TextureId{ rhs.m_TextureId }, m_RefCount{ rhs.m_RefCount }
	{
		++(*m_RefCount);
	}

	CGlTexture& operator=(const CGlTexture& rhs) noexcept
	{
		if (this != &rhs)
		{
			this->Release();
			this->m_TextureId = rhs.m_TextureId;
			this->m_RefCount = rhs.m_RefCount;
			++(*this->m_RefCount);
		}

		return *this;
	}

	CGlTexture(CGlTexture&& rhs) noexcept
		: m_TextureId{ rhs.m_TextureId }, m_RefCount{ std::move(rhs.m_RefCount) }
	{
		rhs.m_TextureId = 0;
	}

	CGlTexture& operator=(CGlTexture&& rhs) noexcept
	{
		if (this != &rhs)
		{
			this->Release();
			this->m_TextureId = rhs.m_TextureId;
			this->m_RefCount = std::move(rhs.m_RefCount);
			rhs.m_TextureId = 0;
		}

		return *this;
	}

	~CGlTexture() noexcept
	{
		Release();
	}

public:
	[[nodiscard]] inline constexpr explicit operator GLuint() const noexcept { return m_TextureId; }
	[[nodiscard]] inline constexpr explicit operator bool() const noexcept { return m_TextureId != 0; }
	[[nodiscard]] inline constexpr auto operator<=>(const CGlTexture& rhs) const noexcept { return this->m_TextureId <=> rhs.m_TextureId; };
	[[nodiscard]] inline constexpr auto operator==(const CGlTexture& rhs) const noexcept { return this->m_TextureId == rhs.m_TextureId; };
	[[nodiscard]] inline           auto RefCount() const noexcept { return *m_RefCount; }
	[[nodiscard]] inline constexpr auto TextureId() const noexcept { return m_TextureId; }

public:
	void Release() noexcept
	{
		if (m_TextureId != 0 && --(*m_RefCount) == 0)
		{
			glDeleteTextures(1, &m_TextureId);
			m_RefCount.reset();	// release the shared_ptr
		}
	}

	void Emplace() noexcept
	{
		this->Release();
		glGenTextures(1, &m_TextureId);
		m_RefCount = std::make_shared<decltype(m_RefCount)::element_type>(1);
	}

	void Emplace(GLuint textureId) noexcept
	{
		this->Release();
		this->m_TextureId = textureId;
		m_RefCount = std::make_shared<decltype(m_RefCount)::element_type>(1);
	}
};
