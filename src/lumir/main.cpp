//import std;
//import vulkan_lib.app;
#include <vector>
#include <ranges>
#include <iostream>
import lumir.app;
import syris.window;
import syris.statistics.statistics;


int main(){

   /* std::vector<int> vi{0,1,2,3,4,5,6,7,8,9};
    auto is_even = [](int i) {return i % 2 == 0; };
    std::ranges::filter_view evens{ vi, is_even };
    for (int i : evens) {
        std::cout << i << "";
    }*/

    std::vector<int> i{ 0,1,2,3,4,5,6,7,8,9 };
    auto is_even = [](int i) {return i % 2 == 0; };

    std::ranges::filter_view evens{ i, is_even };
    for (int i : evens) {
        std::cout << i << "";
    }
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