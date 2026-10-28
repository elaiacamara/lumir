#include "App.hpp"

#include <vulkan-lib/Config.h>
#include <GLFW/glfw3.h>
#include <Syris/include/imgui.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

#include <debug_lib/Logger.hpp>
#include <debug_lib/result.hpp>
#include <vulkan-lib/VulkanLib.hpp>

#include <syris/input/Input.hpp>
#include <Syris/utils/EngineTime.hpp>
#include <Syris/renderer/camera/PerspectiveCamera.hpp>
#include <Syris/renderer/OnUpdate.hpp>
#include <Syris/events/Event.hpp>
#include <utils-lib/containers/Singleton.hpp>

#include "lumir/renderer/Renderer.hpp"
//import std;

namespace lumir {
	App::App(CreateInfo info) :
        m_window(info.window_info),
        m_statistics(info.window_info.statistics),
        m_camera(Syris::PerspectiveCameraController::CreateInfo{
            .aspec_ratio = (float)info.window_info.dimmensions.x / info.window_info.dimmensions.y,
            .zoom_level = 1.0f,
            .statistics = info.window_info.statistics
        })
    {
        glfwSetWindowUserPointer(m_window.get_window(), this);
        glfwSetKeyCallback(m_window.get_window(), [](GLFWwindow* window, int key, int scancode, int action, int mods){
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

            Syris::EventKey event(key, scancode, action, mods);

            app->on_event(event);
        });
        glfwSetMouseButtonCallback(m_window.get_window(), [](GLFWwindow *window, int button, int action, int mods){
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

            Syris::EventMouse event(button, action, mods);
            app->on_event(event);
        });
        glfwSetCursorPosCallback(m_window.get_window(), [](GLFWwindow* window, double x, double y){
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

            Syris::EventCursorPos event(x, y);
            app->on_event(event);
        });
        glfwSetScrollCallback(m_window.get_window(), [](GLFWwindow* window, double xoff, double yoff){
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

            Syris::EventScroll event(xoff, yoff);
            app->on_event(event);
        });
        glfwSetFramebufferSizeCallback(m_window.get_window(), [](GLFWwindow* window, int w, int h){
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));

            Syris::EventWindowResize event(w, h);
            app->on_event(event);
        });
        //m_camera.set_position({0.0f, 0.0f, 2.0f});
		//instance
		Syris::Input::get(m_window.get_window());

		auto instance_res = vkl::make_instance("voxel game", "voxel engine");
		if (!instance_res)
			throw std::runtime_error("failed to make instance");
		m_instance = std::move(instance_res.value());

		//surface
		VkSurfaceKHR c_style_surface;
		if (glfwCreateWindowSurface(m_instance, m_window.get_window(), nullptr, &c_style_surface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface");
		m_surface = c_style_surface;
        
		//debug messenger
        m_dldi = vk::detail::DispatchLoaderDynamic(m_instance, vkGetInstanceProcAddr);
		auto debug_messenger_res = vkl::make_debug_messanger(m_instance, m_dldi);
		if (!debug_messenger_res)
			throw std::runtime_error("failed to make debug messenger");
		m_debug_messenger = std::move(debug_messenger_res.value());

		//physical device
		auto physical_device_res = vkl::choose_physical_device(m_instance);
		if (!physical_device_res)
			throw std::runtime_error("failed to choose physical device");
		m_physical_device = std::move(physical_device_res.value());

		//device
		auto device_res = vkl::create_device(m_physical_device, m_surface);
		if (!device_res)
			throw std::runtime_error("failed to make device");
		m_device = std::move(device_res.value());

		//queue families
        vkl::QueueFamilyIndices indices =
            vkl::get_queue(m_physical_device, m_device, m_surface);
        if (!indices.is_complete())
            throw std::runtime_error("Failed to get queue family indices");
        m_graphics_queue = { m_device.getQueue(indices.graphics_family.value(), 0),
            indices.graphics_family.value() };
        m_present_queue = { m_device.getQueue(indices.present_family.value(), 0),
            indices.present_family.value() };
        m_transfer_queue = { m_device.getQueue(indices.transfer_family.value(), 0),
            indices.transfer_family.value() };
        m_compute_queue = { m_device.getQueue(indices.compute_family.value(), 0),
            indices.compute_family.value() };

        //commands
            //graphics present command pool
        auto graphics_pres_command_pool_res = vkl::make_command_pool(m_device, m_physical_device, m_surface,
            m_graphics_queue.queueFamilyIndex);
        if (!graphics_pres_command_pool_res)
            throw std::runtime_error("Failed to make graphics present command pool");
        m_graphs_pres_command_pool = graphics_pres_command_pool_res.value();

            //transfer command pool
        auto transfer_command_pool_res = vkl::make_command_pool(m_device, m_physical_device, m_surface,
            m_transfer_queue.queueFamilyIndex);
        if (!transfer_command_pool_res)
            throw std::runtime_error("Failed to make transfer command pool");
        m_transfer_command_pool = transfer_command_pool_res.value();
            
            //compute command pool
        auto compute_command_pool_res = vkl::make_command_pool(m_device, m_physical_device, m_surface,
            m_compute_queue.queueFamilyIndex);
        if (!compute_command_pool_res)
            throw std::runtime_error("Failed to make transfer command pool");
        m_compute_command_pool = compute_command_pool_res.value();
        
            //main command buffer

        auto main_command_buffer_res = vkl::make_command_buffer(m_device, m_graphs_pres_command_pool);
        if (!main_command_buffer_res)
            throw std::runtime_error("Failed to make main command buffer");
        m_main_command_buffer = main_command_buffer_res.value();
            
            //transfer command buffer
        auto transfer_command_buffer_res = vkl::make_command_buffer(m_device, m_transfer_command_pool);
        if (!transfer_command_buffer_res)
            throw std::runtime_error("Failed to make transfer command buffer");
        m_transfer_command_buffer = transfer_command_buffer_res.value();
            
            //compute command buffer
        auto compute_command_buffer_res = vkl::make_command_buffer(m_device, m_compute_command_pool);
        if (!compute_command_buffer_res)
            throw std::runtime_error("Failed to make compute command buffer");
        m_compute_command_buffer = compute_command_buffer_res.value();

        //descriptor set layouts
        vkl::DescriptorSetLayoutData vertex_bindings = {};
        vertex_bindings.count = 2;

        vertex_bindings.indices.push_back(0);
        vertex_bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
        vertex_bindings.counts.push_back(1);
        vertex_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

        vertex_bindings.indices.push_back(1);
        vertex_bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
        vertex_bindings.counts.push_back(1);
        vertex_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

        auto vertex_descriptor_set_layout_res = vkl::make_descriptor_set_layout(m_device, vertex_bindings);
        if (!vertex_descriptor_set_layout_res)
            throw std::runtime_error("failed to make descriptor set layout");
        m_vertex_descriptor_set_layout = vertex_descriptor_set_layout_res.value();


        vkl::DescriptorSetLayoutData fragment_bindings = {};
        fragment_bindings.count = 2;
        fragment_bindings.indices.push_back(0);
        fragment_bindings.types.push_back(vk::DescriptorType::eCombinedImageSampler);
        fragment_bindings.counts.push_back(1);
        fragment_bindings.stages.push_back(vk::ShaderStageFlagBits::eFragment);


        fragment_bindings.indices.push_back(1);
        fragment_bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
        fragment_bindings.counts.push_back(1);
        fragment_bindings.stages.push_back(vk::ShaderStageFlagBits::eFragment);

        auto fragment_descriptor_set_layout_res = vkl::make_descriptor_set_layout(m_device, fragment_bindings);
        if (!fragment_descriptor_set_layout_res)
            throw std::runtime_error("failed to make descriptor set layout");
        m_fragment_descriptor_set_layout = fragment_descriptor_set_layout_res.value();
        
        //descriptor pool
        auto support_res = vkl::Swapchain::query_swapchain_support(m_physical_device, m_surface);
        if (!support_res)
            throw std::runtime_error("Failed to query swapchain support");
        vkl::SwapChainSupportDetails support = support_res.value();
        uint32_t frames_in_flight = std::min(
            support.capabilities.maxImageCount,
            support.capabilities.minImageCount + 1
        );
        
        auto descriptor_pool_res = vkl::make_descriptor_pool(m_device, static_cast<uint32_t>(frames_in_flight * 2), std::vector<vkl::DescriptorSetLayoutData>{ vertex_bindings, fragment_bindings});
        if (!descriptor_pool_res)
            throw std::runtime_error("Failed to make descriptor pool");
        m_descriptor_pool = descriptor_pool_res.value();

        ulib::Singleton<vkl::GraphicsContext>::init(vkl::GraphicsContext{
            .device = m_device,
            .physical_device = m_physical_device,
            .graphics_queue = m_graphics_queue,
            .present_queue = m_present_queue,
            .transfer_queue = m_transfer_queue,
            .compute_queue = m_compute_queue,
            .graph_pres_command_pool = m_graphs_pres_command_pool,
            .transfer_command_pool = m_transfer_command_pool,
            .compute_command_pool = m_compute_command_pool,
            .graph_pres_command_buffer = m_main_command_buffer,
            .transfer_command_buffer = m_transfer_command_buffer,
            .compute_command_buffer = m_compute_command_buffer,
            .descriptor_pool = m_descriptor_pool
        });
        auto texture_info = vkl::Image::CreateInfo{
            .source = vkl::Image::Source{std::filesystem::path(PROJECT_ROOT) / "assets" / "images" / "smile.jpg"},
            .dimmensions = {640,640},
            .format = vk::Format::eR8G8B8A8Srgb,
            .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .access = vk::AccessFlagBits::eShaderRead,
            .pipeline_stage = vk::PipelineStageFlagBits::eFragmentShader,
            .context_type = vkl::GraphicsContext::Type::eGraphics,
            .descriptor_layout = vk::DescriptorSetLayout{},
            .descriptor_pool = m_descriptor_pool
        };
        m_texture = std::make_unique<vkl::Image>(texture_info);

        //swapchain
        auto populate_descriptor_sets = [this](vkl::SwapchainFrame& frame) -> db::Result<db::EmptyOk>{
            {
                auto constexpr size = sizeof(glm::mat4) * 3;
                vkl::BufferInput camera_buffer_input{
                    .device = m_device,
                    .physical_device = m_physical_device,
                    .size = sizeof(glm::mat4) * 3,
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent};
                auto camera_buffer_res = vkl::create_buffer(camera_buffer_input);
                if (!camera_buffer_res)
                    return db::error("Failed to create camera buffer", camera_buffer_res.error());
                m_camera.set_buffer(camera_buffer_res.value());
                auto camera_map_res = vkl::mapBuffer(m_device, camera_buffer_res.value(), (void *)&m_camera.getCamera(), 0, sizeof(glm::mat4) * 3);
                if (!camera_map_res)
                    return db::error("Failed to map camera buffer", camera_map_res.error());
                vk::DescriptorBufferInfo buffer_info{
                    camera_buffer_res.value().buffer, // your VkBuffer
                    0,                                // offset
                    sizeof(glm::mat4) * 3             // or sizeof(your data)
                };

                vk::WriteDescriptorSet write_set_0{
                    frame.m_descriptor_sets[0], // dstSet
                    0,                          // dstBinding
                    0,                          // dstArrayElement
                    1,                          // descriptorCount
                    vk::DescriptorType::eUniformBuffer,
                    nullptr,
                    &buffer_info};

                m_device.updateDescriptorSets(write_set_0, {});
            }
            {
                vkl::BufferInput color_buffer_input{
                    .device = m_device,
                    .physical_device = m_physical_device,
                    .size = sizeof(glm::vec3),
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent};
                auto color_buffer_res = vkl::create_buffer(color_buffer_input);
                if (!color_buffer_res)
                    return db::error("Failed to create camera buffer", color_buffer_res.error());
                auto color = glm::vec3(1.0,0.0,0.0);
                auto camera_map_res = vkl::mapBuffer(m_device, color_buffer_res.value(), (void *)&color, 0, sizeof(glm::vec3));
                if (!camera_map_res)
                    return db::error("Failed to map camera buffer", camera_map_res.error());
                vk::DescriptorBufferInfo buffer_info{
                    color_buffer_res.value().buffer, // your VkBuffer
                    0,                                // offset
                    sizeof(glm::vec3)             // or sizeof(your data)
                };

                vk::DescriptorImageInfo image_info{
                    m_texture->get_sampler(),                                   // sampler
                    m_texture->get_image_view(),                                // image view
                    vk::ImageLayout::eShaderReadOnlyOptimal      // MUST match current layout
                };
                std::array<vk::WriteDescriptorSet, 2> write_set_1{vk::WriteDescriptorSet{
                    frame.m_descriptor_sets[1],                 // dstSet
                    0,                             // dstBinding
                    0,                             // dstArrayElement
                    1,                             // descriptorCount
                    vk::DescriptorType::eCombinedImageSampler,
				    &image_info,
                    nullptr
                },
                {
                    frame.m_descriptor_sets[1],                 // dstSet
                    1,                             // dstBinding
                    0,                             // dstArrayElement
                    1,                             // descriptorCount
                    vk::DescriptorType::eUniformBuffer,
				    nullptr,
                    &buffer_info
                }};
                m_device.updateDescriptorSets(write_set_1, {});
                return db::EmptyOk{};
            }
        };

            
        auto layouts_buffer = std::vector<vk::DescriptorSetLayout>{ m_vertex_descriptor_set_layout, m_fragment_descriptor_set_layout };
        vkl::Swapchain::CreateInfo swapchain_info{
            .logical_device = m_device,
            .physical_device = m_physical_device,
            .surface = m_surface,
            .width = info.window_info.dimmensions.x,
            .height = info.window_info.dimmensions.y,
            .command_pool = m_graphs_pres_command_pool,
			.descriptor_pool = m_descriptor_pool,
			.descriptor_set_layouts = layouts_buffer,
            .populate_descriptor_sets = populate_descriptor_sets
        };
        m_swapchain = vkl::Swapchain(swapchain_info);

		m_max_frames_in_flight = m_swapchain.get_frames().size();


        //pipeline
        auto shader_folder_path = std::filesystem::path(PROJECT_ROOT) / "assets" / "shaders" / "example";
		vkl::GraphicsPipelineBundle specs = {};
		specs.device = m_device;
		specs.vertexFilepath = (shader_folder_path / "vertex.spv").string();
		specs.fragmentFilepath = (shader_folder_path / "fragment.spv").string();
        specs.extent = m_swapchain.get_extent();
		specs.swapchainImageFormat = m_swapchain.get_format().format;
        specs.descriptor_set_layouts = { m_vertex_descriptor_set_layout, m_fragment_descriptor_set_layout };
		auto graphics_pipeline_res = vkl::make_graphics_pipeline(specs);
		if (!graphics_pipeline_res)
			throw std::runtime_error("failed to make graphics pipeline");
		vkl::GraphicsPipelineOutBundle graphics_pipeline = graphics_pipeline_res.value();

		m_pipeline = graphics_pipeline.pipeline;
		m_renderpass = graphics_pipeline.renderpass;
		m_layout = graphics_pipeline.layout;


        //framebuffers
        vkl::FramebufferInput framebufferInput;
        framebufferInput.device = m_device;
        framebufferInput.renderPass = m_renderpass;
        framebufferInput.swapchainExtent = m_swapchain.get_extent();
        if (!vkl::make_framebuffers(framebufferInput, m_swapchain.get_frames()))
            throw std::runtime_error("Failed to make framebuffers");


            //frame command buffers
        if (!vkl::make_frame_command_buffers(m_device, m_graphs_pres_command_pool, &m_swapchain.get_frames()))
            throw std::runtime_error("Failed to make frame command buffers");
       
        
        /*
        for (vkl::SwapchainFrame& frame : m_swapchain_frames) {
            auto frame_in_flight_fence_res = vkInit::make_fence(m_device);
            auto frame_image_available_res = vkInit::make_semaphore(m_device);
            auto frame_render_finished_res = vkInit::make_semaphore(m_device);

            if (!frame_in_flight_fence_res || !frame_image_available_res || !frame_render_finished_res)
                throw std::runtime_error("Failed to make sync objects");
            frame.inFlightFence = frame_in_flight_fence_res.value();
            frame.imageAvailable = frame_image_available_res.value();
            frame.renderFinished = frame_render_finished_res.value();

            if (!frame.make_descriptor_resources(m_device, m_physical_device))
                throw std::runtime_error("Failed to make descriptor resources");
            auto frame_descriptor_set_res = vkInit::allocate_descriptor_sets(m_device, m_descriptor_pool, { m_vertex_descriptor_set_layout, m_fragment_descriptor_set_layout });
            if (!frame_descriptor_set_res)
                throw std::runtime_error("Failed to make frame descriptor set");
            frame.descriptor_sets = frame_descriptor_set_res.value();
        }*/

        m_vertex_manager.consume(MeshType::TRIANGLE_R, vkl::getVertexData(vkl::MeshType::eCubeTex));
        //vertexManager->consume(MeshType::TRIANGLE_G, triangle_g);
        //vertexManager->consume(MeshType::TRIANGLE_B, triangle_b);
        auto vertex_finalize_res = m_vertex_manager.finalize(m_device, m_physical_device, m_transfer_queue.queue,
            m_transfer_command_buffer);
        if (!vertex_finalize_res)
            throw std::runtime_error("Failed to finalize vertex data");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup scaling
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(1.0f);   // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = 1.0f;   // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
        io.ConfigDpiScaleFonts = true;     // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
        io.ConfigDpiScaleViewports = true; // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_window.get_window(), true);
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        std::array<vk::DescriptorPoolSize, 11> pool_sizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 100),
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 100)
        };

        vk::DescriptorPoolCreateInfo pool_info{};
        pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_info.maxSets = 100;
        pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
        pool_info.pPoolSizes = pool_sizes.data();

        auto imgui_pool_res = m_device.createDescriptorPool(pool_info);
        if (imgui_pool_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create imgui descriptor pool");
        auto imgui_pool = imgui_pool_res.value;
        /*vkl::single_time_command(m_device, m_graphs_pres_command_pool, m_present_queue.queue, [this](vk::CommandBuffer command_buffer){
            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();*/

        ImGui_ImplVulkan_InitInfo init_info = {};


        // init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
        init_info.Instance = m_instance;
        init_info.PhysicalDevice = m_physical_device;
        init_info.Device = m_device;
        init_info.QueueFamily = m_present_queue.queueFamilyIndex;
        init_info.Queue = m_present_queue.queue;
        //init_info.PipelineCache = g_PipelineCache;
        init_info.DescriptorPool = imgui_pool;
        init_info.MinImageCount = 2;
        init_info.ImageCount = m_max_frames_in_flight;
        //init_info.Allocator = g_Allocator;
        init_info.PipelineInfoMain.RenderPass = m_renderpass; //wd->RenderPass;
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        //init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);

        m_renderer = std::make_unique<Renderer>(Renderer::CreateInfo{
            .device = m_device,
            .swapchain = m_swapchain,
            .window = &m_window,
            .render_f = std::bind_front(&App::record_draw_buffer, this),
            .present_queue = m_present_queue.queue,
            .graphics_queue = m_graphics_queue.queue,
            .render_pass = m_renderpass
        });

        m_camera.m_device = m_device;


        m_world = std::make_unique<World>(info.world_info);
        m_texture_set = ImGui_ImplVulkan_AddTexture(m_texture->get_sampler(), m_texture->get_image_view(), static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal));
	}

	auto App::run() -> db::Result<db::EmptyOk>{
        //Scene scene;
        //Syris::engine_time::Time engine_time;
        //engine_time.start();
        m_renderer->start();
		while (!m_window.should_window_close()) {
            auto res = m_renderer->render();
            //engine_time.next_frame();
			//glfwPollEvents();
            //m_window.on_update(engine_time);
            //auto res = render(scene, engine_time);
            if (!res) {
                db::Logger::client_error(res.error().to_string());
                return db::error("Error running the application", res.error());
            }
            //Syris::Input::is_key_pressed(GLFW_W);
        }
        return db::EmptyOk{};
	}
    auto App::render(Scene& scene, const Syris::engine_time::Time time) -> db::Result<db::EmptyOk> {
        /*
        if (m_device.waitForFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence,
            VK_TRUE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
            return db::error("failed to wait for fences");
        vk::ResultValue<uint32_t> image_index = m_device.acquireNextImageKHR(
            m_swapchain.get_swapchain(), std::numeric_limits<uint64_t>::max(),
            m_swapchain.get_frames()[m_frame_number].m_image_available, nullptr);
        if (image_index.result == vk::Result::eErrorOutOfDateKHR ||
            image_index.result == vk::Result::eSuboptimalKHR) {
            if (!recreate_swapchain()) {
                //if constexpr (_DEBUG)
                    db::Logger::core_error("CRITICAL ERROR failed to recreate swapchain\n");
                return db::error("Failed to recreate swapchain");
            }
            return db::EmptyOk{};
        }
        else if (image_index.result != vk::Result::eSuccess) {
            //if constexpr (_DEBUG)
                db::Logger::core_error("failed to acquire the next image.\n");
            return db::error("Failed to acquire next image");
        }

        vk::CommandBuffer command_buffer = m_swapchain.get_frames()[m_frame_number].m_command_buffer;

        if (command_buffer.reset() != vk::Result::eSuccess)
            return db::error("Failed to reset command buffer");
        m_camera.on_update(time);
        bool show = true;
        int width, height;
        glfwGetFramebufferSize(m_window.get_window(), &width, &height);

        auto extent = m_swapchain.get_extent();
        db::Logger::core_info(std::format("Swapchain: {}\n", extent.width, extent.height));

        int w, h;
        glfwGetFramebufferSize(m_window.get_window(), &w, &h);
        db::Logger::core_info(std::format("Framebuffer: {}\n", w,h));

        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&show);
        ImGui::Begin("show");
        ImGui::Text(std::format("{}",(show)).c_str());
        ImGui::End();

        m_statistics.on_update(time);

        ImGui::Render();
        ImDrawData* main_draw_data = ImGui::GetDrawData();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        if (!record_draw_buffer(command_buffer)) {
            return db::error("Failed to draw the command buffer");
        }

        vk::SubmitInfo submitInfo = {};
        vk::Semaphore waitSemaphores[] = {
            m_swapchain.get_frames()[m_frame_number].m_image_available };
        vk::PipelineStageFlags waitStages[] = {
            vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer;
        vk::Semaphore signalSemaphores[] = {
            m_swapchain.get_frames()[m_frame_number].m_render_finished };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        if (m_device.resetFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence) !=
            vk::Result::eSuccess)
            return db::error("Failed to reset fences");
        if (m_graphics_queue.queue.submit(submitInfo,
            m_swapchain.get_frames()[m_frame_number].m_in_flight_fence) !=
            vk::Result::eSuccess)
            return db::error("Failed to submit to graphics queue");

        vk::PresentInfoKHR presentInfo = {};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        vk::SwapchainKHR swapchains[] = { m_swapchain.get_swapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &image_index.value;
        vk::Result resultPresent = m_present_queue.queue.presentKHR(&presentInfo);
        if (resultPresent == vk::Result::eErrorOutOfDateKHR ||
            resultPresent == vk::Result::eSuboptimalKHR) {
            if (!recreate_swapchain()) {
                return db::error("Failed to recreate swapchain");
            }
            return db::EmptyOk{};
        }
        if (resultPresent != vk::Result::eSuccess)
            return db::error("Failed to present image");
        m_frame_number = (m_frame_number + 1) % m_max_frames_in_flight;
        */
        return db::EmptyOk{};
    }
    db::Result<db::EmptyOk> App::recreate_swapchain() {
        /*m_width = 0;
        m_height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwPollEvents();
        }
        if (device.waitIdle() != vk::Result::eSuccess)
            return std::unexpected(EmptyErr{});
        cleanup_swapchain();

        if (!make_swapchain())
            return std::unexpected(EmptyErr{});
        if (!make_framebuffers())
            return std::unexpected(EmptyErr{});
        if (!make_frame_resources())
            return std::unexpected(EmptyErr{});
        vkInit::CommandBufferInputBundle commandBufferInput = {
            device, graphsPresCommandPool, swapchainFrames };

        auto main_command_buffer_res = vkInit::make_command_buffer(commandBufferInput);
        if (!main_command_buffer_res)
            return std::unexpected(EmptyErr{});
        mainCommandBuffer = main_command_buffer_res.value();
        if (!vkInit::make_frame_command_buffers(commandBufferInput))
            return std::unexpected(EmptyErr{});
        return EmptyOk{};*/
        return db::error("not implemented");
    }

    auto App::record_draw_buffer(vk::CommandBuffer command_buffer, uint32_t frame_number, const Syris::engine_time::Time time) -> db::Result<db::EmptyOk> {
        vk::CommandBufferBeginInfo beginInfo = {};
        if (command_buffer.begin(beginInfo) != vk::Result::eSuccess)
            return db::error("failed to begin main command buffer");;

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_renderpass;
        renderPassInfo.framebuffer = m_swapchain.get_frames()[frame_number].m_framebuffer;
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = m_swapchain.get_extent();
        vk::ClearValue clearValues[2];
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.5f, 1.f});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        command_buffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapchain.get_extent().width;
        viewport.height = (float)m_swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        command_buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{};
        scissor.extent = m_swapchain.get_extent();

        command_buffer.setScissor(0, 1, &scissor);

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_layout, 0, 2, m_swapchain.get_frames()[frame_number].m_descriptor_sets.data(),0, nullptr);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

        /*m_world->change_position(m_camera.getCamera().get_position());

        auto chunks = m_world->get_chunk_manager().get_chunks();
        ImGui::Begin("Meshes");
        for (auto [index, chunk] : std::views::enumerate(chunks)){
            assert(chunk.is_ready());
            auto& mesh = chunk.generate_mesh();
            auto mesh_buffer_res = mesh.get_buffer(m_device, m_physical_device, m_transfer_queue.queue, m_transfer_command_buffer);
            if (!mesh_buffer_res)
                return db::error("Failed to generate mesh", mesh_buffer_res.error());
            if (!mesh_buffer_res.value().has_value())
                continue;
            vk::Buffer vertexBuffers[] = {mesh_buffer_res.value().value().buffer};
            vk::DeviceSize offsets[] = {0};
            command_buffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
            //ImGui::Text(std::format("Mush {} has {} vertices.",index, mesh.get_vertices_count()).c_str());
            command_buffer.draw(mesh.get_vertices_count(), 1, 0, 0);
        }
        ImGui::Image(m_texture_set, {640,640});
        ImGui::End();*/
        const auto update = Syris::OnUpdate{time, frame_number};
        m_camera.on_update(update);
        m_world->on_update(update);

        m_statistics.on_update(time);
        /*
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
        command_buffer.endRenderPass();

        if (command_buffer.end() != vk::Result::eSuccess)
            return db::error("Failed to end render pass");
        */
        return db::EmptyOk{};
    }
	bool App::on_event(Syris::Event& event){
        static bool mouse_locked = false;
        if (event.get_type() == Syris::EventType::KeyPressed){
            auto& key_event = static_cast<Syris::EventKey&>(event);
            if (key_event.get_key() == GLFW_KEY_ESCAPE && key_event.get_action() == GLFW_PRESS)
            {
                mouse_locked = !mouse_locked;

                if (mouse_locked)
                {
                    glfwSetInputMode(m_window.get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                else
                {
                    glfwSetInputMode(m_window.get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                return true;
            }
            
        }
        if (mouse_locked)
            return m_camera.on_event(event);
        return false;
    }

}