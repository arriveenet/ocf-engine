// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

class MainScene : public TestBaseScene {
public:
    void onEnter() override;
    void onImGui() override;
};