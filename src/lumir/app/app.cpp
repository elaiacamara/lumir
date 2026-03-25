module;
#include <vulkan-lib/Config.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
module lumir.app;
import vulkan_lib.instance;
import vulkan_lib.logging;
import vulkan_lib.device;
import vulkan_lib.queue_families;
import vulkan_lib.descriptors;
import vulkan_lib.framebuffer;
import vulkan_lib.pipeline;
import vulkan_lib.commands;
import syris.log.Logger;
import vulkan_lib.sync;
import vulkan_lib.scene;
import debug_lib.result;
import vulkan_lib.camera3D;
import vulkan_lib.renderStructs;
import <glm/gtc/matrix_transform.hpp>;

import <expected>;
import <format>;

namespace lumir {
	App::App(CreateInfo info) : m_window(info.window_info){
		//instance
        Syris::Logger::client_info(std::format("window is {:p}", (void *)m_window.get_window()));
		auto instance_res = vkInit::make_instance("voxel game", "voxel engine");
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
        m_graphics_queue = { m_device.getQueue(indices.graphicsFamily.value(), 0),
            indices.graphicsFamily.value() };
        m_present_queue = { m_device.getQueue(indices.presentFamily.value(), 0),
            indices.presentFamily.value() };
        m_transfer_queue = { m_device.getQueue(indices.transferFamily.value(), 0),
            indices.transferFamily.value() };

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
            
            //main command buffer
        vkl::CommandBufferInputBundle gpCommandPoolInput = {m_device, m_graphs_pres_command_pool, m_swapchain.get_frames()};
        auto main_command_buffer_res = vkl::make_command_buffer(gpCommandPoolInput);
        if (!main_command_buffer_res)
            throw std::runtime_error("Failed to make main command buffer");
        m_main_command_buffer = main_command_buffer_res.value();
            
            //transfer command buffer
        vkl::CommandBufferInputBundle transCommandBufferInput = {
            m_device, m_transfer_command_pool, m_swapchain.get_frames()};
        auto transfer_command_buffer_res = vkl::make_command_buffer(transCommandBufferInput);
        if (!transfer_command_buffer_res)
            throw std::runtime_error("Failed to make transfer command buffer");
        m_transfer_command_buffer = transfer_command_buffer_res.value();

        //swapchain
        vkl::Swapchain::CreateInfo swapchain_info{
            .logical_device = m_device,
            .physical_device = m_physical_device,
            .surface = m_surface,
            .width = info.window_info.dimmensions.x,
            .height = info.window_info.dimmensions.y,
            .command_pool = m_graphs_pres_command_pool
        };
        m_swapchain = vkl::Swapchain(swapchain_info);

        /*
        if (!vkl::query_swapchain_support(m_physical_device, m_surface))
            throw std::runtime_error("swapchain not supported");
        auto bundle_res = vkl::create_swapchain_bundle(m_device, m_physical_device, m_surface,
            info.window_info.dimmensions.x, info.window_info.dimmensions.y);
        if (!bundle_res)
            throw std::runtime_error("failed to create swapchain bundle");
        vkl::SwapChainBundle bundle = bundle_res.value();
        m_swapchain = bundle.swapchain;
        m_swapchain_extent = bundle.extent;
        m_swapchain_frames = bundle.frames;
        m_swapchain_format = bundle.format;
        m_max_frames_in_flight = static_cast<int>(m_swapchain_frames.size());
        m_frame_number = 0;*/

        
		//descriptor set layouts
        vkInit::DescriptorSetLayoutData vertex_bindings = {};
        vertex_bindings.count = 2;

        vertex_bindings.indices.push_back(0);
        vertex_bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
        vertex_bindings.counts.push_back(1);
        vertex_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

        vertex_bindings.indices.push_back(1);
        vertex_bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
        vertex_bindings.counts.push_back(1);
        vertex_bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

        auto vertex_descriptor_set_layout_res = vkInit::make_descriptor_set_layout(m_device, vertex_bindings);
		if (!vertex_descriptor_set_layout_res)
			throw std::runtime_error("failed to make descriptor set layout");
        m_vertex_descriptor_set_layout = vertex_descriptor_set_layout_res.value();
	    

        vkInit::DescriptorSetLayoutData fragment_bindings = {};
        fragment_bindings.count = 1;
        fragment_bindings.indices.push_back(0);
        fragment_bindings.types.push_back(vk::DescriptorType::eCombinedImageSampler);
        fragment_bindings.counts.push_back(1);
        fragment_bindings.stages.push_back(vk::ShaderStageFlagBits::eFragment);

        auto fragment_descriptor_set_layout_res = vkInit::make_descriptor_set_layout(m_device, fragment_bindings);
		if (!fragment_descriptor_set_layout_res)
			throw std::runtime_error("failed to make descriptor set layout");
        m_fragment_descriptor_set_layout = fragment_descriptor_set_layout_res.value();
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
        if (!vkl::make_frame_command_buffers(gpCommandPoolInput))
            throw std::runtime_error("Failed to make frame command buffers");
       
        //descriptor and frames
        auto descriptor_pool_res = vkInit::make_descriptor_pool(m_device, static_cast<uint32_t>(m_swapchain.get_frames().size() * 2), std::vector<vkInit::DescriptorSetLayoutData>{ vertex_bindings , fragment_bindings});
        if (!descriptor_pool_res)
            throw std::runtime_error("Failed to make descriptor pool");
        m_descriptor_pool = descriptor_pool_res.value();
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

        //assests
        std::vector<float> triangle_r = {
            0.0f, -0.05f, 1.0f, 0.0f, 0.0f,0.5f,0.0f,
            0.05f, 0.05f, 1.0f, 0.0f, 0.0f,1.0f,1.0f,
            -0.05f, 0.05f, 1.0f, 0.0f,  0.0f,0.0f,1.0f
        };
        /*std::vector<float> triangle_g = {
            0.0f, -0.05f, 0.0f,   1.0f,  0.0f, 0.05f, 0.05f, 0.0f,
            1.0f, 0.0f,   -0.05f, 0.05f, 0.0f, 1.0f,  0.0f,
        };
        std::vector<float> triangle_b = {
            0.0f, -0.05f, 0.0f, 0.0f,  1.0f,
            0.05f, 0.05f, 0.0f, 0.0f, 1.0f,
            -0.05f, 0.05f, 0.0f, 0.0f,  1.0f,
        };*/
        m_vertex_manager.consume(MeshType::TRIANGLE_R, triangle_r);
        //vertexManager->consume(MeshType::TRIANGLE_G, triangle_g);
        //vertexManager->consume(MeshType::TRIANGLE_B, triangle_b);
        auto vertex_finalize_res = m_vertex_manager.finalize(m_device, m_physical_device, m_transfer_queue.queue,
            m_transfer_command_buffer);
        if (!vertex_finalize_res)
            throw std::runtime_error("Failed to finalize vertex data");

        glm::vec3 eye = { 5.0f, 0.0f, -1.0f };
        glm::vec3 center = glm::vec3(0.0f);
        glm::vec3 up = { 0.0f, -1.0f, 0.0f };
        m_camera = vkInit::Camera{.eye = eye, .center = center, .up = up };
	}

	void App::run() {
        Scene scene;
		while (!m_window.should_window_close()) {
            auto res = render(scene);
            if (!res) {
                Syris::Logger::client_error(res.error().to_string());
                exit(1);
            }
        }
	}
    auto App::render(Scene& scene) -> db::Result<db::EmptyOk> {
        if (m_device.waitForFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence,
            VK_TRUE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
            return db::error("failed to wait for fences");
        vk::ResultValue<uint32_t> image_index = m_device.acquireNextImageKHR(
            m_swapchain.get_swapchain(), std::numeric_limits<uint64_t>::max(),
            m_swapchain.get_frames()[m_frame_number].m_image_available, nullptr);
        if (image_index.result == vk::Result::eErrorOutOfDateKHR ||
            image_index.result == vk::Result::eSuboptimalKHR) {
            if (!recreate_swapchain()) {
                if constexpr (_DEBUG)
                    Syris::Logger::core_error("CRITICAL ERROR failed to recreate swapchain\n");
                return db::error("Failed to recreate swapchain");
            }
            return db::EmptyOk{};
        }
        else if (image_index.result != vk::Result::eSuccess) {
            if constexpr (_DEBUG)
                Syris::Logger::core_error("failed to acquire the next image.\n");
            return db::error("Failed to acquire next image");
        }
        //todo
        //camera.update(delta);

        vk::CommandBuffer commandBuffer = m_swapchain.get_frames()[m_frame_number].m_command_buffer;

        if (commandBuffer.reset() != vk::Result::eSuccess)
            return db::error("Failed to reset command buffer");

        //prepare_frame(image_index.value, m_scene);

        //PREPRARE FRAME
        /*
        vkl::SwapchainFrame& _frame = m_swapchain.get_frames()[image_index.value];
                _frame.cameraData.viewProjection = m_camera.getViewProjection(m_swapchain.get_extent());

        memcpy(_frame.cameraDataWriteLocation,
            &(_frame.cameraData),
            sizeof(vkInit::UBO));

        size_t i = 0;
        _frame.modelTransforms[i++] = glm::translate(glm::mat4(1.0f), glm::vec3(0, -1, 0));

        memcpy(_frame.modelBufferWriteLocation, _frame.modelTransforms.data(), i * sizeof(glm::mat4));
        _frame.write_descriptor_set(m_device);
        */
        //RECORD
        
        if (!record_draw_buffer()) {
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
        submitInfo.pCommandBuffers = &commandBuffer;
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

    auto App::record_draw_buffer() -> db::Result<db::EmptyOk> {
        vk::CommandBufferBeginInfo beginInfo = {};
        if (m_main_command_buffer.begin(beginInfo) != vk::Result::eSuccess)
            return db::error("failed to begin main command buffer");;

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = m_renderpass;
        renderPassInfo.framebuffer = m_swapchain.get_frames()[m_frame_number].m_framebuffer;
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = m_swapchain.get_extent();
        vk::ClearValue clearColor = { std::array<float, 4>{0.0f, 0.5f, 0.25f, 1.0f} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        m_main_command_buffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

        m_main_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_layout, 0, 2, m_swapchain.get_frames()[m_frame_number].m_descriptor_sets.data(),0, nullptr);

        m_main_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

        vk::Buffer vertexBuffers[] = { m_vertex_manager.m_vertex_buffer.buffer };
        vk::DeviceSize offsets[] = { 0 };
        m_main_command_buffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

        m_main_command_buffer.draw(m_vertex_manager.m_sizes[0], 3, m_vertex_manager.m_offsets[0],
            0);

        m_main_command_buffer.endRenderPass();

        if (m_main_command_buffer.end() != vk::Result::eSuccess)
            return db::error("Failed to end render pass");
        return db::EmptyOk{};
    }
}