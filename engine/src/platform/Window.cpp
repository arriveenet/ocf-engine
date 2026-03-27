// SPDX - License - Identifier : MIT

#include "ocf/platform/Window.h"

namespace ocf {

Window::Window()
    : m_title("")
    , m_width(0)
    , m_height(0)
{
}

void Window::setTitle(std::string_view title)
{
    m_title = title;
}

std::string Window::getTitle() const
{
    return m_title;
}

} // namespace ocf
