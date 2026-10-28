// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/math/vec4.h"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace ocf {

class Camera;
class RenderTarget;
class SceneGraph;

/**
 * @brief A render pass: draws scene graphs from a camera into the frame or a render target.
 *
 * Views are drawn in ascending order. A view that does not clear color is drawn over the
 * views before it, which is how a UI is layered on top of the main scene. A view that draws
 * into a render target must come before the views that sample its textures. A view only
 * references its scene graphs, camera and render target; it does not own them.
 */
class OCF_API View {
public:
    enum class ClearFlags : uint8_t {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        All = Color | Depth,
    };

    enum class SortMode : uint8_t {
        Distance,    //!< Opaque, then masked, then blended back to front (3D)
        SubmitOrder, //!< Scene graph traversal order (UI, 2D)
    };

    View();
    virtual ~View();

    /** @brief Replace all scene graphs of the view with a single one. */
    void setScene(SceneGraph* sceneGraph);

    /** @brief The first scene graph, or nullptr. */
    SceneGraph* getScene() const { return m_sceneGraphs.empty() ? nullptr : m_sceneGraphs.front(); }

    void addSceneGraph(SceneGraph* sceneGraph);

    void removeSceneGraph(SceneGraph* sceneGraph);

    const std::vector<SceneGraph*>& getSceneGraphs() const noexcept { return m_sceneGraphs; }

    void setCamera(Camera* camera);

    Camera* getCamera() const { return m_camera; }

    bool hasCamera() const { return m_camera != nullptr; }

    /** @brief Views with a lower order are drawn first. */
    void setOrder(int order) { m_order = order; }

    int getOrder() const noexcept { return m_order; }

    /**
     * @brief Draw into the render target instead of the swapchain.
     *
     * nullptr draws into the swapchain. The camera's aspect ratio should match the target size.
     */
    void setRenderTarget(RenderTarget* renderTarget) { m_renderTarget = renderTarget; }

    RenderTarget* getRenderTarget() const noexcept { return m_renderTarget; }

    /**
     * @brief Set which attachments are cleared before the view is drawn.
     *
     * The first view of a frame drawn into the swapchain always clears both, since the
     * attachments start undefined. So does the first view ever drawn into a render target.
     */
    void setClearFlags(ClearFlags flags) { m_clearFlags = flags; }

    ClearFlags getClearFlags() const noexcept { return m_clearFlags; }

    void setClearColor(const math::vec4& color) { m_clearColor = color; }

    const math::vec4& getClearColor() const noexcept { return m_clearColor; }

    void setSortMode(SortMode mode) { m_sortMode = mode; }

    SortMode getSortMode() const noexcept { return m_sortMode; }

protected:
    std::vector<SceneGraph*> m_sceneGraphs;
    Camera* m_camera = nullptr;
    RenderTarget* m_renderTarget = nullptr;
    int m_order = 0;
    ClearFlags m_clearFlags = ClearFlags::All;
    SortMode m_sortMode = SortMode::Distance;
    math::vec4 m_clearColor = math::vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

constexpr View::ClearFlags operator|(View::ClearFlags lhs, View::ClearFlags rhs)
{
    return static_cast<View::ClearFlags>(
        static_cast<std::underlying_type_t<View::ClearFlags>>(lhs) |
        static_cast<std::underlying_type_t<View::ClearFlags>>(rhs));
}

constexpr bool hasFlag(View::ClearFlags flags, View::ClearFlags flag)
{
    return (static_cast<std::underlying_type_t<View::ClearFlags>>(flags) &
            static_cast<std::underlying_type_t<View::ClearFlags>>(flag)) != 0;
}

} // namespace ocf
