#include "World.hpp"
#include <vulkan-lib/Shader.hpp>
#include <filesystem>
#include <Syris/include/imgui.hpp>
#include <cstdlib>
#include <unordered_map>
#include <set>
#include "Solver.hpp"
namespace lumir{

    World::World(World::CreateInfo& info):
        //m_chunk_manager(ChunkManager::CreateInfo{
        //    .generator = std::bind_front(&World::generator, this),
        //    .initial_pos = {0,0,0},
        //    .chunk_render_count = info.chunk_render_count,
        //    .chunk_dimmensions = info.chunk_dimmensions,
        //}),
        m_chunk_render_count(info.chunk_render_count),
        m_chunk_dimmensions(info.chunk_dimmensions),
        m_shader_file_watcher((fs::path(PROJECT_ROOT) / "assets" / "shaders" / "compute" / "output" / "continental_shader.spv").string())
    {
        m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        m_noise.SetFrequency(0.01f); // controls scale (lower = smoother, higher = noisier)
       // m_chunk_manager.generate({0,0,0}, true);


        vkl::BufferInput buffer_input;
        buffer_input.device = m_graphics_context.device;
        buffer_input.physical_device = m_graphics_context.physical_device;
        buffer_input.properties = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
        buffer_input.size = sizeof(int);
        buffer_input.usage = vk::BufferUsageFlagBits::eStorageBuffer;
        {
            auto buffer_counter_res = vkl::create_buffer(buffer_input);
            if (!buffer_counter_res)
                throw std::runtime_error("Failed to create counter buffer");
            m_buffer_counter = buffer_counter_res.value();
            int value = 0;
            vkl::mapBuffer(m_graphics_context.device, m_buffer_counter, &value, 0, sizeof(int));
        }
        {
            auto buffer = vkl::create_buffer(buffer_input);
            if (!buffer)
                throw std::runtime_error("Failed to create counter buffer");
            m_ids_buffer = buffer.value();
        }
        {
            auto buffer = vkl::create_buffer(buffer_input);
            if (!buffer)
                throw std::runtime_error("Failed to create counter buffer");
            m_mappings_buffer = buffer.value();
        }

        //descriptor layout
        std::array<vk::DescriptorSetLayoutBinding,5> binding_compute;
        binding_compute[0].binding = 0;
        binding_compute[0].descriptorType = vk::DescriptorType::eStorageImage;
        binding_compute[0].descriptorCount = 1;
        binding_compute[0].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_compute[1].binding = 1;
        binding_compute[1].descriptorType = vk::DescriptorType::eStorageImage;
        binding_compute[1].descriptorCount = 1;
        binding_compute[1].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_compute[2].binding = 2;
        binding_compute[2].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_compute[2].descriptorCount = 1;
        binding_compute[2].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_compute[3].binding = 3;
        binding_compute[3].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_compute[3].descriptorCount = 1;
        binding_compute[3].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_compute[4].binding = 4;
        binding_compute[4].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_compute[4].descriptorCount = 1;
        binding_compute[4].stageFlags = vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutBinding binding_fragment{};
        binding_fragment.binding = 0;
        binding_fragment.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding_fragment.descriptorCount = 1;
        binding_fragment.stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_compute_info{};
        descriptor_set_layout_compute_info.bindingCount = binding_compute.size();
        descriptor_set_layout_compute_info.pBindings = binding_compute.data();
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
        std::array<vk::DescriptorPoolSize, 3> pool_sizes{
            vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2},
            vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 3},
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
        m_image_compute_dsc_set = descriptor_set_res.value[0];
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
        
        m_pipeline_layout = pipeline_layout_res.value;
        generate_pipeline();
        /*
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
        */
        int width = 1080;
        int height = 1080;
        auto render_texture_info = vkl::Image::CreateInfo{
            .source = std::nullopt,
            .dimmensions = {width,height},
            .format = vk::Format::eR32G32B32A32Sfloat,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            .layout = vk::ImageLayout::eGeneral,
            .access = vk::AccessFlagBits::eShaderWrite,
            .pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .context_type = vkl::GraphicsContext::Type::eGraphics,
            .descriptor_layout = vk::DescriptorSetLayout{},
            .descriptor_pool = m_graphics_context.descriptor_pool
        }; 
        m_render_image = std::make_unique<vkl::Image>(render_texture_info);

        auto compute_texture_info = vkl::Image::CreateInfo{
            .source = std::nullopt,
            .dimmensions = {width,height},
            .format = vk::Format::eR32G32B32A32Sint,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            .layout = vk::ImageLayout::eGeneral,
            .access = vk::AccessFlagBits::eShaderWrite,
            .pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .context_type = vkl::GraphicsContext::Type::eCompute,
            .descriptor_layout = vk::DescriptorSetLayout{},
            .descriptor_pool = m_graphics_context.descriptor_pool
        };
        
        m_compute_image = std::make_unique<vkl::Image>(compute_texture_info);
        generate_world_texture();
        m_image_fragment_dsc_set = ImGui_ImplVulkan_AddTexture(m_render_image->get_sampler(), m_render_image->get_image_view(), static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal));
        m_shader_file_watcher.set_up_to_date();
    }

    auto World::generator(glm::vec3 position) -> Voxel{
        float value = m_noise.GetNoise(position.x, position.z);
        float height = ((value + 1) / 2) * m_chunk_dimmensions.y;
        if (position.y < height)
            return {Voxel::Type::eSmile};
        return {Voxel::Type::eEmpty};
    }

    void World::change_position(glm::vec3 position){
        //m_chunk_manager.change_position(position);
    }
    void World::on_update(const Syris::OnUpdate& update){
        ImGui::Begin("scrool");
        static int zoom = 1;

        ImGui::DragInt("Zoom", &zoom, 1.0f, 1, 100);
        float scale = (float)zoom;
        ImGui::End();
        ImGui::Begin("world",
            nullptr,
            ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Image(static_cast<VkDescriptorSet>(m_image_fragment_dsc_set), ImVec2(1080 * scale, 1080 * scale));
        ImGui::End();
        /*if (m_shader_source_file_watcher.check_file() == ulib::FileWatcher::Event::Modified){
            int result = std::system("glslangValidator -V shader.comp -o compute.spv");
            if (result == 0){
                generate_pipeline();
                generate_world_texture();
                m_shader_file_watcher.set_up_to_date();
            }
        }*/
        if (m_shader_file_watcher.check_file() == ulib::FileWatcher::Event::Modified){
            
            db::Logger::core_info("shader updated");
            generate_pipeline();
            generate_world_texture();
            m_shader_file_watcher.set_up_to_date();
        }
    }

    void World::update_descriptor_sets(){
        vk::DescriptorImageInfo compute_image_info{};
        compute_image_info.imageView = m_compute_image->get_image_view();
        compute_image_info.imageLayout = vk::ImageLayout::eGeneral;
        vk::DescriptorImageInfo render_image_info{};
        render_image_info.imageView = m_render_image->get_image_view();
        render_image_info.imageLayout = vk::ImageLayout::eGeneral;
        vk::DescriptorBufferInfo buffer_counter_info{};
        buffer_counter_info.buffer = m_buffer_counter.buffer;
        buffer_counter_info.offset = 0;
        buffer_counter_info.range = VK_WHOLE_SIZE;
        vk::DescriptorBufferInfo ids_buffer_info{};
        ids_buffer_info.buffer = m_ids_buffer.buffer;
        ids_buffer_info.offset = 0;
        ids_buffer_info.range = VK_WHOLE_SIZE;
        vk::DescriptorBufferInfo mappings_buffer_info{};
        mappings_buffer_info.buffer = m_mappings_buffer.buffer;
        mappings_buffer_info.offset = 0;
        mappings_buffer_info.range = VK_WHOLE_SIZE;

        std::array<vk::WriteDescriptorSet, 5> writes;
        writes[0].dstSet = m_image_compute_dsc_set;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = vk::DescriptorType::eStorageImage;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &render_image_info;
        
        writes[1].dstSet = m_image_compute_dsc_set;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = vk::DescriptorType::eStorageImage;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &compute_image_info;
        
        writes[2].dstSet = m_image_compute_dsc_set;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorType = vk::DescriptorType::eStorageBuffer;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &buffer_counter_info;

        writes[3].dstSet = m_image_compute_dsc_set;
        writes[3].dstBinding = 3;
        writes[3].dstArrayElement = 0;
        writes[3].descriptorType = vk::DescriptorType::eStorageBuffer;
        writes[3].descriptorCount = 1;
        writes[3].pBufferInfo = &ids_buffer_info;

        writes[4].dstSet = m_image_compute_dsc_set;
        writes[4].dstBinding = 4;
        writes[4].dstArrayElement = 0;
        writes[4].descriptorType = vk::DescriptorType::eStorageBuffer;
        writes[4].descriptorCount = 1;
        writes[4].pBufferInfo = &mappings_buffer_info;

        m_graphics_context.device.updateDescriptorSets(writes, nullptr);
    }

    void World::generate_world_texture(){

        update_descriptor_sets();

        auto bind_dispatch = [this](vk::CommandBuffer command_buffer, std::unique_ptr<vkl::ComputePipeline> &pipeline)
        {
            command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->m_pipeline);
            command_buffer.dispatch((1080 + 7) / 8, (1080 + 7) / 8, 1);
            auto transition_image = m_compute_image->transition_image_layout_within_command(
                vk::ImageLayout::eGeneral, m_graphics_context.compute_command_pool, command_buffer, vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);
            if (!transition_image)
                throw std::runtime_error("Failed to transition image layout");
        };

        auto compute_command = [this](std::function<void(vk::CommandBuffer)> func){
            auto res = vkl::single_time_command(m_graphics_context.device, m_graphics_context.compute_command_pool, m_graphics_context.compute_queue.queue,
                [this, func](vk::CommandBuffer command_buffer) -> db::Result<db::EmptyOk>{
                    command_buffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eCompute,
                        m_compute_continental->m_layout,
                        0,
                        m_image_compute_dsc_set,
                        nullptr);
                    func(command_buffer);
                    return db::EmptyOk{};
                    });
        };

        auto get_counter = [this]() -> int{
            int *counter;
            vkMapMemory(m_graphics_context.device, m_buffer_counter.bufferMemory, 0, sizeof(int), 0, (void **)&counter);

            int counter_value = *counter;
            vkUnmapMemory(m_graphics_context.device, m_buffer_counter.bufferMemory);
            return counter_value;
        };
        auto set_counter = [this](int value){
            int *counter;
            vkMapMemory(m_graphics_context.device, m_buffer_counter.bufferMemory, 0, sizeof(int), 0, (void **)&counter);

            *counter = value;
            vkUnmapMemory(m_graphics_context.device, m_buffer_counter.bufferMemory);
        };

        //continents islands id and first step of continents ID
        compute_command([this, bind_dispatch](vk::CommandBuffer command_buffer){
            auto transition_compute_image_1 = m_compute_image->transition_image_layout_within_command(
                vk::ImageLayout::eGeneral, m_graphics_context.compute_command_pool, command_buffer, vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderWrite);
            if (!transition_compute_image_1)
                throw std::runtime_error("Failed to transition image layout");
            auto transition_render_image_1 = m_render_image->transition_image_layout_within_command(
                vk::ImageLayout::eGeneral, m_graphics_context.compute_command_pool, command_buffer, vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderWrite);
            if (!transition_render_image_1)
                throw std::runtime_error("Failed to transition image layout");

            bind_dispatch(command_buffer,m_compute_continental);

            for (int i = 0; i < 1000; i++)
            {
                bind_dispatch(command_buffer,m_compute_island_id);
                bind_dispatch(command_buffer,m_compute_island_id_2);
            }
            for (int i = 0; i < 1000; i++)
            {
                bind_dispatch(command_buffer,m_compute_continent_id);
                bind_dispatch(command_buffer,m_compute_continent_id2);
            }
            bind_dispatch(command_buffer, m_compute_continent_id3);
        });

        int counter_value = get_counter();
        set_counter(0);
        db::Logger::core_info(std::format("counter value is {}", counter_value));

        vkl::BufferInput buffer_input;
        buffer_input.device = m_graphics_context.device;
        buffer_input.physical_device = m_graphics_context.physical_device;
        buffer_input.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        buffer_input.size = sizeof(IDsDist) * counter_value;
        buffer_input.usage = vk::BufferUsageFlagBits::eStorageBuffer;
        auto buffer_res = vkl::create_buffer(buffer_input);
        if (!buffer_res)
            throw std::runtime_error("Failed to create buffer");
        m_ids_buffer = buffer_res.value();
        update_descriptor_sets();
        
        compute_command([this, bind_dispatch](vk::CommandBuffer command_buffer){
            bind_dispatch(command_buffer, m_compute_continent_id4);
        });
        
        IDsDist *ids;
        vkMapMemory(m_graphics_context.device, m_ids_buffer.bufferMemory, 0, sizeof(IDsDist) * counter_value, 0, (void **)&ids);

        std::span<IDsDist> ids_span(ids, counter_value);
        
        Solver solver;
            solver.fill(ids_span | std::views::transform([](const IDsDist& p) {
                    return std::array<int, 2>{ p.IDs.x, p.IDs.y };
                })
                | std::views::join);
        vkUnmapMemory(m_graphics_context.device, m_ids_buffer.bufferMemory);
        for (auto pair : ids_span)
            solver.pair(pair);
        std::unordered_map<int, Continent> continents = solver.get_continent_map();
        auto island_to_continent = solver.get_island_to_continent();
        db::Logger::core_info(std::format("total of {} continents", continents.size()));
        db::Logger::core_info(std::format("total of {} islands", island_to_continent.size()));

        vkl::BufferInput mappings_info{
            .device = m_graphics_context.device,
            .physical_device = m_graphics_context.physical_device,
            .size = sizeof(glm::ivec2) * island_to_continent.size(),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer,
            .properties = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        };
        auto mappings_buffer_res = vkl::create_buffer(mappings_info);
        if (!mappings_buffer_res)
            throw std::runtime_error("Failed to create mappings buffer");
        m_mappings_buffer = mappings_buffer_res.value();
        std::vector<glm::ivec2> mappings_contiguous;
        mappings_contiguous.reserve(island_to_continent.size());
        for (auto [island, continent] : island_to_continent){
            mappings_contiguous.emplace_back(island, continent);
        }
        if (!vkl::mapBuffer(m_graphics_context.device, m_mappings_buffer, mappings_contiguous.data(), 0, sizeof(glm::ivec2) * mappings_contiguous.size()))
            throw std::runtime_error("Failed to map mappings buffer");

        update_descriptor_sets();

        set_counter(island_to_continent.size());

        compute_command([this, bind_dispatch](vk::CommandBuffer command_buffer){
            bind_dispatch(command_buffer, m_compute_continent_id5);
        });
            
        set_counter(0);
        
        compute_command([this, bind_dispatch](vk::CommandBuffer command_buffer){
            bind_dispatch(command_buffer, m_compute_continent_area);
        });
        ContinentPos *continent_positions_map;
        vkMapMemory(m_graphics_context.device, m_ids_buffer.bufferMemory, 0, sizeof(ContinentPos) * counter_value, 0, (void **)&continent_positions_map);

        std::span<ContinentPos> continent_positions_span(continent_positions_map, counter_value);
        std::vector<ContinentQuad> continent_positions;
        continent_positions.reserve(continents.size());
        for (auto [i, id_pos] : std::views::enumerate(continent_positions_span)){
            continents[id_pos.id].m_quad.add(id_pos.pos);
        }

        /*
        std::array<vk::DescriptorSetLayoutBinding,5> binding_continent_generation;
        binding_continent_generation[0].binding = 0;
        binding_continent_generation[0].descriptorType = vk::DescriptorType::eStorageImage;
        binding_continent_generation[0].descriptorCount = 1;
        binding_continent_generation[0].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_continent_generation[1].binding = 1;
        binding_continent_generation[1].descriptorType = vk::DescriptorType::eStorageImage;
        binding_continent_generation[1].descriptorCount = 1;
        binding_continent_generation[1].stageFlags = vk::ShaderStageFlagBits::eCompute;

        binding_continent_generation[2].binding = 2;
        binding_continent_generation[2].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_continent_generation[2].descriptorCount = 1;
        binding_continent_generation[2].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_continent_generation[3].binding = 3;
        binding_continent_generation[3].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_continent_generation[3].descriptorCount = 1;
        binding_continent_generation[3].stageFlags = vk::ShaderStageFlagBits::eCompute;
        binding_continent_generation[4].binding = 4;
        binding_continent_generation[4].descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_continent_generation[4].descriptorCount = 1;
        binding_continent_generation[4].stageFlags = vk::ShaderStageFlagBits::eCompute;
        vk::DescriptorPoolCreateInfo continent_descriptor_pool_info{
            
        };
        
        for(auto& [id,continent] : continents){
            auto image_res = continent.prepare_image();
            if (!image_res)
                throw std::runtime_error("Failed to prepare image");
            db::Logger::core_info(std::format("Continent {}: {}", id, continent.m_quad.debug()));
        }*/
        
        auto res5 = vkl::single_time_command(m_graphics_context.device, m_graphics_context.compute_command_pool, m_graphics_context.compute_queue.queue,
            [this, bind_dispatch](vk::CommandBuffer command_buffer) -> db::Result<db::EmptyOk>{
                command_buffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eCompute,
                    m_compute_continental->m_layout,
                    0,
                    m_image_compute_dsc_set,
                    nullptr);
                bind_dispatch(command_buffer, m_compute_render);
                auto transition_image = m_render_image->transition_image_layout_within_command(
                vk::ImageLayout::eShaderReadOnlyOptimal, m_graphics_context.compute_command_pool, command_buffer, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
                if (!transition_image)
                    throw std::runtime_error("Failed to transition image layout");
                return db::EmptyOk{};
            }
        );
    }

    void World::generate_pipeline(){
        auto make_pipeline = [this](std::string shader_name) -> std::unique_ptr<vkl::ComputePipeline>{
            fs::path shader_path = fs::path(PROJECT_ROOT) / "assets" / "shaders" / "compute" / "output" / shader_name;
            auto shader_module_res = vkl::create_module(shader_path.string(), m_graphics_context.device);
            if (!shader_module_res)
                throw shader_module_res.error();

            auto pipeline_info = vkl::ComputePipeline::CreateInfo{
                .device = m_graphics_context.device,
                .shader_module = shader_module_res.value(),
                .pipeline_layout = m_pipeline_layout};
            return std::make_unique<vkl::ComputePipeline>(pipeline_info);
        };
        
        m_compute_continental = make_pipeline("continental_shader.spv");
        m_compute_island_id = make_pipeline("island_id_shader.spv");
        m_compute_island_id_2 = make_pipeline("island_id_shader2.spv");
        m_compute_continent_id = make_pipeline("continent_id_shader.spv");
        m_compute_continent_id2 = make_pipeline("continent_id_shader2.spv");
        m_compute_continent_id3 = make_pipeline("continent_id_shader3.spv");
        m_compute_continent_id4 = make_pipeline("continent_id_shader4.spv");
        m_compute_continent_id5 = make_pipeline("continent_id_shader5.spv");
        m_compute_continent_area = make_pipeline("continent_area_shader.spv");
        m_compute_render = make_pipeline("render_shader.spv");
    }
}