// SPDX-License-Identifier: MIT
#include "ocf/scene/Component.h"
#include "ocf/scene/Node.h"
#include "ocf/scene/SceneGraph.h"

#include <gtest/gtest.h>

#include <vector>

using namespace ocf;

TEST(NodeTest, AddComponentSetsTheOwner)
{
    Node node;
    Component* component = node.addComponent<Component>();

    EXPECT_EQ(component->getOwner(), &node);
}

TEST(NodeTest, WorldMatrixCombinesTheParents)
{
    SceneGraph sceneGraph;
    Node* parent = sceneGraph.getRoot()->createChild();
    parent->setPosition(math::vec3(10.0f, 20.0f, 0.0f));
    Node* child = parent->createChild();
    child->setPosition(math::vec3(1.0f, 2.0f, 0.0f));

    sceneGraph.update(0.0f);

    const math::vec4 origin = child->getWorldMatrix() * math::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(origin.x, 11.0f);
    EXPECT_FLOAT_EQ(origin.y, 22.0f);
}

TEST(SceneGraphTest, TraverseVisibleNodesSkipsHiddenSubtrees)
{
    SceneGraph sceneGraph;
    Node* root = sceneGraph.getRoot();
    Node* visible = root->createChild();
    Node* hidden = root->createChild();
    Node* hiddenChild = hidden->createChild();
    hidden->setVisible(false);

    std::vector<Node*> visited;
    sceneGraph.traverseVisibleNodes(root, [&visited](Node* node) { visited.push_back(node); });

    EXPECT_EQ(visited, (std::vector<Node*>{root, visible}));

    visited.clear();
    sceneGraph.traverseNodes(root, [&visited](Node* node) { visited.push_back(node); });

    EXPECT_EQ(visited, (std::vector<Node*>{root, visible, hidden, hiddenChild}));
}
