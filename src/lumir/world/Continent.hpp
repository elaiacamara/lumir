#pragma once

#include <set>
#include <format>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>

#include <debug_lib/Result.hpp>
#include <vulkan-lib/Image.hpp>


namespace lumir{
    struct ContinentQuad{
        glm::ivec2 m_bot_left{std::numeric_limits<int>::max(), -1};
        glm::ivec2 m_top_right{-1,std::numeric_limits<int>::max()};
        void add(glm::ivec2 p);
        std::string debug();
    };

    struct Continent{
        std::set<int> m_island_ids;
        ContinentQuad m_quad;
        int m_ID;
        std::unique_ptr<vkl::Image> m_image;
        std::vector<int> m_covered_sectors;
        const glm::ivec2 m_world_dimmensions{1080,1080};
        const glm::ivec2 m_sector_distribution{10,10};
        const int m_sector_size{2048};

        Continent() = default;
        Continent(Continent&& moved)noexcept = default;
        auto prepare_image(vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout descriptor_layout) -> db::Result<db::EmptyOk>;
    };
}