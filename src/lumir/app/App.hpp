#pragma once
#include <vulkan-lib/Config.h>
#include <vector>
#include <unordered_map>

#include "lumir/renderer/Renderer.hpp"
#include "lumir/window/Window.hpp"
#include "lumir/world/World.hpp"
#include <debug_lib/result.hpp>
#include <syris/app/Window.hpp>
#include <syris/renderer/camera/PerspectiveCameraController.hpp>
#include <syris/utils/EngineTime.hpp>
#include <Syris/statistics/Statistics.hpp>
#include <Syris/events/Event.hpp>
#include <Syris/Jobs/ThreadPool.hpp>
#include <vulkan-lib/QueueFamilies.hpp>
#include <vulkan-lib/Swapchain.hpp>
#include <vulkan-lib/SwapchainFrame.hpp>
#include <vulkan-lib/scene.hpp>
#include <vulkan-lib/Image.hpp>
#include <vulkan-lib/vertexManager.hpp>



namespace lumir {
	class App {
	public:
		struct CreateInfo {
			Syris::Window::CreateInfo window_info;
			World::CreateInfo world_info;
		};
		App(CreateInfo info);
		~App() {};
		auto run() -> db::Result<db::EmptyOk>;
		bool on_event(Syris::Event& event);
	private:
		auto render(Scene& scene, const Syris::engine_time::Time time) -> db::Result<db::EmptyOk>;
        auto recreate_swapchain() -> db::Result<db::EmptyOk>;
		auto record_draw_buffer(vk::CommandBuffer command_buffer, uint32_t frame_number, const Syris::engine_time::Time time) -> db::Result<db::EmptyOk>;
		//window
		lumir::MainWindow m_window;
		vk::SurfaceKHR m_surface;
		Syris::Statistics& m_statistics;

		//instance
		vk::Instance m_instance;

		//debug
		vk::DebugUtilsMessengerEXT m_debug_messenger{ nullptr };
        vk::detail::DispatchLoaderDynamic m_dldi;

		//device
		vk::Device m_device;
		vk::PhysicalDevice m_physical_device;

		//family queue
        vkl::Queue m_graphics_queue;
        vkl::Queue m_present_queue;
        vkl::Queue m_transfer_queue;
        vkl::Queue m_compute_queue;


        //swapchain
		vkl::Swapchain m_swapchain{ nullptr };
        /*vk::SwapchainKHR m_swapchain;
        std::vector<vkl::SwapchainFrame> m_swapchain_frames;
        vk::Format m_swapchain_format;
        vk::Extent2D m_swapchain_extent;
		*/

        //pipeline related variables
        vk::Pipeline m_pipeline;
        vk::RenderPass m_renderpass;
        vk::PipelineLayout m_layout;

        //commands
        vk::CommandPool m_graphs_pres_command_pool;
        vk::CommandPool m_transfer_command_pool;
        vk::CommandPool m_compute_command_pool;
        vk::CommandBuffer m_main_command_buffer;
        vk::CommandBuffer m_transfer_command_buffer;
        vk::CommandBuffer m_compute_command_buffer;

        //sync objects
		int m_max_frames_in_flight;
		//int m_frame_number = 0;

		//descriptors
		vk::DescriptorSetLayout m_vertex_descriptor_set_layout;
		vk::DescriptorSetLayout m_fragment_descriptor_set_layout;
        vk::DescriptorPool m_descriptor_pool;


		//assets
        std::unique_ptr<vkl::Image> m_texture;
		VkDescriptorSet m_texture_set;
		VertexManager m_vertex_manager;
        
		Syris::PerspectiveCameraController m_camera;
		std::unique_ptr<Renderer> m_renderer;
		std::unique_ptr<World> m_world;
		//Syris::ThreadPool m_thread_pool{8};
	};
}
