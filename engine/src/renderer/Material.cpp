#include "ocf/renderer/Material.h"

namespace ocf {

struct Material::BuilderDetails {
};

Material::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Material::Builder::~Builder()
{
    delete m_impl;
}

Material* Material::Builder::build(Engine& engine)
{
    Material* material = new Material(engine, *this);
    return material;
}

Material::Material(Engine& engine, const Builder& builder)
{
}

Material::~Material()
{
}

} // namespace ocf
