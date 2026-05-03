//import std;
//import vulkan_lib.app;
#include <vector>
#include <ranges>
#include <iostream>
#include "lumir/app/App.hpp"
#include <syris/app/Window.hpp>
#include <syris/statistics/statistics.hpp>
#include "lumir/world/World.hpp"
#include "lumir/app/Resources.hpp"
#include <print>


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
    lumir::Resources::init();
    lumir::Resources::get().thread_pool.enqueue([](){db::Logger::core_info("hello from another thread");});
    
    lumir::App::CreateInfo app_info{
        .window_info = Syris::Window::CreateInfo{
            .name = "engine",
            .dimmensions = {1500,1000},
            .vSync = true,
            .imgui = Syris::Window::ImGuiOptions{
                .main_dockspace = true
            },
            .path_to_shader_folder = std::filesystem::path(PROJECT_ROOT) / "assets" / "shaders",
            .statistics = statistics,
        },
        .world_info = lumir::World::CreateInfo{
            .chunk_render_count = 3,
            .chunk_dimmensions = {16,16,16},
        }
    };
    lumir::App app(app_info);
    auto res = app.run();
    if (!res)
        db::break_point();
    /*
    vkl::App app(1000,1000);
    app.run();*/
    return 0;
}