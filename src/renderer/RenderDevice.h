#pragma once
#include "Definitions.h"
#include "Base/Math/Vector.h"

namespace renderer
{

class IRenderDevice;

class IGPUResource
{
public:
    virtual ~IGPUResource() = default;
};

//
//  Buffer Interface
//
class IBuffer : public IGPUResource
{
public:
    virtual ~IBuffer() = default;
    virtual void *Map() = 0;
    virtual void Unmap() = 0;
};

//
// Vertex Element
//
// Usage example:
//  struct Vertex
//  {
//      glm::vec3 pos;
//      glm::vec3 normal;
//      glm::vec2 uv;
//  };
//
//  VertexElementList elements = {};
//  const size_t stride = sizeof(Vertex);
//  elements.emplace_back(VertexElementUsage_Position,  VertexElementFormat_Float3, offsetof(Vertex, pos));
//  elements.emplace_back(VertexElementUsage_Normal,    VertexElementFormat_Float3, offsetof(Vertex, normal));
//  elements.emplace_back(VertexElementUsage_TexCoord0, VertexElementFormat_Float2, offsetof(Vertex, uv));
//
struct VertexElement
{
    VertexElement() = default;
    VertexElement(VertexElementUsage inUsage,
                  VertexElementFormat inFormat,
                  size_t inAlignedOffset) :
        usage(inUsage),
        format(inFormat),
        alignedOffset(inAlignedOffset)
    {
    }

    bool operator==(const VertexElement &element) const;

    VertexElementUsage usage;
    VertexElementFormat format;
    size_t alignedOffset;
};

typedef std::vector<VertexElement> VertexElementList;

class IVertexDeclaration : public IGPUResource
{
public:
    virtual ~IVertexDeclaration() = default;
};

//
// Shader Program Interface
//
struct ShaderBytecode
{
    char *source;
    size_t length;
};

class IShaderProgram : public IGPUResource
{
public:
    ~IShaderProgram() = default;

    virtual int32_t GetParameterLocation(const char *name) = 0;

    virtual void SetBool(int32_t location, bool value) = 0;
    virtual void SetFloat(int32_t location, const float value) = 0;
    virtual void SetVec3(int32_t location, const float *values) = 0;
    virtual void SetMat3(int32_t location, const float *values) = 0;
    virtual void SetMat4(int32_t location, const float *values) = 0;
};

std::shared_ptr<IShaderProgram> CreateShaderProgramFromFiles(std::shared_ptr<IRenderDevice> device, const char *VSFilename, const char *FSFilename);
std::shared_ptr<IShaderProgram> CreateShaderProgramFromFiles(std::shared_ptr<IRenderDevice> device, const char *VSFilename, const char *GSFilename, const char *FSFilename);

//
// SamplerState Interface
//
struct SamplerStateInitializer
{
    SamplerStateInitializer() = default;
    SamplerStateInitializer(SamplerFilter inFilter,
                            SamplerWrapMode inWrapU = SamplerWrap_Repeat,
                            SamplerWrapMode inWrapV = SamplerWrap_Repeat,
                            SamplerWrapMode inWrapW = SamplerWrap_Repeat,
                            uint32_t inMaxAnisotropy = 16,
                            int32_t inMipBias = 0,
                            uint32_t inMinMipLevel = 0,
                            uint32_t inMaxMipLevel = UINT32_MAX) :
        filter(inFilter),
        wrapU(inWrapU),
        wrapV(inWrapV),
        wrapW(inWrapW),
        mipBias(inMipBias),
        minMipLevel(inMinMipLevel),
        maxMipLevel(inMaxMipLevel),
        maxAnisotropy(inMaxAnisotropy)
    {
    }

    bool operator==(const SamplerStateInitializer &initializer) const;

    SamplerFilter filter;
    SamplerWrapMode wrapU;
    SamplerWrapMode wrapV;
    SamplerWrapMode wrapW;
    int32_t mipBias;
    uint32_t minMipLevel;
    uint32_t maxMipLevel;
    uint32_t maxAnisotropy;
};

class ISamplerState : public IGPUResource {};

//
// Texture Interface
//
class ITexture : public IGPUResource
{
public:
    ITexture(uint32_t inNumMipMaps, PixelFormat inFormat) :
        numMipMaps(inNumMipMaps),
        format(inFormat)
    {
    }

    virtual ~ITexture() = default;
    uint32_t GetNumMipMaps() const { return numMipMaps; }
    PixelFormat GetFormat() const { return format; }

private:
    uint32_t numMipMaps;
    PixelFormat format;
};

class ITexture2D : public ITexture
{
public:
    ITexture2D(uint32_t inWidth, uint32_t inHeight, uint32_t inNumMipMaps, PixelFormat inFormat) :
        ITexture(inNumMipMaps, inFormat),
        width(inWidth),
        height(inHeight)
    {
    }

    uint32_t GetWidth()  const { return width; }
    uint32_t GetHeight() const { return height; }

private:
    uint32_t width;
    uint32_t height;
};

class ITextureCube : public ITexture
{
public:
    ITextureCube(uint32_t inWidth, uint32_t inHeight, uint32_t inNumMipMaps, PixelFormat inFormat) :
        ITexture(inNumMipMaps, inFormat),
        width(inWidth),
        height(inHeight)
    {
    }

    uint32_t GetWidth()  const { return width; }
    uint32_t GetHeight() const { return height; }

private:
    uint32_t width;
    uint32_t height;
};

//
// Surface structures
//

struct SurfaceMaterial
{
    enum { MaxNumTextures = 4 };

    std::shared_ptr<ISamplerState> sampler[MaxNumTextures];
    std::shared_ptr<ITexture> texture[MaxNumTextures];
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
    float shininess;
};

struct Surface
{
    uint32_t drawStateFlags;
    std::shared_ptr<IBuffer> vertexBuffer;
    std::shared_ptr<IVertexDeclaration> vertexDeclaration;
    std::shared_ptr<IBuffer> indexBuffer;
    uint32_t numVertices;
    uint32_t numIndices;
    uint32_t primitiveCount;
    SurfaceMaterial material;
    
    void Clear()
    {
        drawStateFlags = DrawState_Default;
        vertexBuffer = nullptr;
        vertexDeclaration = nullptr;
        indexBuffer = nullptr;
        numVertices = UINT32_MAX;
        numIndices = UINT32_MAX;
        primitiveCount = 0;
    }
};

//
// Camera Interface
//
class ICamera
{
public:
    virtual ~ICamera() = default;
    virtual const glm::vec3 &GetViewPositionVector() const = 0;
    virtual const glm::mat4 &GetViewMatrix() const = 0;
    virtual const glm::mat4 &GetProjMatrix() const = 0;
};

//
//  RenderDevice Interface
//
class IRenderDevice
{
public:
    virtual ~IRenderDevice() {}

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;

    virtual std::shared_ptr<IBuffer> CreateBuffer(int usageFlags, const void *data, size_t size) = 0;
    virtual std::shared_ptr<IVertexDeclaration> CreateVertexDelclaration(const VertexElementList &vertexElements, size_t stride) = 0;
    virtual std::shared_ptr<IShaderProgram> CreateShaderProgram(const ShaderBytecode &VS, const ShaderBytecode &FS) = 0;
    virtual std::shared_ptr<IShaderProgram> CreateShaderProgram(const ShaderBytecode &VS, const ShaderBytecode &GS, const ShaderBytecode &FS) = 0;
    virtual void SetShaderProgram(const std::shared_ptr<IShaderProgram> program) = 0;
    virtual std::shared_ptr<ITexture2D> CreateTexture2D(int32_t width, int32_t height, PixelFormat format, int32_t numMipMaps, const void *data) = 0;

    // data array order has to be: 0=right, 1=left, 2=top, 3=bottom, 4=back, 5=front
    virtual std::shared_ptr<ITextureCube> CreateTextureCube(int32_t width, int32_t height, PixelFormat format, const void *data[]) = 0;

    virtual void SetTexture(uint32_t textureIndex, const std::shared_ptr<ITexture> texture) = 0;
    virtual std::shared_ptr<ISamplerState> CreateSamplerState(const SamplerStateInitializer &initializer) = 0;
    virtual void SetSamplerState(uint32_t textureIndex, const std::shared_ptr<ISamplerState> state) = 0;

    virtual void Clear(uint32_t targets, const glm::vec4 color, float depth = 1.0f) = 0;
    virtual void Render(const Surface *surf, const ICamera *camera) = 0;
};

std::shared_ptr<IRenderDevice> CreateRenderDevice();

// Calculate the total number of mip levels used for an 2d texture
int CalculateNumMipLevels(uint32_t width, uint32_t height);

// SamplerStateInitializer hasher for compatibility with 
// std::unordered_map<> used by the SamplerState cache.
struct SamplerStateInitializerHasher
{
    size_t operator()(const SamplerStateInitializer &state) const;
};

struct VertexElementListHasher
{
    size_t operator()(const VertexElementList &vertexElements) const;
};

} // renderer