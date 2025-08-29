// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <gl/glew.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#ifdef __INTELLISENSE__
#include <__msvc_all_public_headers.hpp>
#undef min
#undef max
#else
import std.compat;
#endif

import GL.Shader;
import Game.Tilesets;
import Game.Path;
import Database.Raw.PBS;
import Database.RX;



#pragma region INI CONFIG

inline constexpr char INI_CONFIG[] = R"(
[Window][WindowOverViewport_11111111]
Pos=0,19
Size=1280,781
Collapsed=0

[Window][Map List]
Pos=0,19
Size=253,781
Collapsed=0
DockId=0x00000001,1

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Texture Array Test]
Pos=255,19
Size=1025,781
Collapsed=0
DockId=0x00000002,2

[Window][Type Chart]
Pos=255,19
Size=1025,781
Collapsed=0
DockId=0x00000002,0

[Window][AutoTiles]
Pos=0,19
Size=253,781
Collapsed=0
DockId=0x00000001,0

[Window][Map Display]
Pos=255,19
Size=1025,781
Collapsed=0
DockId=0x00000002,1

[Docking][Data]
DockSpace   ID=0x08BD597D Window=0x1BBC0F80 Pos=218,165 Size=1280,781 Split=X
  DockNode  ID=0x00000001 Parent=0x08BD597D SizeRef=253,781 Selected=0xDA164F2D
  DockNode  ID=0x00000002 Parent=0x08BD597D SizeRef=1025,781 CentralNode=1 Selected=0x85D93F07
)";

#pragma endregion


namespace TestWindow
{
	extern void ShowWindow() noexcept;
}

namespace PokemonEssentials::Resources
{
	extern void LoadAutotiles() noexcept;
	extern void LoadTilesets() noexcept;
}

namespace Window
{
	extern void MainDockSpaceViewport() noexcept;
	extern void AutoTiles() noexcept;
	extern void MapList() noexcept;
	extern void MapDisplay() noexcept;
	extern void TypeInfo() noexcept;
}

// Main code
int main(int argc, char* argv[]) noexcept
{
	if (argc == 2)
	{
		std::filesystem::path Candidate{ argv[1] };

		if (std::filesystem::exists(Candidate) && std::filesystem::is_directory(Candidate))
			PokemonEssentials::AssignGamePath(std::move(Candidate));
	}
	else
	{
		std::println("Default game path is used: '{}'", PokemonEssentials::GamePath.u8string());
	}

	std::jthread thread_LoadPBS{ [] static noexcept { PBS::Load(PokemonEssentials::GamePath); } },
		thread_LoadRxData{ [] static noexcept { Database::RX::Load(PokemonEssentials::GamePath); } };

	glfwSetErrorCallback(
		[](int error, const char* description) {
			fprintf(stderr, "GLFW Error %d: %s\n", error, description);
		}
	);
	if (!glfwInit())
		return 1;

	// GL 3.0 + GLSL 130
	static constexpr char glsl_version[] = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Create window with graphics context
	auto const main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
	auto const window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
	if (window == nullptr)
		return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	io.ConfigViewportsNoTaskBarIcon = true;

	io.IniFilename = nullptr; // Avoid using on-disk .ini file
	ImGui::LoadIniSettingsFromMemory(INI_CONFIG, sizeof(INI_CONFIG));

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
	io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
#endif

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//style.FontSizeBase = 20.0f;
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
	//IM_ASSERT(font != nullptr);

	// Our state
	static constexpr ImVec4 clear_color{ 0.45f, 0.55f, 0.60f, 1.00f };

	CompileBuiltinShader();

	PokemonEssentials::Resources::LoadAutotiles();
	PokemonEssentials::Resources::LoadTilesets();

	// SYNC POINT
	thread_LoadPBS.join();
	thread_LoadRxData.join();

	Game::CompileTilesets();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		Window::MainDockSpaceViewport();
		TestWindow::ShowWindow();
		Window::TypeInfo();
		Window::AutoTiles();
		Window::MapList();
		Window::MapDisplay();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
