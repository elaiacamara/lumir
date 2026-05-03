#pragma once
#include <vulkan-lib/Memory.hpp>
#include <vulkan-lib/Mesh.hpp>
#include <glm/glm.hpp>
#include <atomic>
#include <mdspan>

#include "Voxel.hpp"
#include <vector>
namespace lumir{
    class Chunk{
    public:
        struct CreateInfo
        {
            glm::uvec3 dimmensions;
            glm::ivec3 position;
            std::function<Voxel(glm::vec3 position)> generator;
        };
        Chunk(const CreateInfo &info);
        Chunk(const Chunk &ref) = delete;
        ~Chunk();
        Chunk(Chunk &&other) noexcept
            : m_voxels_data(std::move(other.m_voxels_data)),
              m_voxels(other.m_voxels), // careful if this is tied to data!
              m_mesh(std::move(other.m_mesh)),
              m_mesh_up_to_date(other.m_mesh_up_to_date),
              m_dimmensions(other.m_dimmensions),
              m_position(other.m_position),
              m_chunk_position(other.m_chunk_position)
        {
            // manually copy atomic value
            m_ready.store(other.m_ready.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
        }

        auto generate_mesh(bool forced = false) -> vkl::Mesh &;
        void update_voxel(glm::vec3 position, Voxel voxel);
        glm::ivec3 get_chunk_position()const {return m_chunk_position;}
        bool is_ready() const {return m_ready.load();}
    private:
        void reset_span();
        std::vector<Voxel> m_voxels_data;
        std::mdspan<Voxel, std::dextents<size_t, 3>> m_voxels;
        vkl::Mesh m_mesh;
        bool m_mesh_up_to_date = false;
        glm::uvec3 m_dimmensions;
        glm::vec3 m_position;
        glm::ivec3 m_chunk_position;
        std::atomic<bool> m_ready{false};
        std::function<Voxel(glm::vec3 position)> m_generator;
    };
}