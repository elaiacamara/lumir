#pragma once
#include <vulkan-lib/Config.h>
#include <FastNoiseLite.h>

#include <syris/utils/containers/Singleton.hpp>
#include <syris/Jobs/ThreadPool.hpp>
#include <syris/renderer/OnUpdate.hpp>
#include <vulkan-lib/ComputePipeline.hpp>
#include <vulkan-lib/Image.hpp>
#include <vulkan-lib/GraphicsContext.hpp>


#include "ChunkManager.hpp"
namespace lumir{
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

        auto get_chunk_manager() -> ChunkManager& {return m_chunk_manager;}
        void change_position(glm::vec3 position);
        void on_update(const Syris::OnUpdate& update);
    private:

        vkl::GraphicsContext& m_graphics_context = Syris::Singleton<vkl::GraphicsContext>::get();
        ChunkManager m_chunk_manager;
        FastNoiseLite m_noise;
        uint32_t m_chunk_render_count;
        glm::uvec3 m_chunk_dimmensions;
        std::unique_ptr<vkl::ComputePipeline> m_compute_pipeline;
        std::unique_ptr<vkl::Image> m_image;
        vk::DescriptorSet m_image_fragment_dsc_set;
    };
}