#pragma once

#include <Syris/utils/EngineTime.hpp>
#include <debug_lib/Result.hpp>
#include <functional>
#include <Syris/utils/FastFunction.hpp>
#include <GLFW/glfw3.h>
#include <vulkan-lib/Config.h>
#include <vulkan-lib/Swapchain.hpp>
#include <Syris/include/imgui.hpp>
#include <Syris/app/Window.hpp>
namespace lumir{
    class Renderer{
    public:
        using RenderFunction = std::function<db::Result<db::EmptyOk>(
                vk::CommandBuffer command_buffer, uint32_t frame_number, const Syris::engine_time::Time time)>;
        struct CreateInfo{
            vk::Device device;
            vkl::Swapchain& swapchain;
            Syris::Window* window;
            RenderFunction render_f;
            vk::Queue present_queue;
            vk::Queue graphics_queue;
            vk::RenderPass render_pass;
        };
        Renderer(const CreateInfo& info):
            m_device(info.device),
            m_swapchain(info.swapchain),
            m_max_frames_in_flight(m_swapchain.get_frames().size()),
            m_present_queue(info.present_queue),
            m_graphics_queue(info.graphics_queue),
            m_window(info.window),
            m_render_f(info.render_f),
            m_render_pass(info.render_pass){
            

        }

        void start(){
            m_time.start();
        }
        auto begin_frame() -> db::Result<const Syris::engine_time::Time>{
            m_time.next_frame();
			glfwPollEvents();
            if (m_device.waitForFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence,
                                       VK_TRUE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
                return db::error("failed to wait for fences");
            vk::ResultValue<uint32_t> image_index = m_device.acquireNextImageKHR(
                m_swapchain.get_swapchain(), std::numeric_limits<uint64_t>::max(),
                m_swapchain.get_frames()[m_frame_number].m_image_available, nullptr);
            if (image_index.result == vk::Result::eErrorOutOfDateKHR ||
                image_index.result == vk::Result::eSuboptimalKHR)
            {
                /*if (!recreate_swapchain())
                {
                    // if constexpr (_DEBUG)
                    db::Logger::core_error("CRITICAL ERROR failed to recreate swapchain\n");
                    return db::error("Failed to recreate swapchain");
                }*/
                return m_time;
            }
            else if (image_index.result != vk::Result::eSuccess)
            {
                // if constexpr (_DEBUG)
                db::Logger::core_error("failed to acquire the next image.\n");
                return db::error("Failed to acquire next image");
            }
            return m_time;
        }
        auto end_frame() -> db::Result<Syris::engine_time::Time>;

        auto render() -> db::Result<db::EmptyOk>{
            return render(m_render_f);
        }
        auto render(RenderFunction render) -> db::Result<db::EmptyOk>{
            m_time.next_frame();
			glfwPollEvents();
            if (m_device.waitForFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence,
                                       VK_TRUE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
                return db::error("failed to wait for fences");
            vk::ResultValue<uint32_t> image_index = m_device.acquireNextImageKHR(
                m_swapchain.get_swapchain(), std::numeric_limits<uint64_t>::max(),
                m_swapchain.get_frames()[m_frame_number].m_image_available, nullptr);
            if (image_index.result == vk::Result::eErrorOutOfDateKHR ||
                image_index.result == vk::Result::eSuboptimalKHR)
            {
                db::break_point();
                /*if (!recreate_swapchain())
                {
                    // if constexpr (_DEBUG)
                    db::Logger::core_error("CRITICAL ERROR failed to recreate swapchain\n");
                    return db::error("Failed to recreate swapchain");
                }*/
               return db::EmptyOk{};
            }
            else if (image_index.result != vk::Result::eSuccess)
            {
                // if constexpr (_DEBUG)
                db::Logger::core_error("failed to acquire the next image.\n");
                return db::error("Failed to acquire next image");
            }
            vk::CommandBuffer command_buffer = m_swapchain.get_frames()[m_frame_number].m_command_buffer;

            if (command_buffer.reset() != vk::Result::eSuccess)
                return db::error("Failed to reset command buffer");

            bool show = true;
            int width, height;
            glfwGetFramebufferSize(m_window->get_window(), &width, &height);

            ImGuiIO &io = ImGui::GetIO();
            io.DisplaySize = ImVec2((float)width, (float)height);
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
/*
            vk::CommandBufferBeginInfo beginInfo = {};
            if (command_buffer.begin(beginInfo) != vk::Result::eSuccess)
                return db::error("failed to begin main command buffer");;

            vk::RenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.renderPass = m_render_pass;
            renderPassInfo.framebuffer = m_swapchain.get_frames()[m_frame_number].m_framebuffer;
            renderPassInfo.renderArea.offset.x = 0;
            renderPassInfo.renderArea.offset.y = 0;
            renderPassInfo.renderArea.extent = m_swapchain.get_extent();
            vk::ClearValue clearColor = { std::array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            command_buffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
*/
            auto render_res = m_render_f(command_buffer, m_frame_number, m_time);
            if (!render_res)
                return db::error("Failed to render frame", render_res.error());

            ImGui::Render();
            ImDrawData* main_draw_data = ImGui::GetDrawData();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
            command_buffer.endRenderPass();

            if (command_buffer.end() != vk::Result::eSuccess)
                return db::error("Failed to end render pass");

            vk::SubmitInfo submitInfo = {};
            vk::Semaphore waitSemaphores[] = {
                m_swapchain.get_frames()[m_frame_number].m_image_available};
            vk::PipelineStageFlags waitStages[] = {
                vk::PipelineStageFlagBits::eColorAttachmentOutput};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &command_buffer;
            vk::Semaphore signalSemaphores[] = {
                m_swapchain.get_frames()[m_frame_number].m_render_finished};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
            if (m_device.resetFences(1, &m_swapchain.get_frames()[m_frame_number].m_in_flight_fence) !=
                vk::Result::eSuccess)
                return db::error("Failed to reset fences");
            if (m_graphics_queue.submit(submitInfo,
                                              m_swapchain.get_frames()[m_frame_number].m_in_flight_fence) !=
                vk::Result::eSuccess)
                return db::error("Failed to submit to graphics queue");

            vk::PresentInfoKHR presentInfo = {};
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            vk::SwapchainKHR swapchains[] = {m_swapchain.get_swapchain()};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &image_index.value;
            vk::Result resultPresent = m_present_queue.presentKHR(&presentInfo);
            if (resultPresent == vk::Result::eErrorOutOfDateKHR ||
                resultPresent == vk::Result::eSuboptimalKHR)
            {
                db::break_point();
                //if (!recreate_swapchain())
                //{
                //    return db::error("Failed to recreate swapchain");
               // }
                return db::EmptyOk{};
            }
            if (resultPresent != vk::Result::eSuccess)
                return db::error("Failed to present image");
            m_frame_number = (m_frame_number + 1) % m_max_frames_in_flight;
            return db::EmptyOk{};
        }

        private:
            vk::Device m_device;
            vkl::Swapchain m_swapchain;
            Syris::engine_time::Time m_time;
            uint32_t m_frame_number = 0;
            uint32_t m_max_frames_in_flight;
            vk::Queue m_present_queue;
            vk::Queue m_graphics_queue;
            Syris::Window* m_window;
            RenderFunction m_render_f;
            vk::RenderPass m_render_pass;
    };
}