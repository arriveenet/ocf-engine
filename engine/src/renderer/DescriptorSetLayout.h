// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/RHIEnums.h"
#include "ocf/rhi/Handle.h"

#include <unordered_map>
#include <string>

namespace ocf {

class Engine;

struct UniformMember {
    std::string name;
    rhi::UniformType type;
    size_t offset;
    size_t size;
};

// 1つのバインディング(UBO)の情報
struct UniformBlockInfo {
    uint32_t binding;
    std::string name;
    size_t totalSize;
    std::unordered_map<std::string, UniformMember> members;
};

// 1つのバインディング(テクスチャ)の情報
struct TextureInfo {
    uint32_t binding;
    std::string name;
    rhi::SamplerType samplerType;
    rhi::ShaderStageFlags shaderStageFlags;
};

class DescriptorSetLayout {
public:
    using DescriptorSetLayoutHandle = rhi::DescriptorSetLayoutHandle;

    DescriptorSetLayout() = default;
    virtual ~DescriptorSetLayout() = default;

    void create(Engine& engine);
    void terminate(Engine& engine);

    void addUniformBlock(uint32_t binding, const std::string& name, size_t totalSize)
    {
        m_blocks[name] = {binding, name, totalSize, {}};
        m_bindingToBlockName[binding] = name;
    }

    void addUniformMember(const std::string& blockName, const std::string& memberName,
                          rhi::UniformType type, size_t offset, size_t size)
    {
        m_blocks[blockName].members[memberName] = {memberName, type, offset, size};
    }

    void addTexture(uint32_t binding, const std::string& name,
                    rhi::SamplerType samplerType = rhi::SamplerType::Sampler2D,
                    rhi::ShaderStageFlags shaderStageFlags = rhi::ShaderStageFlags::Fragment)
    {
        m_textures[name] = {binding, name, samplerType, shaderStageFlags};
    }

    const UniformBlockInfo* getBlockInfo(const std::string& blockName) const
    {
        auto it = m_blocks.find(blockName);
        return it != m_blocks.end() ? &it->second : nullptr;
    }

    const TextureInfo* getTextureInfo(const std::string& name) const
    {
        auto it = m_textures.find(name);
        return it != m_textures.end() ? &it->second : nullptr;
    }

    DescriptorSetLayoutHandle getHandle() const { return m_handle; }

    const std::unordered_map<std::string, UniformBlockInfo>& getBlocks() const { return m_blocks; }

    const std::unordered_map<std::string, TextureInfo>& getTextures() const { return m_textures; }

private:
    DescriptorSetLayoutHandle m_handle;
    std::unordered_map<std::string, UniformBlockInfo> m_blocks;
    std::unordered_map<uint32_t, std::string> m_bindingToBlockName;
    std::unordered_map<std::string, TextureInfo> m_textures;
};

} // namespace ocf
