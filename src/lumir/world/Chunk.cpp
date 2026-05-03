#include "Chunk.hpp"
#include <ranges>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "lumir/app/Resources.hpp"
#include <stacktrace>
namespace lumir{
    Chunk::Chunk(const Chunk::CreateInfo& info):
    m_dimmensions(info.dimmensions),
    m_position(info.position * (glm::ivec3)info.dimmensions),
    m_chunk_position(info.position),
    m_generator(info.generator)
    {
        Resources::get().thread_pool.enqueue([&](){
            m_voxels_data.resize(m_dimmensions.x * m_dimmensions.y * m_dimmensions.z);
            reset_span();
            for (uint32_t z = 0; z < m_dimmensions.z; z++){
                for (uint32_t y = 0; y < m_dimmensions.y; y++){
                    for (uint32_t x = 0; x < m_dimmensions.x; x++){
                        m_voxels[x,y,z] = m_generator(glm::vec3{x,y,z} + m_position);
                    }
                }
            }
            generate_mesh();
            m_ready.store(true);
        });
    }
    void Chunk::reset_span(){
        m_voxels = std::mdspan<Voxel, std::dextents<size_t, 3>>{
            m_voxels_data.data(),
            (size_t)m_dimmensions.x,
            (size_t)m_dimmensions.y,
            (size_t)m_dimmensions.z
        };
    }
    auto Chunk::generate_mesh(bool forced) -> vkl::Mesh&{
        if (m_mesh_up_to_date)
            return m_mesh;
        assert(m_voxels_data.size() == m_dimmensions.x * m_dimmensions.y * m_dimmensions.z);
        const auto ptr = this;
        auto call_stack = std::stacktrace::current();
        auto is_empty = [&](size_t x, size_t y, size_t z) -> bool
        {
            return m_voxels[x, y, z].type == Voxel::Type::eEmpty;
        };
        static const std::array<glm::ivec3, 6> dirs = {
            glm::ivec3(-1, 0, 0), // LEFT  (-X)
            glm::ivec3(1, 0, 0),  // RIGHT (+X)
            glm::ivec3(0, 1, 0),  // TOP   (+Y)
            glm::ivec3(0, -1, 0), // BOTTOM(-Y)
            glm::ivec3(0, 0, 1),  // FRONT (+Z)
            glm::ivec3(0, 0, -1)  // BACK  (-Z)
        };

        static const auto cube_vertices = vkl::getVertexData(vkl::MeshType::eCubeTex);

        for (size_t z = 0; z < m_dimmensions.z; ++z)
        for (size_t y = 0; y < m_dimmensions.y; ++y)
        for (size_t x = 0; x < m_dimmensions.x; ++x)
        {
            const auto voxel = m_voxels[x, y, z];
            if (voxel.type == Voxel::Type::eEmpty)
                continue;

            for (auto [index,d] : std::views::enumerate(dirs))
            {
                int nx = x + d.x;
                int ny = y + d.y;
                int nz = z + d.z;

                if (nx < 0 || ny < 0 || nz < 0 ||
                    nx >= m_dimmensions.x || ny >= m_dimmensions.y || nz >= m_dimmensions.z)
                {
                    // outside world → face is visible
                    continue;
                }

                if (m_voxels[nx, ny, nz].type == Voxel::Type::eEmpty)
                {
                    //db::Logger::core_info(std::format("added a face [{},{},{}] dir {}",x,y,z , glm::to_string(d)));
                    
                    std::vector<float> range = {cube_vertices.begin() + index * 6 * 5, cube_vertices.begin() + index * 6 * 5 + 6 * 5};
                    for (uint32_t i = 0; i < 6; i++){
                        range[i * 5] += x + m_position.x;
                        range[i * 5 + 1] += y + m_position.y;
                        range[i * 5 + 2] += z + m_position.z;
                    }
                    m_mesh.append(range);
                    // neighbor empty → face visible
                }
            }
        }
        m_mesh_up_to_date = true;
        return m_mesh;
    }
    Chunk::~Chunk(){
        
    }
}