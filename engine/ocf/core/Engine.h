// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/core/compiler.h"
#include "ocf/core/FrameCounter.h"
#include "ocf/math/vec2.h"

#include <functional>
#include <memory>
#include <vector>

struct ImGuiIO;

namespace ocf {

namespace audio {
class AudioSystem;
}

namespace rhi {
class Device;
}

class Window;
class SceneManager;
class Renderer;
class EventDispatcher;
class View;
class FontManager;
class TextureManager;
class ImGuiLayer;

/**
 * @class Engine
 * @brief Core engine class that manages the application lifecycle, rendering, and scene management.
 *
 * The Engine is the central component responsible for initializing the rendering device,
 * managing the render loop, updating scenes, and coordinating drawing operations.
 */
class OCF_API Engine {
public:
    using Device = rhi::Device;

    /**
     * @struct Config
     * @brief Configuration structure for Engine initialization.
     */
    struct Config {
        std::shared_ptr<Window> window; ///< Shared pointer to the application window
        EventDispatcher& eventDispatcher; ///< Reference to the event dispatcher
        bool vsync = true; ///< Synchronize presentation with the display refresh
    };

    /**
     * @brief Creates and initializes a new Engine instance.
     * @param config Configuration settings for the engine
     * @return Pointer to the newly created Engine instance
     * @note The caller is responsible for destroying the Engine instance using destroy()
     */
    static Engine* create(const Config& config);

    /**
     * @brief Destroys an Engine instance and releases all associated resources.
     * @param engine Pointer to the Engine instance to destroy
     */
    static void destroy(Engine* engine);

    /**
     * @brief Updates the engine and the active scenes for the current frame.
     *
     * This method should be called once per frame to update game logic,
     * physics, and other time-dependent operations.
     */
    void update();

    /**
     * @brief Renders the views of the active scenes to the window.
     *
     * This method should be called once per frame after update() to draw
     * all scene objects using the renderer.
     */
    void draw();

    /**
     * @brief Starts the main application loop.
     *
     * This method blocks until the application is terminated and handles
     * the continuous update and draw cycle.
     */
    void mainLoop();

    /**
     * @brief Retrieves the scene manager, which runs the scenes of the application.
     * @return Reference to the SceneManager instance
     */
    SceneManager& getSceneManager() const;

    /**
     * @brief Retrieves the rendering device.
     * @return Reference to the RHI Device instance
     */
    rhi::Device& getDevice() const;

    /**
     * @brief Retrieves the renderer instance.
     * @return Reference to the Renderer instance
     */
    Renderer& getRenderer() const;

    /**
     * @brief Retrieves the texture manager instance.
     * @return Reference to the TextureManager instance
     */
    TextureManager& getTextureManager() const;
    
    /**
     * @brief Retrieves the font manager instance.
     * @return Reference to the FontManager instance
     */
    FontManager& getFontManager() const;
  
    /**
     * @brief Retrieves the audio system instance.
     * @return Reference to the AudioSystem instance
     */
    audio::AudioSystem& getAudioSystem() const;

    /**
     * @brief Returns whether vertical synchronization (VSync) is enabled.
     * @return `true` if VSync is enabled, `false` if it is disabled.
     */
    bool isVsyncEnabled() const noexcept { return m_vsync; }

    /**
     * @brief Gets the current window size.
     * @return 2D integer vector containing width and height of the window
     */
    math::ivec2 getWindowSize() const;

    /**
     * @brief Gets the current framebuffer size.
     * @return 2D integer vector containing width and height of the framebuffer
     */
    math::ivec2 getFramebufferSize() const;

    /**
     * @brief Retrieves the frame counter instance.
     * @return Reference to the FrameCounter instance
     */
    const FrameCounter& getFrameCounter() const noexcept { return m_frameCounter; }

    /**
     * @brief Retrieves the event dispatcher instance.
     * @return Reference to the EventDispatcher instance for handling events.
     */
    EventDispatcher& getEventDispatcher() const noexcept { return m_eventDispatcher; }

    /**
     * @brief Configures Dear ImGui, e.g. to add fonts or change the style.
     * 
     * The function is called right away. When the engine is built without OCF_ENABLE_IMGUI,
     * it is not called at all.
     * 
     * @param configure Function given the ImGui IO of the engine's context
     */
    void configureImGui(const std::function<void(ImGuiIO&)>& configure);

private:
    /**
     * @brief Private constructor for Engine initialization.
     * @param config Configuration settings for the engine
     * @note Use create() to instantiate Engine instead
     */
    Engine(const Config& config);

    /**
     * @brief Private destructor.
     * @note Use destroy() to deallocate Engine instead
     */
    ~Engine();

    /**
     * @brief Initializes engine subsystems including device and renderer.
     * @return true if initialization was successful, false otherwise
     */
    bool init();

private:
    std::shared_ptr<Window> m_window;                   //!< Shared pointer to the application window
    std::unique_ptr<SceneManager> m_sceneManager;       //!< Unique pointer to the scene manager
    std::unique_ptr<Renderer> m_renderer;               //!< Unique pointer to the renderer instance
    std::unique_ptr<rhi::Device> m_device;              //!< Unique pointer to the rendering device
    std::unique_ptr<TextureManager> m_textureManager;   //!< Unique pointer to the texture manager
    std::unique_ptr<FontManager> m_fontManager;         //!< Unique pointer to the font manager
    std::unique_ptr<audio::AudioSystem> m_audioSystem;  //!< Unique pointer to the audio system
#if OCF_ENABLE_IMGUI
    std::unique_ptr<ImGuiLayer> m_imGuiLayer;           //!< Dear ImGui debug UI
#endif
    std::vector<View*> m_frameViews;       //!< Views drawn this frame, in drawing order
    FrameCounter m_frameCounter;           //!< Frame counter for tracking FPS
    EventDispatcher& m_eventDispatcher;    //!< Reference to the event dispatcher
    bool m_vsync = true;                   //!< Synchronize presentation with the display refresh
};

} // namespace ocf
