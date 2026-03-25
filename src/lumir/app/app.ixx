module;
#include <vulkan-lib/Config.h>

export module lumir.app;

import lumir.MainWindow;
import syris.window;
import vulkan_lib.queue_families;
import vulkan_lib.Swapchain;
import vulkan_lib.SwapchainFrame;
import vulkan_lib.scene;
import vulkan_lib.Image;
import vulkan_lib.vertexManager;
import debug_lib.result;
import vulkan_lib.camera3D;
import <vector>;
import <unordered_map>;

namespace lumir {
	export class App {
	public:
		struct CreateInfo {
			Syris::Window::CreateInfo window_info;
		};
		App(CreateInfo info);
		~App() {};
		void run();
	private:
		auto render(Scene& scene) -> db::Result<db::EmptyOk>;
        auto recreate_swapchain() -> db::Result<db::EmptyOk>;
		auto record_draw_buffer() -> db::Result<db::EmptyOk>;
		//window
		lumir::MainWindow m_window;
		vk::SurfaceKHR m_surface;

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
        vk::CommandBuffer m_main_command_buffer;
        vk::CommandBuffer m_transfer_command_buffer;

        //sync objects
		int m_max_frames_in_flight;
		int m_frame_number = 0;

		//descriptors
		vk::DescriptorSetLayout m_vertex_descriptor_set_layout;
		vk::DescriptorSetLayout m_fragment_descriptor_set_layout;
        vk::DescriptorPool m_descriptor_pool;


		//assets
        std::unordered_map<MeshType, vkl::Image*> m_materials;
		VertexManager m_vertex_manager;
        
		vkInit::Camera m_camera;
	};
}
