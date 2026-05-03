#pragma once
#include <stdint.h>
namespace lumir{
    struct Voxel{
        enum class Type : uint8_t{
            eSmile,
            eEmpty
        };
        Type type;
    };
}