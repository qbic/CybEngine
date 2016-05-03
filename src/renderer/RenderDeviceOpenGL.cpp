﻿#include "Precompiled.h"
#include "RenderDevice.h"
#include "RenderDeviceOpenGL.h"
#include "Base/Debug.h"
#include "Base/Macros.h"

namespace renderer
{

static const OpenGLTextureFormatInfo pixelFormats[PixelFormat_Count] = 
{
//    internalFormat        format              type                compressed
    { GL_NONE,              GL_NONE,            GL_NONE,            GL_FALSE },     // PixelFormat_Unknown
    { GL_RGBA8,             GL_RGBA,            GL_UNSIGNED_BYTE,   GL_FALSE },     // PixelFormat_R8G8B8A8
    { GL_R8,                GL_RED,             GL_UNSIGNED_BYTE,   GL_FALSE },     // PixelFormat_R8
    { GL_RGBA32F,           GL_RGBA,            GL_FLOAT,           GL_FALSE },     // PixelFormat_R32G32B32A32F
    { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT,           GL_FALSE },     // PixelFormat_Depth24
};

static const OpenGLVertexElementUsageInfo vertexElementUsageInfo[VertexElementUsage_Count] =
{
//    attribLocation        attribName
    { 0,                    "Position"  },                                          // VertexElementUsage_Position
    { 1,                    "Normal"    },                                          // VertexElementUsage_Normal
    { 2,                    "TexCoord0" },                                          // VertexElementUsage_TexCoord0
    { 3,                    "TexCoord1" },                                          // VertexElementUsage_TexCoord1
    { 4,                    "TexCoord2" },                                          // VertexElementUsage_TexCoord2
    { 5,                    "TexCoord3" },                                          // VertexElementUsage_TexCoord3
    { 6,                    "Color"     }                                           // VertexElementUsage_Color
};

static const OpenGLVertexElementFormatInfo vertexElementTypeInfo[VertexElementFormat_Count] =
{
//    elementType           numComponents       alignedSíze         normalized
    { GL_FLOAT,             1,                  4,                  GL_FALSE },     // VertexElementFormat_Float1
    { GL_FLOAT,             2,                  8,                  GL_FALSE },     // VertexElementFormat_Float2
    { GL_FLOAT,             3,                  12,                 GL_FALSE },     // VertexElementFormat_Float3
    { GL_FLOAT,             4,                  16,                 GL_FALSE },     // VertexElementFormat_Float4
    { GL_UNSIGNED_BYTE,     4,                  4,                  GL_FALSE },     // VertexElementFormat_UByte4
    { GL_UNSIGNED_BYTE,     4,                  4,                  GL_TRUE  },     // VertexElementFormat_UByte4N
    { GL_UNSIGNED_SHORT,    2,                  4,                  GL_FALSE },     // VertexElementFormat_Short2
    { GL_UNSIGNED_SHORT,    4,                  8,                  GL_FALSE },     // VertexElementFormat_Short4
};

//
// OpenGL Buffer
//
OpenGLBuffer::~OpenGLBuffer()
{
    glDeleteBuffers(1, &resource);
}

void *OpenGLBuffer::Map()
{
    assert(usage == GL_DYNAMIC_DRAW);
    glBindBuffer(target, resource);
    return glMapBuffer(target, GL_WRITE_ONLY);
}

void OpenGLBuffer::Unmap()
{
    glBindBuffer(target, resource);
    glUnmapBuffer(target);
}

//
// OpenGL shader compiler
//
OpenGLShaderCompiler::~OpenGLShaderCompiler()
{
    FOR_EACH(compiledShaderStages, [&](auto &shader) { glDeleteShader(shader); });
}

bool OpenGLShaderCompiler::CompileShaderStage(GLenum stage, const ShaderBytecode &bytecode)
{
    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, (const GLchar **)&bytecode.source, (const GLint *)&bytecode.length);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar infoLog[KILOBYTES(1)];
        glGetShaderInfoLog(shader, sizeof(infoLog), 0, infoLog);
        DEBUG_LOG_TEXT_COND(infoLog[0], "Compiling shader:\n\n%s\nFailed: %s", bytecode.source, infoLog);
        return false;
    }

    compiledShaderStages.push_back(shader);
    return true;
}

bool OpenGLShaderCompiler::LinkAndClearShaderStages(GLuint &outProgram)
{
    GLuint program = glCreateProgram();
    FOR_EACH(compiledShaderStages, [&](const GLuint &shader) { glAttachShader(program, shader); });

    for (uint32_t i = 0; i < VertexElementUsage_Count; i++)
    {
        const OpenGLVertexElementUsageInfo *usageInfo = &vertexElementUsageInfo[i];
        glBindAttribLocation(program, usageInfo->attribLocation, usageInfo->attribName);
    }

    glLinkProgram(program);
    FOR_EACH(compiledShaderStages, [&](const GLuint &shader) { glDetachShader(program, shader); glDeleteShader(shader); });
    compiledShaderStages.clear();

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar infoLog[KILOBYTES(1)];
        glGetProgramInfoLog(program, sizeof(infoLog), 0, infoLog);
        DEBUG_LOG_TEXT_COND(infoLog[0], "Linking shaders failed: %s", infoLog);
        return false;
    }

    outProgram = program;
    return true;
}

//
// OpenGL Shader Program
//
OpenGLShaderProgram::~OpenGLShaderProgram()
{
    glDeleteProgram(resource);
}

int32_t OpenGLShaderProgram::GetParameterLocation(const char *name)
{
    return glGetUniformLocation(resource, name);
}

void OpenGLShaderProgram::SetFloatArray(int32_t location, size_t num, const float *values)
{
    glProgramUniform1fv(resource, location, (GLsizei)num, values);
}

void OpenGLShaderProgram::SetMat4(int32_t location, const float *values)
{
    glProgramUniformMatrix4fv(resource, location, 1, GL_FALSE, values);
}

//
// OpenGL Sampler State
//
OpenGLSamplerState::~OpenGLSamplerState()
{
    glDeleteSamplers(1, &resource);
}

//==============================
// OpenGL Image
//==============================

template <class BaseType>
OpenGLTextureBase<BaseType>::~OpenGLTextureBase()
{
    glDeleteTextures(1, &resource);
}

//==============================
// Render Device
//==============================

GLenum TranslateCullMode(RasterizerCullMode mode)
{
    switch (mode)
    {
    case CullMode_CW: return GL_FRONT;
    case CullMode_CCW: return GL_BACK;
    }

    return GL_NONE;
}

GLenum TranslateFillMode(RasterizerFillMode mode)
{
    switch (mode)
    {
    case FillMode_Point: return GL_POINT;
    case FillMode_Wireframe: return GL_LINE;
    case FillMode_Solid: return GL_FILL;
    }

    return GL_LINE;
}

GLint TranslateWrapMode(SamplerWrapMode mode)
{
    switch (mode)
    {
    case SamplerWrap_Repeat: return GL_REPEAT;
    case SamplerWrap_RepeatMirror: return GL_MIRRORED_REPEAT;
    case SamplerWrap_Clamp: return GL_CLAMP_TO_EDGE;
    }

    return GL_REPEAT;
}

void GLAPIENTRY DebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message, const void * /*userParam*/)
{
    static std::unordered_map<GLenum, const char *> toString = {
        { GL_DEBUG_SOURCE_API_ARB,                  "OpenGL"                },
        { GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB,        "Windows"               },
        { GL_DEBUG_SOURCE_SHADER_COMPILER_ARB,      "Shader compiler"       },
        { GL_DEBUG_SOURCE_THIRD_PARTY_ARB,          "Third party"           },
        { GL_DEBUG_SOURCE_APPLICATION_ARB,          "Application"           },
        { GL_DEBUG_SOURCE_OTHER_ARB,                "Other"                 },
        { GL_DEBUG_TYPE_ERROR_ARB,                  "Error"                 },
        { GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB,    "Deprecated behaviour"  },
        { GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB,     "Undefined behaviour"   },
        { GL_DEBUG_TYPE_PORTABILITY_ARB,            "Portability"           },
        { GL_DEBUG_TYPE_PERFORMANCE_ARB,            "Performence"           },
        { GL_DEBUG_TYPE_MARKER,                     "Marker"                },
        { GL_DEBUG_TYPE_PUSH_GROUP,                 "Push group"            },
        { GL_DEBUG_TYPE_POP_GROUP,                  "Pop group"             },
        { GL_DEBUG_TYPE_OTHER_ARB,                  "Other"                 },
        { GL_DEBUG_SEVERITY_HIGH_ARB,               "High"                  },
        { GL_DEBUG_SEVERITY_MEDIUM_ARB,             "Medium"                },
        { GL_DEBUG_SEVERITY_LOW_ARB,                "Low"                   },
        { GL_DEBUG_SEVERITY_NOTIFICATION,           "Notification"          }
    };

    DEBUG_LOG_TEXT("[driver] %s %s %#x %s: %s", toString[source], toString[type], id, toString[severity], message);
}

void OpenGLRenderDevice::Init()
{
    glewExperimental = true;
    const GLenum err = glewInit();
    THROW_FATAL_COND(err != GLEW_OK, std::string("glew init: ") + (char *)glewGetErrorString(err));

    // show driver and library versions
    DEBUG_LOG_TEXT("Using OpenGL version %s", glGetString(GL_VERSION));
    DEBUG_LOG_TEXT("Shader language %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    DEBUG_LOG_TEXT("GLEW %s", glewGetString(GLEW_VERSION));

    // show GPU memory on supported devices
    if (GL_NVX_gpu_memory_info)
    {
        GLint totalMemKb = 0;
        GLint avialableMemKb = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMemKb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &avialableMemKb);
        DEBUG_LOG_TEXT("Total / Available GPU memory: %dKb / %dKb", totalMemKb, avialableMemKb);
    }
   
    // enable opengl debug output
    glDebugMessageCallback(DebugOutputCallback, NULL);
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // initialize gl context state
    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    DEBUG_LOG_TEXT("Max texture units: %d", maxTextureUnits);

    glGenVertexArrays(1, &vaoId);

    // setup default states
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glFrontFace(GL_CCW);

   // glEnable(GL_MULTISAMPLE);

    // set default filter mode to anisotropic with max anisotropy
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&imageFilterMaxAnisotropy);
    DEBUG_LOG_TEXT("Max texture anisotropy: %d", imageFilterMaxAnisotropy);
    const SamplerStateInitializer initializer(SamplerFilter_Anisotropic, renderer::SamplerWrap_Repeat, renderer::SamplerWrap_Repeat, imageFilterMaxAnisotropy);
    const std::shared_ptr<ISamplerState> samplerState = CreateSamplerState(initializer);
    SetSamplerState(0, samplerState);
    SetSamplerState(1, samplerState);
    SetSamplerState(2, samplerState);
    SetSamplerState(3, samplerState);

    isInititialized = true;
}

void OpenGLRenderDevice::Shutdown()
{
    if (isInititialized)
    {
        glDeleteVertexArrays(1, &vaoId);
        isInititialized = false;
    }
}

std::shared_ptr<IBuffer> OpenGLRenderDevice::CreateBuffer(const void *data, size_t size, int usageFlags)
{
    GLenum target = GL_INVALID_ENUM;
    switch (usageFlags & Buffer_TypeMask)
    {
    case Buffer_Vertex: target = GL_ARRAY_BUFFER; break;
    case Buffer_Index:  target = GL_ELEMENT_ARRAY_BUFFER; break;
    }

    const GLenum usage = (usageFlags & Buffer_ReadOnly) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
    
    GLuint resource = 0;
    glCreateBuffers(1, &resource);
    glBindBuffer(target, resource);
    glBufferData(target, size, data, usage);

    return std::make_shared<OpenGLBuffer>(resource, target, usage, size);
}

std::shared_ptr<IVertexDeclaration> OpenGLRenderDevice::CreateVertexDelclaration(const VertexElementList &vertexElements)
{
    const auto searchResult = vertexDeclarationCache.find(vertexElements);
    if (searchResult != vertexDeclarationCache.end())
    {
        return searchResult->second;
    }

    OpenGLVertexElementList openGLVertexElements;
    for (const auto &element : vertexElements)
    {
        const OpenGLVertexElementUsageInfo *useInfo = &vertexElementUsageInfo[element.usage];
        const OpenGLVertexElementFormatInfo *typeInfo = &vertexElementTypeInfo[element.format];
        openGLVertexElements.emplace_back(
            useInfo->attribLocation,
            typeInfo->numComponents,
            typeInfo->elementType,
            typeInfo->normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLsizei>(element.stride),
            static_cast<GLuint>(element.alignedOffset));
    }

    auto vertexDeclaration =  std::make_shared<OpenGLVertexDeclaration>(openGLVertexElements);
    vertexDeclarationCache[vertexElements] = vertexDeclaration;
    return vertexDeclaration;
}

std::shared_ptr<IShaderProgram> OpenGLRenderDevice::CreateShaderProgram(const ShaderBytecode &VS, const ShaderBytecode &FS)
{
    OpenGLShaderCompiler compiler;
    compiler.CompileShaderStage(GL_VERTEX_SHADER, VS);
    compiler.CompileShaderStage(GL_FRAGMENT_SHADER, FS);

    GLuint programId = 0;
    compiler.LinkAndClearShaderStages(programId);

    return std::make_shared<OpenGLShaderProgram>(programId);
}

void OpenGLRenderDevice::SetShaderProgram(const std::shared_ptr<IShaderProgram> program)
{
    currentShaderProgram = std::static_pointer_cast<OpenGLShaderProgram>(program);
    glUseProgram(currentShaderProgram->resource);
}

std::shared_ptr<ITexture2D> OpenGLRenderDevice::CreateTexture2D(int32_t width, int32_t height, PixelFormat format, int32_t numMipMaps, const void *data)
{
    const OpenGLTextureFormatInfo *formatInfo = &pixelFormats[format];
    const GLenum target = GL_TEXTURE_2D;
    GLuint textureId = 0;
    
    glCreateTextures(target, 1, &textureId);
    glBindTexture(target, textureId);

    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, numMipMaps > 1 ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);

    glTexImage2D(target, 0, formatInfo->internalFormat, width, height, 0, formatInfo->format, formatInfo->type, data);
    glGenerateMipmap(target);

    auto texture = std::make_shared<OpenGLTexture2D>(
        textureId,
        target,
        width,
        height,
        CalculateNumMipLevels(width, height),
        format);
    return texture;
}

void OpenGLRenderDevice::SetTexture(uint32_t textureIndex, const std::shared_ptr<ITexture> texture)
{
    glActiveTexture(GL_TEXTURE0 + textureIndex);
    if (texture != nullptr)
    {
        std::shared_ptr<OpenGLTexture2D> glTexture = std::static_pointer_cast<OpenGLTexture2D>(texture);
        glBindTexture(glTexture->target, glTexture->resource);
    }
    else
    {
        // TODO: check context state for what target type is currently bound,
        // and use that to reset the GL drivers texture state.
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

std::shared_ptr<ISamplerState> OpenGLRenderDevice::CreateSamplerState(const SamplerStateInitializer &initializer)
{
    const auto searchResult = samplerStateCache.find(initializer);
    if (searchResult != samplerStateCache.end())
    {
        return searchResult->second;
    }

    auto samplerState = std::make_shared<OpenGLSamplerState>();

    samplerState->wrapS = TranslateWrapMode(initializer.wrapU);
    samplerState->wrapT = TranslateWrapMode(initializer.wrapV);
    samplerState->LODBias = initializer.mipBias;

    switch (initializer.filter)
    {
    case SamplerFilter_Anisotropic:
        samplerState->magFilter = GL_LINEAR;
        samplerState->minFilter = GL_LINEAR_MIPMAP_LINEAR;
        samplerState->maxAnisotropy = Clamp(initializer.maxAnisotropy, 1U, imageFilterMaxAnisotropy);
        break;
    case SamplerFilter_Trilinear:
        samplerState->magFilter = GL_LINEAR;
        samplerState->minFilter = GL_LINEAR_MIPMAP_LINEAR;
        break;
    case SamplerFilter_Bilinear:
        samplerState->magFilter = GL_LINEAR;
        samplerState->minFilter = GL_LINEAR_MIPMAP_NEAREST;
        break;
    case SamplerFilter_Point:
        samplerState->magFilter = GL_NEAREST;
        samplerState->minFilter = GL_NEAREST_MIPMAP_NEAREST;
        break;
    }

    glGenSamplers(1, &samplerState->resource);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_WRAP_S, samplerState->wrapS);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_WRAP_T, samplerState->wrapT);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_LOD_BIAS, samplerState->LODBias);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_MAG_FILTER, samplerState->magFilter);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_MIN_FILTER, samplerState->minFilter);
    glSamplerParameteri(samplerState->resource, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerState->maxAnisotropy);

    samplerStateCache[initializer] = samplerState;
    return samplerState;
}

void OpenGLRenderDevice::SetSamplerState(uint32_t textureIndex, const std::shared_ptr<ISamplerState> state)
{
    assert(state);
    const std::shared_ptr<OpenGLSamplerState> samplerState = std::static_pointer_cast<OpenGLSamplerState>(state);
    glActiveTexture(GL_TEXTURE0 + textureIndex);
    glBindSampler(textureIndex, samplerState->resource);
}

void OpenGLRenderDevice::Clear(uint32_t targets, const glm::vec4 color, float depth)
{
    const GLbitfield mask = (targets & Clear_Color ? GL_COLOR_BUFFER_BIT : 0) |
        (targets & Clear_Depth ? GL_DEPTH_BUFFER_BIT : 0) |
        (targets & Clear_Stencil ? GL_STENCIL_BUFFER_BIT : 0);

    glClearColor(color.r, color.g, color.b, color.a);
    glClearDepth(depth);
    glClear(mask);
}

static void SetRasterizerState(const RasterizerState &state)
{
    const RasterizerCullMode cullMode = state.cullMode;
    if (cullMode == CullMode_None)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        switch (cullMode)
        {
        case CullMode_CCW: glCullFace(GL_FRONT); break;
        case CullMode_CW: glCullFace(GL_BACK); break;
        }
    }

    switch (state.fillMode)
    {
    case FillMode_Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); glPointSize(state.pointSize); break;
    case FillMode_Wireframe: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glLineWidth(state.lineWidth); break;
    case FillMode_Solid: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
    }
}

void OpenGLRenderDevice::Render(const Surface *surf, const ICamera *camera)
{
    assert(surf);
    assert(camera);

    glBindVertexArray(vaoId);

    // setup matrices
    // TODO: add support for model matrices (this is kinda hacked together atm)
    int32_t projMatrixLoc = currentShaderProgram->GetParameterLocation("ProjMatrix");
    int32_t viewMatrixLoc = currentShaderProgram->GetParameterLocation("ModelViewMatrix");
    currentShaderProgram->SetMat4(projMatrixLoc, camera->GetProjMatrix());
    currentShaderProgram->SetMat4(viewMatrixLoc, camera->GetViewMatrix());

    SetRasterizerState(surf->rasterState);

    const SurfaceMaterial *material = &surf->material;
    if (material->texture[0])
        SetTexture(0, material->texture[0]);    // TODO: Texture state should be manually set by the user!

    // setup geometry
    const SurfaceGeometry *geo = &surf->geometry;
    const std::shared_ptr<OpenGLVertexDeclaration> vertexDeclaration = std::static_pointer_cast<OpenGLVertexDeclaration>(geo->vertexDeclaration);
    const std::shared_ptr<OpenGLBuffer> VBO = std::static_pointer_cast<OpenGLBuffer>(geo->VBO);
    const std::shared_ptr<OpenGLBuffer> IBO = std::static_pointer_cast<OpenGLBuffer>(geo->IBO);
    glBindBuffer(VBO->target, VBO->resource);
    glBindBuffer(IBO->target, IBO->resource);

    for (const auto &element : vertexDeclaration->vertexElements)
    {
        glEnableVertexAttribArray(element.attributeLocation);
        glVertexAttribPointer(element.attributeLocation, element.numComponents, element.type, element.normalized, element.stride, (const GLvoid *)element.offset);
    }

    // draw
    //BindVertexLayout(pipelineState->GetVertexLayout());
    glDrawElements(GL_TRIANGLES, geo->indexCount, GL_UNSIGNED_SHORT, NULL);
    //UnBindVertexLayout(pipelineState->GetVertexLayout());
}

std::shared_ptr<IRenderDevice> CreateRenderDevice()
{
    return std::make_shared<OpenGLRenderDevice>();
}

} // renderer