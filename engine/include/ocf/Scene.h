#pragma once
#include <memory>
#include <vector>

namespace ocf {

class Node;
class Renderer;
class Camera;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void update(float deltaTime);

    void render(Renderer* renderer);

    Node* getRoot() const { return m_root.get(); }

    void addCamera(Camera* camera);
    void removeCamera(Camera* camera);

protected:
    std::unique_ptr<Node> m_root;
    std::vector<Camera*> m_cameras;
};

} // namespace ocf
