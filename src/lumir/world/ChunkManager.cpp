#include "ChunkManager.hpp"
#include <ranges>
#include <Syris/include/imgui.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
namespace lumir{
    ChunkManager::ChunkManager(const ChunkManager::CreateInfo& info):
        m_generator(info.generator),
        m_chunk_position(info.initial_pos),
        m_chunk_render_count(info.chunk_render_count),
        m_chunk_dimmensions(info.chunk_dimmensions),
        m_chunks_data(std::pow(info.chunk_render_count * 2 + 1, 3)){
        
        generate(info.initial_pos, true);
    }

/*
    void ChunkManager::reset_span(){
        m_voxels = std::mdspan<Voxel, std::dextents<size_t, 3>>{
            m_voxels_data.data(),
            (size_t)m_dimmensions.x,
            (size_t)m_dimmensions.y,
            (size_t)m_dimmensions.z
        };
    }
*/
    void ChunkManager::generate(glm::ivec3 new_chunk_pos, bool recreate){
        Chunk::CreateInfo chunk_info{
            .dimmensions = m_chunk_dimmensions,
            .position = {0, 0, 0},
            .generator = m_generator};
        bool intersected = false;
        // bounding boxes
        glm::ivec3 old_min = m_chunk_position - glm::ivec3{(int)m_chunk_render_count};
        glm::ivec3 old_max = m_chunk_position + glm::ivec3{(int)m_chunk_render_count};

        glm::ivec3 new_min = new_chunk_pos - glm::ivec3{(int)m_chunk_render_count};
        glm::ivec3 new_max = new_chunk_pos + glm::ivec3{(int)m_chunk_render_count};

        // intersections
        int32_t ix_min = std::max(old_min.x, new_min.x);
        int32_t ix_max = std::min(old_max.x, new_max.x);

        int32_t iy_min = std::max(old_min.y, new_min.y);
        int32_t iy_max = std::min(old_max.y, new_max.y);

        int32_t iz_min = std::max(old_min.z, new_min.z);
        int32_t iz_max = std::min(old_max.z, new_max.z);

        if ((ix_min <= ix_max &&
            iy_min <= iy_max &&
            iz_min <= iz_max) && !recreate)
        { // intersection
            intersected = true;
            glm::ivec3 intersection_min = {ix_min, iy_min, iz_min};
            glm::ivec3 intersection_max = {ix_max, iy_max, iz_max};
            Chunk::CreateInfo chunk_info{
                .dimmensions = m_chunk_dimmensions,
                .position = {0, 0, 0},
                .generator = m_generator};
            for (uint32_t z = 0; z < m_chunk_render_count * 2 + 1; z++){
                for (uint32_t y = 0; y < m_chunk_render_count * 2 + 1; y++){
                    for (uint32_t x = 0; x < m_chunk_render_count * 2 + 1; x++){
                        glm::ivec3 pos = glm::ivec3{x, y, z} + old_min;
                        if (!(glm::all(glm::greaterThanEqual(pos, intersection_min)) && glm::all(glm::lessThanEqual(pos, intersection_max)))){
                            while (!m_chunks_data.get(pos).is_ready()){}
                            m_chunks_data.remove(pos);
                        }
                    }
                }
            }
            uint32_t added_count = 0;
            for (uint32_t z = 0; z < m_chunk_render_count * 2 + 1; z++){
                for (uint32_t y = 0; y < m_chunk_render_count * 2 + 1; y++){
                    for (uint32_t x = 0; x < m_chunk_render_count * 2 + 1; x++){
                        glm::ivec3 pos = glm::ivec3{x, y, z} + new_min;
                        if (!(glm::all(glm::greaterThanEqual(pos, intersection_min)) && glm::all(glm::lessThanEqual(pos, intersection_max))))
                        {
                            chunk_info.position = pos;
                            m_chunks_data.add_emplace(pos, chunk_info);
                        }
                    }
                }
            }
                
        }
        else
        { // no intersection
            intersected = false;
            uint32_t index = 0;
            Chunk::CreateInfo chunk_info{
                .dimmensions = m_chunk_dimmensions,
                .position = {0, 0, 0},
                .generator = m_generator};
            for (auto& chunk : m_chunks_data.get_view()){
                while(!chunk.is_ready()){}
            }
            m_chunks_data.clear();
            for (uint32_t z = 0; z < m_chunk_render_count * 2 + 1; z++)
            {
                for (uint32_t y = 0; y < m_chunk_render_count * 2 + 1; y++)
                {
                    for (uint32_t x = 0; x < m_chunk_render_count * 2 + 1; x++)
                    {
                        glm::ivec3 pos = glm::ivec3{x, y, z} + new_chunk_pos - (int)m_chunk_render_count;
                        chunk_info.position = pos;
                        m_chunks_data.add_emplace(pos, chunk_info);
                    }
                }
            }
        }
    }

    void ChunkManager::change_position(glm::vec3 position){
        glm::ivec3 new_chunk_pos = (glm::ivec3)position / (glm::ivec3)m_chunk_dimmensions;
        if (new_chunk_pos != m_chunk_position)
            generate(new_chunk_pos, false);
        m_chunk_position = new_chunk_pos;
        ImGui::Begin("Chunk position");
        ImGui::Text(glm::to_string(new_chunk_pos).c_str());
        ImGui::End();
        ImGui::Begin("Old Chunk position");
        ImGui::Text(glm::to_string(m_chunk_position).c_str());
        ImGui::End();
    }
}