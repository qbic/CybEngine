#include "Precompiled.h"
#include "Renderer/RenderDevice.h"
#include "Base/Debug.h"
#include "Base/File.h"
#include "Base/MurmurHash.h"
#include "Base/Algorithm.h"

namespace renderer
{

bool VertexElement::operator==(const VertexElement &element) const
{
    return (usage == element.usage) &&
        (format == element.format) &&
        (alignedOffset == element.alignedOffset);
}

bool SamplerStateInitializer::operator==(const SamplerStateInitializer &initializer) const
{
    return memcmp(this, &initializer, sizeof(initializer)) == 0;
}

struct ShaderBytecodeFromFile : public ShaderBytecode
{
    ShaderBytecodeFromFile(const char *filename);
    ~ShaderBytecodeFromFile();
    bool IsValid() const { return length > 0; }
};

ShaderBytecodeFromFile::ShaderBytecodeFromFile(const char *filename)
{
    SysFile shaderFile(filename, FileOpen_Read);

    length = shaderFile.GetLength();
    if (length > 0)
    {
        source = new char[length + 1];
        shaderFile.Read((uint8_t *)source, length);
        source[length] = '\0';
    }
}

ShaderBytecodeFromFile::~ShaderBytecodeFromFile()
{
    if (length > 0)
    {
        delete[] source;
    }
}

std::shared_ptr<IShaderProgram> CreateShaderProgramFromFiles(std::shared_ptr<IRenderDevice> device, const char *VSFilename, const char *FSFilename)
{
    ShaderBytecodeFromFile VS(VSFilename);
    ShaderBytecodeFromFile FS(FSFilename);

    RETURN_NULL_IF(!VS.IsValid() || !FS.IsValid());
    return device->CreateShaderProgram(VS, FS);
}

std::shared_ptr<IShaderProgram> CreateShaderProgramFromFiles(std::shared_ptr<IRenderDevice> device, const char *VSFilename, const char *GSFilename, const char *FSFilename)
{
    ShaderBytecodeFromFile VS(VSFilename);
    ShaderBytecodeFromFile GS(GSFilename);
    ShaderBytecodeFromFile FS(FSFilename);

    RETURN_NULL_IF(!VS.IsValid() || !GS.IsValid() || !FS.IsValid());
    return device->CreateShaderProgram(VS, GS, FS);
}

int CalculateNumMipLevels(uint32_t width, uint32_t height)
{
    int n = 1;
    while (width > 1 || height > 1)
    {
        width >>= 1;
        height >>= 1;
        n++;
    }

    return n;
}

size_t SamplerStateInitializerHasher::operator()(const SamplerStateInitializer &state) const
{
    return CalculateMurmurHash(&state, sizeof(state));
}

size_t VertexElementListHasher::operator()(const VertexElementList &vertexElements) const
{
    return CalculateMurmurHash(&vertexElements[0], sizeof(VertexElement) * vertexElements.size());
}

} // renderer