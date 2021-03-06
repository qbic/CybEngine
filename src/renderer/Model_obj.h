#pragma once
#include "Base/Math/Vector.h"
#include "renderer/RenderDevice.h"

namespace renderer
{

typedef uint32_t OBJ_Index;

struct OBJ_Edge
{
    OBJ_Edge(const OBJ_Index inVertexIndex, const OBJ_Index inTexCoordIndex) :
        vertexIndex(inVertexIndex),
        texCoordIndex(inTexCoordIndex)
    {
    }

    OBJ_Index vertexIndex;
    OBJ_Index texCoordIndex;
};

struct OBJ_Face
{
    std::vector<OBJ_Edge> edges;
    Vec3f normal;
};

struct OBJ_FaceGroup
{
    OBJ_FaceGroup(const std::string &faceGroupName) :
        name(faceGroupName)
    {
    }

    std::string name;
    std::string materialName;
    std::vector<OBJ_Face> faces;
};

struct OBJ_Material
{
    std::string name;
    Vec3f ambientColor;
    Vec3f diffuseColor;
    Vec3f specularColor;
    std::string ambientTexture;
    std::string diffuseTexture;
    std::string specularTexture;
    std::string bumpTexture;
    float dissolve;
    float shininess;
};

typedef std::unordered_map<std::string, OBJ_Material> OBJ_MaterialMap;

struct OBJ_PosNormalTangentVertex
{
    OBJ_PosNormalTangentVertex(const Vec3f &inPos,
                               const Vec3f &inNormal,
                               const Vec3f &inTangent) :
        pos(inPos),
        normal(inNormal),
        tangent(inTangent)
    {
    }

    Vec3f pos;
    Vec3f normal;
    Vec3f tangent;
};

struct OBJ_RawModel
{
    OBJ_RawModel(const std::string &inName) :
        name(inName)
    {
    }

    OBJ_FaceGroup *AddEmptyFaceGroup(const std::string &faceGroupName)
    {
        
        if (!faceGroups.empty() && faceGroups.back().faces.empty())
        {
            faceGroups.back().name = faceGroupName;
        } else
        {
            faceGroups.emplace_back(faceGroupName);
        }

        return &faceGroups.back();
    }

    std::string name;
    std::vector<OBJ_PosNormalTangentVertex> vertices;
    std::vector<glm::vec2> texCoords;       // texCoords are held oudside vertices sence a vertex can have more than one texCoord in an .obj
    std::vector<OBJ_FaceGroup> faceGroups;
    OBJ_MaterialMap materials;
};

struct OBJ_Vertex
{
    OBJ_Vertex() = default;
    OBJ_Vertex(const Vec3f &inPosition,
               const Vec3f &inNormal,
               const Vec3f &inTangent,
               const glm::vec2 &inTexCoord) :
        position(inPosition),
        normal(inNormal),
        tangent(inTangent),
        texCoord(inTexCoord)
    {
    }

    Vec3f position;
    Vec3f normal;
    Vec3f tangent;
    glm::vec2 texCoord;
};

struct OBJ_TriSurface
{
    OBJ_TriSurface(const std::string &inName, const OBJ_Material &inMaterial) :
        name(inName),
        material(inMaterial)
    {
    }

    std::string name;
    std::vector<OBJ_Vertex> vertices;
    std::vector<OBJ_Index> indices;
    OBJ_Material material;
};

struct OBJ_CompiledModel
{
    OBJ_CompiledModel(const std::string &inName) :
        name(inName)
    {
    }

    std::string name;
    std::vector<OBJ_TriSurface> surfaces;
};

std::shared_ptr<OBJ_RawModel> OBJ_LoadModel(const std::string &filename);
std::shared_ptr<OBJ_CompiledModel> OBJ_CompileRawModel(const std::shared_ptr<OBJ_RawModel> rawModel);

} // engine