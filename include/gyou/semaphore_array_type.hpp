#ifndef INCLUDE_GYOU_SEMAPHORE_ARRAY_TYPE_HPP_
#define INCLUDE_GYOU_SEMAPHORE_ARRAY_TYPE_HPP_

#include <array>

#include <corral/corral.h>
#include <magic_enum/magic_enum.hpp>

#include "gyou/structs/service_enum.hpp"
namespace gyou
{
    using semaphores_array_type
        = std::array<corral::Semaphore,
                     magic_enum::enum_count<gyou::Service>()>;
}

#endif  // INCLUDE_GYOU_SEMAPHORE_ARRAY_TYPE_HPP_
