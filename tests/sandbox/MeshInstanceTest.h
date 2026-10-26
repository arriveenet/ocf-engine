// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/core/Reference.h>
#include <ocf/resource/Mesh.h>
#include <ocf/scene/Node.h>

class ocf::SceneGraph;
class ocf::Camera;

class MeshInstanceTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onLoad() override;
    void onImGui() override;

private:
    ocf::Ref<ocf::Mesh> m_mesh;
    ocf::Node* m_modelNode = nullptr;
};