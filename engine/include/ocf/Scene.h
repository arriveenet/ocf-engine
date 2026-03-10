#pragma once
#include <memory>
#include <vector>

namespace ocf {

class Node;
class Renderer;
class CameraComponent;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void update(float deltaTime);

    void render(Renderer* renderer);

    Node* getRoot() const { return m_root.get(); }

    void addCamera(CameraComponent* camera);
    void removeCamera(CameraComponent* camera);

protected:
    std::unique_ptr<Node> m_root;
    std::vector<CameraComponent*> m_cameras;
};

} // namespace ocf
