
#include <imgui.h>

namespace Window
{
	void MainDockSpaceViewport() noexcept
	{
		ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_AutoHideTabBar);

#ifdef _DEBUG
		static bool show_demo_window = false;

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
#endif

		bool bShowAbout{};

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Application"))
			{
#ifdef _DEBUG
				ImGui::SeparatorText("Debug");
				ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
#endif
				ImGui::SeparatorText("Application");
				bShowAbout = ImGui::MenuItem("About");

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		//AboutDialog(bShowAbout);
	}
}
