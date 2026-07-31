#pragma once

#include "ocf/core/Reference.h"

#include <string>

namespace ocf {

class Resource : public RefCounted {
public:
    Resource() = default;
    virtual ~Resource() = default;

    const std::string& getName() const noexcept { return m_name; }

protected:
    std::string m_name;
};

} // namespace ocf