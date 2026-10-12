// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <imgui.h>

// ImGuiのコンテキスト生成・破棄とバージョン整合性のテスト
TEST(ImGuiTest, CreateAndDestroyContext)
{
    IMGUI_CHECKVERSION();

    ImGuiContext* context = ImGui::CreateContext();
    ASSERT_NE(context, nullptr);
    EXPECT_EQ(ImGui::GetCurrentContext(), context);
    EXPECT_STREQ(ImGui::GetVersion(), IMGUI_VERSION);

    ImGui::DestroyContext(context);
    EXPECT_EQ(ImGui::GetCurrentContext(), nullptr);
}
