#include "stdafx.h"
#include "RenderDevice.h"

#include "core/FileUtils.h"
#include "core/Log.h"

namespace renderer
{

struct BuiltinShaderSet
{
    ShaderSet::Builtin builtin;
    BuiltinShaders vertShader;
    BuiltinShaders fragShader;
};

static const BuiltinShaderSet builtinShaders[] = {
    { ShaderSet::Builtin_Color,   VShader_MVP, FShader_LitGouraud },
    { ShaderSet::Builtin_Texture, VShader_MVP, FShader_LitGouraud }
};

SurfaceGeometry::SurfaceGeometry()
{
    format = VertexFormat_Invalid;
    indexCount = 0;
    rasterState = Raster_DrawSolid | Raster_CullBack | Raster_OrderCCW;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
}

uint32_t PackRGBA(float r, float g, float b, float a)
{
    uint32_t color = ((uint32_t)(r * 255) << 24) |
                     ((uint32_t)(g * 255) << 16) |
                     ((uint32_t)(b * 255) << 8) |
                      (uint32_t)(a * 255);
    return color;
}

void UnpackRGBA(uint32_t color, float &r, float &g, float &b, float &a)
{
    r = ((color >> 24) & 255) * (1.f / 255.f);
    g = ((color >> 16) & 255) * (1.f / 255.f);
    b = ((color >> 8) & 255) * (1.f / 255.f);
    a = (color & 255) * (1.f / 255.f);
}

bool ShaderSet::SetUniform1f(const char *name, float x)
{
    const float v[] = { x };
    return SetUniformfv(name, 1, v);
}

bool ShaderSet::SetUniform2f(const char *name, float x, float y)
{
    const float v[] = { x, y };
    return SetUniformfv(name, 2, v);
}

bool ShaderSet::SetUniform3f(const char *name, float x, float y, float z)
{
    const float v[] = { x, y, z };
    return SetUniformfv(name, 3, v);
}

bool ShaderSet::SetUniform4f(const char *name, float x, float y, float z, float w)
{
    const float v[] = { x, y, z, w };
    return SetUniformfv(name, 4, v);
}

bool ShaderSet::SetUniform4fv(const char *name, const float *v)
{
    return SetUniformfv(name, 4, v);
}

bool ShaderSet::SetUniform4x4f(const char *name, const glm::mat4 &m)
{
    return SetUniformfv(name, 16, glm::value_ptr(m));
}

void RenderDevice::SetDefaultShader(std::shared_ptr<ShaderSet> shader)
{
    if (!shader)
        return;

    defaultShader = shader;
}

std::shared_ptr<ShaderSet> RenderDevice::LoadBuiltinShaderSet(ShaderSet::Builtin shader)
{
    if (!builtinShaderSets[shader])
    {
        auto vertShader = LoadBuiltinShader(Shader::Vertex,   builtinShaders[shader].vertShader);
        auto fragShader = LoadBuiltinShader(Shader::Fragment, builtinShaders[shader].fragShader);
        builtinShaderSets[shader] = CreateShaderSet({ vertShader, fragShader });
    }

    return builtinShaderSets[shader];
}

void RenderDevice::SetProjection(const glm::mat4 &proj)
{
    projection = proj;
}

} // renderer