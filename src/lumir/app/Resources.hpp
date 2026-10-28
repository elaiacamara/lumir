#pragma once
#include <utils-lib/containers/Singleton.hpp>
#include <Syris/Jobs/ThreadPool.hpp>
namespace lumir{
    struct ResourcesImpl{
        Syris::ThreadPool thread_pool{8};
    };
    using Resources = ulib::Singleton<ResourcesImpl>;
}