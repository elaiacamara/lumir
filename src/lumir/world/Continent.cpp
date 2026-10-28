#include "Continent.hpp"
#include <vulkan-lib/GraphicsContext.hpp>

namespace lumir{

    void ContinentQuad::add(glm::ivec2 p)
    {
        m_bot_left.x = std::min(m_bot_left.x, p.x);
        m_bot_left.y = std::max(m_bot_left.y, p.y);

        m_top_right.x = std::max(m_top_right.x, p.x);
        m_top_right.y = std::min(m_top_right.y, p.y);
    }
    std::string ContinentQuad::debug(){
        return std::format("[{},{}]", glm::to_string(m_bot_left), glm::to_string(m_top_right));
    }

    
    auto Continent::prepare_image(vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout descriptor_layout) -> db::Result<db::EmptyOk>{
        const glm::ivec2 top_left{m_quad.m_bot_left.x, m_quad.m_top_right.y};
        const glm::ivec2 bot_right{m_quad.m_top_right.x, m_quad.m_bot_left.y};

        const glm::ivec2 sector_top_left{
            top_left.x / (m_world_dimmensions.x / m_sector_distribution.x),
            top_left.y / (m_world_dimmensions.y / m_sector_distribution.y)};

        const glm::ivec2 sector_bot_left{
            top_left.x / (m_world_dimmensions.x / m_sector_distribution.x),
            top_left.y / (m_world_dimmensions.y / m_sector_distribution.y)};
        
        for (int y = top_left.y; y < bot_right.y; y++){
            for (int x = top_left.x; x < bot_right.x; x++){
                m_covered_sectors.emplace_back(y * bot_right.x - top_left.x + x);
            }
        }
        vkl::Image::CreateInfo height_image_info{
            .source = std::nullopt,
            .dimmensions = {m_sector_size,m_sector_size},
            .format = vk::Format::eR32Sfloat,
            .layers_count = static_cast<uint32_t>(m_covered_sectors.size()) * 2,
            .usage = vk::ImageUsageFlagBits::eStorage,
            .layout = vk::ImageLayout::eGeneral,
            .access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
            .pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .context_type = vkl::GraphicsContext::Type::eCompute,
            .descriptor_layout = descriptor_layout,
            .descriptor_pool = descriptor_pool
        };
        //water water-velocity sediment
        vkl::Image::CreateInfo wvs_image_info{
            .source = std::nullopt,
            .dimmensions = {m_sector_size,m_sector_size},
            .format = vk::Format::eR16G16B16A16Sfloat,
            .layers_count = static_cast<uint32_t>(m_covered_sectors.size()) * 2,
            .usage = vk::ImageUsageFlagBits::eStorage,
            .layout = vk::ImageLayout::eGeneral,
            .access = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
            .pipeline_stage = vk::PipelineStageFlagBits::eComputeShader,
            .context_type = vkl::GraphicsContext::Type::eCompute,
            .descriptor_layout = descriptor_layout,
            .descriptor_pool = descriptor_pool
        };
        return db::EmptyOk{};
    }
}