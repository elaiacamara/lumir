#pragma once
#include <Syris/utils/containers/Singleton.hpp>
#include <Syris/Jobs/ThreadPool.hpp>
namespace lumir{
    struct ResourcesImpl{
        Syris::ThreadPool thread_pool{8};
    };
    using Resources = Syris::Singleton<ResourcesImpl>;
}