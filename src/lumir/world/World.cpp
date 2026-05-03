#include "World.hpp"
#include <vulkan-lib/Shader.hpp>
#include <filesystem>
#include <Syris/include/imgui.hpp>
namespace lumir{

    World::World(World::CreateInfo& info):
        m_chunk_manager(ChunkManager::CreateInfo{
            .generator = std::bind_front(&World::generator, this),
            .initial_pos = {0,0,0},
            .chunk_render_count = info.chunk_render_count,
            .chunk_dimmensions = info.chunk_dimmensions,
        }),
        m_chunk_render_count(info.chunk_render_count),
        m_chunk_dimmensions(info.chunk_dimmensions)
    {
        m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_noise.SetFrequency(0.01f); // controls scale (lower = smoother, higher = noisier)
        m_chunk_manager.generate({0,0,0}, true);


        //descriptor layout
        vk::DescriptorSetLayoutBinding binding_compute{};
        binding_compute.binding = 0;
        binding_compute.descriptorType = vk::DescriptorType::eStorageImage;
        binding_compute.descriptorCount = 1;
        binding_compute.stageFlags = vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutBinding binding_fragment{};
        binding_fragment.binding = 0;
        binding_fragment.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding_fragment.descriptorCount = 1;
        binding_fragment.stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_compute_info{};
        descriptor_set_layout_compute_info.bindingCount = 1;
        descriptor_set_layout_compute_info.pBindings = &binding_compute;
        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_fragment_info{};
        descriptor_set_layout_fragment_info.bindingCount = 1;
        descriptor_set_layout_fragment_info.pBindings = &binding_fragment;

        auto descriptor_layout_compute_res = m_graphics_context.device.createDescriptorSetLayout(descriptor_set_layout_compute_info);
        if (descriptor_layout_compute_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create descriptor set layout");

        auto descriptor_layout_fragment_res = m_graphics_context.device.createDescriptorSetLayout(descriptor_set_layout_fragment_info);
        if (descriptor_layout_fragment_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create descriptor set layout");
        //descriptor pool
        std::array<vk::DescriptorPoolSize, 2> pool_sizes{
            vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1},
            vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 5}};

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.poolSizeCount = pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = 5;

        auto pool_res =
            m_graphics_context.device.createDescriptorPool(poolInfo);
        if (pool_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create descriptor pool");

        //descriptor sets
        std::array<vk::DescriptorSetLayout, 2> layouts{descriptor_layout_compute_res.value, descriptor_layout_fragment_res.value};
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = pool_res.value;
        allocInfo.descriptorSetCount = layouts.size();
        allocInfo.pSetLayouts = layouts.data();

        auto descriptor_set_res =
            m_graphics_context.device.allocateDescriptorSets(allocInfo);
        if (descriptor_set_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to allocate descriptor set");
        auto descriptor_set = descriptor_set_res.value[0];
        m_image_fragment_dsc_set = descriptor_set_res.value[1];
        //pipeline
        vk::PipelineLayoutCreateInfo pipeline_layout_info(
            {},
            1,
            &descriptor_layout_compute_res.value,
            0,
            nullptr
        );

        auto pipeline_layout_res = m_graphics_context.device.createPipelineLayout(pipeline_layout_info);
        if (pipeline_layout_res.result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create pipeline layout");

        fs::path shader_path = fs::path(PROJECT_ROOT) / "assets" / "shaders" / "compute" / "compute.spv";
        auto shader_module_res = vkl::create_module(shader_path.string(), m_graphics_context.device);
        if (!shader_module_res)
            throw shader_module_res.error();
        auto pipeline_info = vkl::ComputePipeline::CreateInfo{
            .device = m_graphics_context.device,
            .shader_module = shader_module_res.value(),
            .pipeline_layout = pipeline_layout_res.value
        };

        m_compute_pipeline = std::make_unique<vkl::ComputePipeline>(pipeline_info);
        int width = 1080;
        int height = 1080;
        auto texture_info = vkl::Image::CreateInfo{
            .device = m_graphics_context.device,
            .physical_device = m_graphics_context.physical_device,
            .source = std::nullopt,
            .dimmensions = {width,height},
            .format = vk::Format::eR32G32B32A32Sfloat,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            .layout = vk::ImageLayout::eGeneral,
            .access = vk::AccessFlagBits::eShaderWrite,
            .pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .command_buffer = m_graphics_context.graph_pres_command_buffer,
            .command_pool = m_graphics_context.graph_pres_command_pool,
            .queue = m_graphics_context.graphics_queue.queue,
            .descriptor_layout = vk::DescriptorSetLayout{},
            .descriptor_pool = m_graphics_context.descriptor_pool
        };
        
        m_image = std::make_unique<vkl::Image>(texture_info);

        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;


        if (m_graphics_context.compute_command_buffer.begin(begin_info) != vk::Result::eSuccess)
            throw std::runtime_error("Failed to begin command buffer");
        m_graphics_context.compute_command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_compute_pipeline->m_pipeline);
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageView = m_image->get_image_view();
        imageInfo.imageLayout = vk::ImageLayout::eGeneral;

        vk::WriteDescriptorSet write{};
        write.dstSet = descriptor_set;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = vk::DescriptorType::eStorageImage;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        m_graphics_context.device.updateDescriptorSets(write, nullptr);
        m_graphics_context.compute_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            pipeline_layout_res.value,
            0,
            descriptor_set,
            nullptr);

    

        m_graphics_context.compute_command_buffer.dispatch((width + 7) / 8, (height + 7) / 8, 1);

        if (m_graphics_context.compute_command_buffer.end() != vk::Result::eSuccess)
            throw std::runtime_error("Failed to end command buffer");

		vk::SubmitInfo submit_info{};
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &m_graphics_context.compute_command_buffer;

		if (m_graphics_context.compute_queue.queue.submit(submit_info) != vk::Result::eSuccess)
			throw std::runtime_error("Failed to submit buffer to queue");
		if (m_graphics_context.compute_queue.queue.waitIdle() != vk::Result::eSuccess)
			throw std::runtime_error("Failed to wait idle");

        auto transition_res =  m_image->transition_image_layout(vk::ImageLayout::eShaderReadOnlyOptimal, m_graphics_context.graph_pres_command_pool, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
        if (!transition_res)
            throw std::runtime_error("failed to transtion image layout");
        /*
        vk::DescriptorImageInfo image_info_imagui{};
        image_info_imagui.imageView = m_image->get_image_view();
        image_info_imagui.sampler = m_image->get_sampler();
        image_info_imagui.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::WriteDescriptorSet imgui_write{};
        imgui_write.dstSet = descriptor_set_res.value[1];
        imgui_write.dstBinding = 0;
        imgui_write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        imgui_write.descriptorCount = 1;
        imgui_write.pImageInfo = &image_info_imagui;

        m_graphics_context.device.updateDescriptorSets(imgui_write, {});
        */
       m_image_fragment_dsc_set = ImGui_ImplVulkan_AddTexture(m_image->get_sampler(), m_image->get_image_view(), static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal));
    }

    auto World::generator(glm::vec3 position) -> Voxel{
        float value = m_noise.GetNoise(position.x, position.z);
        float height = ((value + 1) / 2) * m_chunk_dimmensions.y;
        if (position.y < height)
            return {Voxel::Type::eSmile};
        return {Voxel::Type::eEmpty};
    }

    void World::change_position(glm::vec3 position){
        m_chunk_manager.change_position(position);
    }
    void World::on_update(const Syris::OnUpdate& update){
        ImGui::Begin("world");
        ImGui::Image(static_cast<VkDescriptorSet>(m_image_fragment_dsc_set), {1080,1080});
        ImGui::End();
    }
}