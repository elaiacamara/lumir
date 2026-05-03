#pragma once
#include <NamedType/named_type.hpp>
#include <glm/glm.hpp>

namespace lumir{
    using ChunkPos = fluent::NamedType<glm::uvec3, struct ChunkPosTag>;
    using WorldPos = fluent::NamedType<glm::vec3, struct WorldPosTag>;

}