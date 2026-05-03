#pragma once
#include "Chunk.hpp"
#include <mdspan>
#include <glm/glm.hpp>
#include <NamedType/named_type.hpp>
#include <vector>
#include <vulkan-lib/Mesh.hpp>
#include <debug_lib/Result.hpp>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <Syris/utils/containers/SmartVector.hpp>

namespace lumir{
    class ChunkManager{
    public:
        struct CreateInfo{
            std::function<Voxel(glm::vec3 position)> generator;
            glm::vec3 initial_pos;
            uint32_t chunk_render_count;
            glm::vec3 chunk_dimmensions;
        };
        ChunkManager(const CreateInfo& info);
        ChunkManager() = delete;
        ChunkManager(const ChunkManager& ref) = delete;
        ChunkManager(ChunkManager&& ref) = default;

        void generate(glm::ivec3 new_chunk_pos, bool recreate);
        void change_position(glm::vec3 world_position);
        auto get_chunks(){return m_chunks_data.get_view() | std::views::filter([](const Chunk& chunk){return chunk.is_ready();});}
    private:
        std::function<Voxel(glm::vec3 position)> m_generator;
        glm::ivec3 m_chunk_position;
        uint32_t m_chunk_render_count;
        glm::uvec3 m_chunk_dimmensions;
        Syris::utils::SmartVector<Chunk,glm::ivec3> m_chunks_data;
        std::mdspan<Voxel, std::dextents<size_t, 3>> m_voxels;
    };
}