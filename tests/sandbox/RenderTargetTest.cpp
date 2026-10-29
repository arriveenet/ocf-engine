// SPDX-License-Identifier: MIT
#include "RenderTargetTest.h"

#include <ocf/core/Engine.h>
#include <ocf/math/geometric.h>
#include <ocf/platform/FileSystem.h>
#include <ocf/renderer/RenderTarget.h>
#include <ocf/renderer/Texture.h>
#include <ocf/resource/ModelImporter.h>
#include <ocf/scene/3d/MeshInstance.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/SceneGraph.h>
#include <ocf/scene/View.h>

#include <imgui.h>

using namespace ocf;

void RenderTargetTest::onLoad()
{
    ModelImporter importer;
    auto result = importer.loadFromFile("models/FlightHelmet/glTF/FlightHelmet.gltf");
    if (result.isOk()) {
        m_mesh = result.unwrap();
    }

    m_modelNode = m_world->getRoot()->createChild();
}

void RenderTargetTest::onEnter()
{
    initializeScene();

    Engine& engine = getEngine();

    // The textures the world view draws into. The color one is then sampled by the sprite.
    m_colorTexture = Texture::Builder()
                         .width(TARGET_SIZE)
                         .height(TARGET_SIZE)
                         .format(Texture::InternalFormat::RGBA8)
                         .usage(Texture::Usage::ColorAttachment | Texture::Usage::Sampleable)
                         .build(engine);
    m_depthTexture = Texture::Builder()
                         .width(TARGET_SIZE)
                         .height(TARGET_SIZE)
                         .format(Texture::InternalFormat::DEPTH32F)
                         .usage(Texture::Usage::DepthAttachment)
                         .build(engine);
    m_renderTarget = RenderTarget::Builder()
                         .texture(RenderTarget::AttachmentPoint::Color, m_colorTexture)
                         .texture(RenderTarget::AttachmentPoint::Depth, m_depthTexture)
                         .build(engine);

    // The world is drawn into the render target before the screen view samples it
    m_view->setRenderTarget(m_renderTarget);
    m_view->setOrder(-1);
    m_view->setClearColor(m_targetClearColor);

    m_camera->perspective(math::radians(60.0f), 1.0f, 0.1f, 100.0f);
    m_camera->lookAt(math::vec3(0.0f, 0.5f, 1.5f), math::vec3(0.0f, 0.3f, 0.0f),
                     math::vec3(0.0f, 1.0f, 0.0f));

    if (m_mesh.ptr() != nullptr) {
        m_mesh->createSubMeshBuffers(engine);

        FileSystem* fileSystem = FileSystem::getInstance();
        const auto vsPath = fileSystem->getAssetFullPath("shaders/basic_pbr.vert.spv");
        const auto fsPath = fileSystem->getAssetFullPath("shaders/basic_pbr.frag.spv");

        m_modelNode->addComponent<MeshInstance>(engine, vsPath, fsPath)->setMesh(m_mesh);
    }

    // The screen shows the render target's color texture as a sprite
    const math::ivec2 size = engine.getWindowSize();
    const math::vec2 screenSize(static_cast<float>(size.x), static_cast<float>(size.y));

    m_screen = createSceneGraph();
    Camera* screenCamera = m_screen->getRoot()->createChild()->addComponent<Camera>();
    screenCamera->orthographic(0.0f, screenSize.x, 0.0f, screenSize.y);

    View* screenView = createView();
    screenView->setScene(m_screen);
    screenView->setCamera(screenCamera);
    screenView->setClearColor(math::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    screenView->setSortMode(View::SortMode::SubmitOrder);

    Node* spriteNode = m_screen->getRoot()->createChild();
    spriteNode->setPosition(math::vec3(screenSize.x * 0.5f, screenSize.y * 0.5f, 0.0f));
    m_sprite = spriteNode->addComponent<Sprite>(engine, m_colorTexture);
    m_sprite->setPixelsPerUnit(1.0f);
}

void RenderTargetTest::onExit()
{
    Engine& engine = getEngine();

    if (m_mesh.ptr() != nullptr) {
        m_mesh->terminate(engine);
    }

    // The device defers the destruction until in-flight frames are done with them
    m_renderTarget->terminate(engine);
    delete m_renderTarget;
    m_renderTarget = nullptr;

    m_colorTexture->terminate(engine);
    delete m_colorTexture;
    m_colorTexture = nullptr;

    m_depthTexture->terminate(engine);
    delete m_depthTexture;
    m_depthTexture = nullptr;
}

void RenderTargetTest::onUpdate(float deltaTime)
{
    m_rotation += m_rotationSpeed * deltaTime;
    m_modelNode->setRotation(math::vec3(0.0f, m_rotation, 0.0f));
}

void RenderTargetTest::onImGui()
{
    ImGui::Begin("RenderTargetTest");

    if (ImGui::ColorEdit3("Target Clear Color", &m_targetClearColor.x)) {
        m_view->setClearColor(m_targetClearColor);
    }

    ImGui::SliderFloat("Rotation Speed", &m_rotationSpeed, -180.0f, 180.0f);

    ImGui::End();
}
