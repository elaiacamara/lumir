#include <filesystem>

import vulkan_lib.app;
import lumir.app;
import syris.window;
import syris.statistics.statistics;

int main(){
    Syris::Statistics statistics;
    lumir::App::CreateInfo app_info{
        .window_info = Syris::Window::CreateInfo{
            .name = "engine",
            .dimmensions = {1000,1000},
            .vSync = true,
            .imgui = Syris::Window::ImGuiOptions{
                .main_dockspace = true
            },
            .path_to_shader_folder = std::filesystem::path(PROJECT_ROOT) / "assets" / "shaders",
            .statistics = statistics,
        }
    };
    lumir::App app(app_info);
    app.run();
    /*
    vkl::App app(1000,1000);
    app.run();*/
    return 0;
}