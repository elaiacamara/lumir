#pragma once
#include <vulkan-lib/Config.h>
#include <FastNoiseLite.h>
#include <set>

#include <utils-lib/containers/Singleton.hpp>
#include <syris/Jobs/ThreadPool.hpp>
#include <syris/renderer/OnUpdate.hpp>
#include <vulkan-lib/ComputePipeline.hpp>
#include <vulkan-lib/Image.hpp>
#include <vulkan-lib/GraphicsContext.hpp>
#include <vulkan-lib/Memory.hpp>
#include <utils-lib/filesystem/FileWatcher.hpp>

#include "ChunkManager.hpp"
namespace lumir{
    template<typename T>
    class UniqueMap{
        public:
            UniqueMap(vk::DeviceMemory memory, uint32_t count = 1) : m_memory(memory){
                m_device = ulib::Singleton<vkl::GraphicsContext>::get().device;
                vkMapMemory(m_device, m_memory, 0, sizeof(T) * count, 0, (void **)&mapping);
            }
            ~UniqueMap(){
                vkUnmapMemory(m_device, m_memory);
            }
            T* get(){return m_mapping;}
        private:
            T* m_mapping;
            vk::Device m_device;
            vk::DeviceMemory m_memory;
    };
    class World{
    public:
        struct CreateInfo{
            uint32_t chunk_render_count;
            glm::uvec3 chunk_dimmensions;
        };
        World(CreateInfo& info);
        World(World&& move) = default;
        World() = delete;
        World(const World& ref) = delete;
        Voxel generator(glm::vec3 position);

        //auto get_chunk_manager() -> ChunkManager& {return m_chunk_manager;}
        void change_position(glm::vec3 position);
        void on_update(const Syris::OnUpdate& update);
        struct IDsDist{
            glm::ivec2 IDs;
            float dist;
            float padding;
        };
        struct ContinentPos{
            glm::ivec2 pos;
            float id;
            float padding;
        };
    private:

        void generate_world_texture();
        void generate_pipeline();
        void update_descriptor_sets();
        vkl::GraphicsContext& m_graphics_context = ulib::Singleton<vkl::GraphicsContext>::get();
        //ChunkManager m_chunk_manager;
        FastNoiseLite m_noise;
        uint32_t m_chunk_render_count;
        glm::uvec3 m_chunk_dimmensions;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continental;
        std::unique_ptr<vkl::ComputePipeline> m_compute_island_id;
        std::unique_ptr<vkl::ComputePipeline> m_compute_island_id_2;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_id;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_id2;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_id3;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_id4;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_id5;
        std::unique_ptr<vkl::ComputePipeline> m_compute_continent_area;
        std::unique_ptr<vkl::ComputePipeline> m_compute_render;
        std::unique_ptr<vkl::Image> m_compute_image;
        std::unique_ptr<vkl::Image> m_render_image;
        vkl::Buffer m_buffer_counter;
        vkl::Buffer m_ids_buffer;
        vkl::Buffer m_mappings_buffer;
        vk::DescriptorSet m_image_fragment_dsc_set;
        vk::DescriptorSet m_image_compute_dsc_set;
        vk::PipelineLayout m_pipeline_layout;
        ulib::FileWatcher m_shader_file_watcher;
    };
}