module;

#include <gl/glew.h>

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#endif

export module GL.Painter;

#ifndef __INTELLISENSE__
import std.compat;
#endif

export struct CGlPainter2D final
{
	std::vector<float> m_Vertices{};
	std::vector<GLuint> m_Indices{};

	GLuint m_vbo{}, m_vao{}, m_ebo{};

	CGlPainter2D() noexcept
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glGenBuffers(1, &m_ebo);
	}
	CGlPainter2D(CGlPainter2D const&) noexcept = delete;
	CGlPainter2D(CGlPainter2D&& rhs) noexcept
		: m_Vertices(std::move(rhs.m_Vertices)), m_Indices(std::move(rhs.m_Indices)),
		m_vbo(rhs.m_vbo), m_vao(rhs.m_vao), m_ebo(rhs.m_ebo)
	{
		rhs.m_vbo = 0;
		rhs.m_vao = 0;
		rhs.m_ebo = 0;
	}
	CGlPainter2D& operator=(CGlPainter2D const&) noexcept = delete;
	CGlPainter2D& operator=(CGlPainter2D&& rhs) noexcept
	{
		if (this != &rhs)
		{
			if (m_vao != 0)
			{
				glDeleteVertexArrays(1, &m_vao);
				m_vao = 0;
			}

			if (m_vbo != 0)
			{
				glDeleteBuffers(1, &m_vbo);
				m_vbo = 0;
			}

			if (m_ebo != 0)
			{
				glDeleteBuffers(1, &m_ebo);
				m_ebo = 0;
			}

			m_Vertices = std::move(rhs.m_Vertices);
			m_Indices = std::move(rhs.m_Indices);

			m_vbo = rhs.m_vbo;
			m_vao = rhs.m_vao;
			m_ebo = rhs.m_ebo;

			rhs.m_vbo = 0;
			rhs.m_vao = 0;
			rhs.m_ebo = 0;
		}

		return *this;
	}
	~CGlPainter2D() noexcept
	{
		if (m_vao != 0)
		{
			glDeleteVertexArrays(1, &m_vao);
			m_vao = 0;
		}

		if (m_vbo != 0)
		{
			glDeleteBuffers(1, &m_vbo);
			m_vbo = 0;
		}

		if (m_ebo != 0)
		{
			glDeleteBuffers(1, &m_ebo);
			m_ebo = 0;
		}
	}

	void Commit(int iStride) const noexcept
	{
		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(decltype(m_Vertices)::value_type), m_Vertices.data(), GL_STATIC_DRAW);

		// 3. 复制我们的索引数组到一个索引缓冲中，供OpenGL使用
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(decltype(m_Indices)::value_type), m_Indices.data(), GL_STATIC_DRAW);

		// 4. 设置顶点属性指针
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			iStride * sizeof(decltype(m_Vertices)::value_type),   // stride
			(void*)(0 * sizeof(decltype(m_Vertices)::value_type)) // array buffer offset
		);
		glEnableVertexAttribArray(0);

		// texture coord attribute
		glVertexAttribPointer(
			1,                  // attribute 1
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			iStride * sizeof(decltype(m_Vertices)::value_type),   // stride
			(void*)(2 * sizeof(decltype(m_Vertices)::value_type)) // array buffer offset
		);
		glEnableVertexAttribArray(1);

		// texture id attribute
		glVertexAttribPointer(
			2,                  // attribute 2
			1,                  // size
			GL_FLOAT,    // type
			GL_FALSE,           // normalized?
			iStride * sizeof(decltype(m_Vertices)::value_type),   // stride
			(void*)(4 * sizeof(decltype(m_Vertices)::value_type)) // array buffer offset
		);
		glEnableVertexAttribArray(2);
	}

	void Paint(GLsizei iCount = -1, std::intptr_t iOffset = 0) const noexcept
	{
		if (iCount < 0)
			iCount = (GLsizei)m_Indices.size();

		// Offset is in bytes.
		iOffset *= sizeof(decltype(m_Indices)::value_type);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_INT, (void*)iOffset);
		glBindVertexArray(0);

		glDisable(GL_BLEND);
	}
};
