#pragma once

namespace ocf {

class Engine;

class Material {
    struct BuilderDetails;

public:
    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Material* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Material(Engine& engine, const Builder& builder);
    ~Material();

private:

};

} // namespace ocf
