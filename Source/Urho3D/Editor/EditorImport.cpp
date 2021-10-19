//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Basically c&p Tools/AssetImporter

#include "../Editor/EditorImport.h"

#include "../Core/Context.h"
#include "../Container/Str.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Resource/Image.h"
#include "../IO/Log.h"

#ifdef WIN32
#include <windows.h>
#endif

// #include "assimp/config.h"
// #include "assimp/cimport.h"
// #include "assimp/scene.h"
// #include "assimp/postprocess.h"
// #include "assimp/DefaultLogger.hpp"

// FBX transform chain
enum TransformationComp
{
    TransformationComp_Translation = 0,
    TransformationComp_RotationOffset,
    TransformationComp_RotationPivot,
    TransformationComp_PreRotation,
    TransformationComp_Rotation,
    TransformationComp_PostRotation,
    TransformationComp_RotationPivotInverse,

    TransformationComp_ScalingOffset,
    TransformationComp_ScalingPivot,
    TransformationComp_Scaling,

    // Not checking these
    // They are typically flushed out in the fbxconverter, but there
    // might be cases where they're not, hence, leaving them.
    #ifdef EXT_TRANSFORMATION_CHECK
    TransformationComp_ScalingPivotInverse,
    TransformationComp_GeometricTranslation,
    TransformationComp_GeometricRotation,
    TransformationComp_GeometricScaling,
    #endif

    TransformationComp_MAXIMUM
};

const char *transformSuffix[TransformationComp_MAXIMUM] =
{
    "Translation",          // TransformationComp_Translation = 0,
    "RotationOffset",       // TransformationComp_RotationOffset,
    "RotationPivot",        // TransformationComp_RotationPivot,
    "PreRotation",          // TransformationComp_PreRotation,
    "Rotation",             // TransformationComp_Rotation,
    "PostRotation",         // TransformationComp_PostRotation,
    "RotationPivotInverse", // TransformationComp_RotationPivotInverse,

    "ScalingOffset",        // TransformationComp_ScalingOffset,
    "ScalingPivot",         // TransformationComp_ScalingPivot,
    "Scaling",              // TransformationComp_Scaling,

    #ifdef EXT_TRANSFORMATION_CHECK
    "ScalingPivotInverse",  // TransformationComp_ScalingPivotInverse,
    "GeometricTranslation", // TransformationComp_GeometricTranslation,
    "GeometricRotation",    // TransformationComp_GeometricRotation,
    "GeometricScaling",     // TransformationComp_GeometricScaling,
    #endif
};

static const unsigned MAX_CHANNELS = 4;

using namespace Urho3D;

struct OutModel
{
    String outName_;
    aiNode* rootNode_{};
    HashSet<unsigned> meshIndices_;
    PODVector<aiMesh*> meshes_;
    PODVector<aiNode*> meshNodes_;
    PODVector<aiNode*> bones_;
    PODVector<aiNode*> pivotlessBones_;
    PODVector<aiAnimation*> animations_;
    PODVector<float> boneRadii_;
    PODVector<BoundingBox> boneHitboxes_;
    aiNode* rootBone_{};
    unsigned totalVertices_{};
    unsigned totalIndices_{};
};

namespace Urho3D
{

EditorImport::EditorImport(Context* context)
    : Object (context)
{
    String programDir = GetSubsystem<FileSystem>()->GetProgramDir();
    resourcePath_ = programDir;
}

EditorImport::~EditorImport()
{
    aiReleaseImport(scene_);
}

String EditorImport::FromAIString(const aiString& str)
{
    return String(str.data);
}

Vector2 EditorImport::ToVector2(const aiVector2D& vec)
{
    return Vector2(vec.x, vec.y);
}

Vector3 EditorImport::ToVector3(const aiVector3D& vec)
{
    return Vector3(vec.x, vec.y, vec.z);
}

Quaternion EditorImport::ToQuaternion(const aiQuaternion& quat)
{
    return Quaternion(quat.w, quat.x, quat.y, quat.z);
}

Matrix3x4 EditorImport::ToMatrix3x4(const aiMatrix4x4& mat)
{
    Matrix3x4 ret;
    memcpy(&ret.m00_, &mat.a1, sizeof(Matrix3x4));
    return ret;
}

aiMatrix4x4 EditorImport::ToAIMatrix4x4(const Matrix3x4& mat)
{
    aiMatrix4x4 ret;
    memcpy(&ret.a1, &mat.m00_, sizeof(Matrix3x4));
    return ret;
}

String EditorImport::SanitateAssetName(const String& name)
{
    String fixedName = name;
    fixedName.Replace("<", "");
    fixedName.Replace(">", "");
    fixedName.Replace("?", "");
    fixedName.Replace("*", "");
    fixedName.Replace(":", "");
    fixedName.Replace("\"", "");
    fixedName.Replace("/", "");
    fixedName.Replace("\\", "");
    fixedName.Replace("|", "");

    return fixedName;
}

Matrix3x4 EditorImport::GetOffsetMatrix(OutModel& model, const String& boneName)
{
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        aiMesh* mesh = model.meshes_[i];
        aiNode* node = model.meshNodes_[i];
        for (unsigned j = 0; j < mesh->mNumBones; ++j)
        {
            aiBone* bone = mesh->mBones[j];
            if (boneName == bone->mName.data)
            {
                aiMatrix4x4 offset = bone->mOffsetMatrix;
                aiMatrix4x4 nodeDerivedInverse = GetMeshBakingTransform(node, model.rootNode_);
                nodeDerivedInverse.Inverse();
                offset *= nodeDerivedInverse;
                return ToMatrix3x4(offset);
            }
        }
    }

    // Fallback for rigid skinning for which actual offset matrix information doesn't exist
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        aiMesh* mesh = model.meshes_[i];
        aiNode* node = model.meshNodes_[i];
        if (!mesh->HasBones() && boneName == node->mName.data)
        {
            aiMatrix4x4 nodeDerivedInverse = GetMeshBakingTransform(node, model.rootNode_);
            nodeDerivedInverse.Inverse();
            return ToMatrix3x4(nodeDerivedInverse);
        }
    }

    return Matrix3x4::IDENTITY;
}

void EditorImport::GetBlendData(OutModel& model, aiMesh* mesh, aiNode* meshNode, PODVector<unsigned>& boneMappings, Vector<PODVector<unsigned char> >&
        blendIndices, Vector<PODVector<float> >& blendWeights)
{
    blendIndices.Resize(mesh->mNumVertices);
    blendWeights.Resize(mesh->mNumVertices);
    boneMappings.Clear();

    // If model has more bones than can fit vertex shader parameters, write the per-geometry mappings
    if (model.bones_.Size() > maxBones_)
    {
        if (mesh->mNumBones > maxBones_)
        {
            URHO3D_LOGERROR(
                "Geometry (submesh) has over " + String(maxBones_) + " bone influences. Try splitting to more submeshes\n"
                "that each stay at " + String(maxBones_) + " bones or below."
            );
            return;
        }
        if (mesh->mNumBones > 0)
        {
            boneMappings.Resize(mesh->mNumBones);
            for (unsigned i = 0; i < mesh->mNumBones; ++i)
            {
                aiBone* bone = mesh->mBones[i];
                String boneName = FromAIString(bone->mName);
                unsigned globalIndex = GetBoneIndex(model, boneName);
                if (globalIndex == M_MAX_UNSIGNED)
                {
                    URHO3D_LOGERROR("Bone " + boneName + " not found");
                    return;
                }
                    
                boneMappings[i] = globalIndex;
                for (unsigned j = 0; j < bone->mNumWeights; ++j)
                {
                    unsigned vertex = bone->mWeights[j].mVertexId;
                    blendIndices[vertex].Push(i);
                    blendWeights[vertex].Push(bone->mWeights[j].mWeight);
                }
            }
        }
        else
        {
            // If mesh does not have skinning information, implement rigid skinning so that it stays compatible with AnimatedModel
            String boneName = FromAIString(meshNode->mName);
            unsigned globalIndex = GetBoneIndex(model, boneName);
            if (globalIndex == M_MAX_UNSIGNED)
                URHO3D_LOGERROR("Warning: bone " + boneName + " not found, skipping rigid skinning");
            else
            {
                boneMappings.Push(globalIndex);
                for (unsigned i = 0; i < mesh->mNumVertices; ++i)
                {
                    blendIndices[i].Push(0);
                    blendWeights[i].Push(1.0f);
                }
            }
        }
    }
    else
    {
        if (mesh->mNumBones > 0)
        {
            for (unsigned i = 0; i < mesh->mNumBones; ++i)
            {
                aiBone* bone = mesh->mBones[i];
                String boneName = FromAIString(bone->mName);
                unsigned globalIndex = GetBoneIndex(model, boneName);
                if (globalIndex == M_MAX_UNSIGNED)
                {
                    URHO3D_LOGERROR("Bone " + boneName + " not found");
                    return;
                }
                    
                for (unsigned j = 0; j < bone->mNumWeights; ++j)
                {
                    unsigned vertex = bone->mWeights[j].mVertexId;
                    blendIndices[vertex].Push(globalIndex);
                    blendWeights[vertex].Push(bone->mWeights[j].mWeight);
                }
            }
        }
        else
        {
            String boneName = FromAIString(meshNode->mName);
            unsigned globalIndex = GetBoneIndex(model, boneName);
            if (globalIndex == M_MAX_UNSIGNED)
                URHO3D_LOGERROR("Warning: bone " + boneName + " not found, skipping rigid skinning");
            else
            {
                for (unsigned i = 0; i < mesh->mNumVertices; ++i)
                {
                    blendIndices[i].Push(globalIndex);
                    blendWeights[i].Push(1.0f);
                }
            }
        }
    }

    // Normalize weights now if necessary, also remove too many influences
    for (unsigned i = 0; i < blendWeights.Size(); ++i)
    {
        if (blendWeights[i].Size() > 4)
        {
            URHO3D_LOGERROR("Warning: more than 4 bone influences in vertex " + String(i));

            while (blendWeights[i].Size() > 4)
            {
                unsigned lowestIndex = 0;
                float lowest = M_INFINITY;
                for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
                {
                    if (blendWeights[i][j] < lowest)
                    {
                        lowest = blendWeights[i][j];
                        lowestIndex = j;
                    }
                }
                blendWeights[i].Erase(lowestIndex);
                blendIndices[i].Erase(lowestIndex);
            }
        }

        float sum = 0.0f;
        for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
            sum += blendWeights[i][j];
        if (sum != 1.0f && sum != 0.0f)
        {
            for (unsigned j = 0; j < blendWeights[i].Size(); ++j)
                blendWeights[i][j] /= sum;
        }
    }
}

String EditorImport::GetMeshMaterialName(aiMesh* mesh)
{
    aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];
    aiString matNameStr;
    material->Get(AI_MATKEY_NAME, matNameStr);
    String matName = SanitateAssetName(FromAIString(matNameStr));
    if (matName.Trimmed().Empty())
        matName = GenerateMaterialName(material);

    return (useSubdirs_ ? "Materials/" : "") + matName + ".xml";
}

String EditorImport::GenerateMaterialName(aiMaterial* material)
{
    for (unsigned i = 0; i < scene_->mNumMaterials; ++i)
    {
        if (scene_->mMaterials[i] == material)
            return inputName_ + "_Material" + String(i);
    }

    // Should not go here
    return String::EMPTY;
}

String EditorImport::GetMaterialTextureName(const String& nameIn)
{
    // Detect assimp embedded texture
    if (nameIn.Length() && nameIn[0] == '*')
        return GenerateTextureName(ToInt(nameIn.Substring(1)));
    else
        return (useSubdirs_ ? "Data/Textures/" : "") + nameIn;
}

String EditorImport::GenerateTextureName(unsigned texIndex)
{
    if (texIndex < scene_->mNumTextures)
    {
        // If embedded texture contains encoded data, use the format hint for file extension. Else save RGBA8 data as PNG
        aiTexture* tex = scene_->mTextures[texIndex];
        if (!tex->mHeight)
            return (useSubdirs_ ? "Data/Textures/" : "") + inputName_ + "_Texture" + String(texIndex) + "." + tex->achFormatHint;
        else
            return (useSubdirs_ ? "Data/Textures/" : "") + inputName_ + "_Texture" + String(texIndex) + ".png";
    }

    // Should not go here
    return String::EMPTY;
}

aiMatrix4x4 EditorImport::GetDerivedTransform(aiNode* node, aiNode* rootNode, bool rootInclusive)
{
    return GetDerivedTransform(node->mTransformation, node, rootNode, rootInclusive);
}

aiMatrix4x4 EditorImport::GetDerivedTransform(aiMatrix4x4 transform, aiNode* node, aiNode* rootNode, bool rootInclusive)
{
    // If basenode is defined, go only up to it in the parent chain
    while (node && node != rootNode)
    {
        node = node->mParent;
        if (!rootInclusive && node == rootNode)
            break;
        if (node)
            transform = node->mTransformation * transform;
    }
    return transform;
}

aiMatrix4x4 EditorImport::GetMeshBakingTransform(aiNode* meshNode, aiNode* modelRootNode)
{
    if (meshNode == modelRootNode)
        return {};
    else
        return GetDerivedTransform(meshNode, modelRootNode);
}

void EditorImport::GetPosRotScale(const aiMatrix4x4& transform, Vector3& pos, Quaternion& rot, Vector3& scale)
{
    aiVector3D aiPos;
    aiQuaternion aiRot;
    aiVector3D aiScale;
    transform.Decompose(aiScale, aiRot, aiPos);
    pos = ToVector3(aiPos);
    rot = ToQuaternion(aiRot);
    scale = ToVector3(aiScale);
}

bool EditorImport::ImportModel(const String& inName)
{
    unsigned flags =
        aiProcess_ConvertToLeftHanded |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FixInfacingNormals |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords |
        aiProcess_FindInstances |
        aiProcess_OptimizeMeshes |
        aiProcess_CalcTangentSpace;

    aiPropertyStore *aiprops = aiCreatePropertyStore();
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, 1);       //default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_ALL_MATERIALS, 0);             //default = false;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_MATERIALS, 1);                 //default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_CAMERAS, 1);                   //default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_LIGHTS, 1);                    //default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, 1);                //default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_STRICT_MODE, 0);                    //default = false;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);                //**false, default = true;
        aiSetImportPropertyInteger(aiprops, AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, 1);//default = true;

    scene_ = aiImportFileExWithProperties(GetNativePath(inName).CString(), flags, nullptr, aiprops);

    rootNode_ = scene_->mRootNode;

    String name = Urho3D::GetFileName(inName);    
    String outFile(name + ".mdl");
    ExportModel(outFile, scene_->mFlags & AI_SCENE_FLAGS_INCOMPLETE);

    if (!noMaterials_)
    {
        HashSet<String> usedTextures;
        ExportMaterials(usedTextures);
        if (!noTextures_)
            CopyTextures(usedTextures, GetPath(inName));
    }

    aiReleasePropertyStore(aiprops);

    return true;
}

bool EditorImport::ExportModel(const String& outName, bool animationOnly)
{
    if (outName.Empty())
    {
        // ErrorExit("No output file defined");
        URHO3D_LOGERROR("No output file defined");
        return false;
    }

    OutModel model;
    model.rootNode_ = rootNode_;
    model.outName_ = outName;

    CollectMeshes(model, model.rootNode_);
    CollectBones(model, animationOnly);
    BuildBoneCollisionInfo(model);
    BuildAndSaveModel(model);
    if (!noAnimations_)
    {
        CollectAnimations(&model);
        BuildAndSaveAnimations(&model);

        // Save scene-global animations
        CollectAnimations();
        BuildAndSaveAnimations();
    }

    return true;
}

unsigned EditorImport::GetNumValidFaces(aiMesh* mesh)
{
    unsigned ret = 0;

    for (unsigned j = 0; j < mesh->mNumFaces; ++j)
    {
        if (mesh->mFaces[j].mNumIndices == 3)
            ++ret;
    }

    return ret;
}

PODVector<VertexElement> EditorImport::GetVertexElements(aiMesh* mesh, bool isSkinned)
{
    PODVector<VertexElement> ret;

    // Position must always be first and of type Vector3 for raycasts to work
    ret.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));

    if (mesh->HasNormals())
        ret.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));

    for (unsigned i = 0; i < mesh->GetNumColorChannels() && i < MAX_CHANNELS; ++i)
        ret.Push(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, i));

    /// \todo Assimp mesh structure can specify 3D UV-coords. How to determine the difference? For now always treated as 2D.
    for (unsigned i = 0; i < mesh->GetNumUVChannels() && i < MAX_CHANNELS; ++i)
        ret.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, i));

    if (mesh->HasTangentsAndBitangents())
        ret.Push(VertexElement(TYPE_VECTOR4, SEM_TANGENT));

    if (isSkinned)
    {
        ret.Push(VertexElement(TYPE_VECTOR4, SEM_BLENDWEIGHTS));
        ret.Push(VertexElement(TYPE_UBYTE4, SEM_BLENDINDICES));
    }

    return ret;
}

aiNode* EditorImport::GetNode(const String& name, aiNode* rootNode, bool caseSensitive)
{
    if (!rootNode)
        return nullptr;
    if (!name.Compare(rootNode->mName.data, caseSensitive))
        return rootNode;
    for (unsigned i = 0; i < rootNode->mNumChildren; ++i)
    {
        aiNode* found = GetNode(name, rootNode->mChildren[i], caseSensitive);
        if (found)
            return found;
    }
    return nullptr;
}

unsigned EditorImport::GetBoneIndex(OutModel& model, const String& boneName)
{
    for (unsigned i = 0; i < model.bones_.Size(); ++i)
    {
        if (boneName == model.bones_[i]->mName.data)
            return i;
    }
    return M_MAX_UNSIGNED;
}

void EditorImport::WriteShortIndices(unsigned short*& dest, aiMesh* mesh, unsigned index, unsigned offset)
{
    if (mesh->mFaces[index].mNumIndices == 3)
    {
        *dest++ = mesh->mFaces[index].mIndices[0] + offset;
        *dest++ = mesh->mFaces[index].mIndices[1] + offset;
        *dest++ = mesh->mFaces[index].mIndices[2] + offset;
    }
}

void EditorImport::WriteLargeIndices(unsigned*& dest, aiMesh* mesh, unsigned index, unsigned offset)
{
    if (mesh->mFaces[index].mNumIndices == 3)
    {
        *dest++ = mesh->mFaces[index].mIndices[0] + offset;
        *dest++ = mesh->mFaces[index].mIndices[1] + offset;
        *dest++ = mesh->mFaces[index].mIndices[2] + offset;
    }
}

void EditorImport::WriteVertex(float*& dest, aiMesh* mesh, unsigned index, bool isSkinned, BoundingBox& box,
    const Matrix3x4& vertexTransform, const Matrix3& normalTransform, Vector<PODVector<unsigned char> >& blendIndices,
    Vector<PODVector<float> >& blendWeights)
{
    Vector3 vertex = vertexTransform * ToVector3(mesh->mVertices[index]);
    box.Merge(vertex);
    *dest++ = vertex.x_;
    *dest++ = vertex.y_;
    *dest++ = vertex.z_;

    if (mesh->HasNormals())
    {
        Vector3 normal = normalTransform * ToVector3(mesh->mNormals[index]);
        *dest++ = normal.x_;
        *dest++ = normal.y_;
        *dest++ = normal.z_;
    }

    for (unsigned i = 0; i < mesh->GetNumColorChannels() && i < MAX_CHANNELS; ++i)
    {
        *((unsigned*)dest) = Color(mesh->mColors[i][index].r, mesh->mColors[i][index].g, mesh->mColors[i][index].b,
            mesh->mColors[i][index].a).ToUInt();
        ++dest;
    }

    for (unsigned i = 0; i < mesh->GetNumUVChannels() && i < MAX_CHANNELS; ++i)
    {
        Vector3 texCoord = ToVector3(mesh->mTextureCoords[i][index]);
        *dest++ = texCoord.x_;
        *dest++ = texCoord.y_;
    }

    if (mesh->HasTangentsAndBitangents())
    {
        Vector3 tangent = normalTransform * ToVector3(mesh->mTangents[index]);
        Vector3 normal = normalTransform * ToVector3(mesh->mNormals[index]);
        Vector3 bitangent = normalTransform * ToVector3(mesh->mBitangents[index]);
        // Check handedness
        float w = 1.0f;
        if ((tangent.CrossProduct(normal)).DotProduct(bitangent) < 0.5f)
            w = -1.0f;

        *dest++ = tangent.x_;
        *dest++ = tangent.y_;
        *dest++ = tangent.z_;
        *dest++ = w;
    }

    if (isSkinned)
    {
        for (unsigned i = 0; i < 4; ++i)
        {
            if (i < blendWeights[index].Size())
                *dest++ = blendWeights[index][i];
            else
                *dest++ = 0.0f;
        }

        auto* destBytes = (unsigned char*)dest;
        ++dest;
        for (unsigned i = 0; i < 4; ++i)
        {
            if (i < blendIndices[index].Size())
                *destBytes++ = blendIndices[index][i];
            else
                *destBytes++ = 0;
        }
    }
}

void EditorImport::CollectMeshes(OutModel& model, aiNode* node)
{
    for (unsigned i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene_->mMeshes[node->mMeshes[i]];
        for (unsigned j = 0; j < model.meshes_.Size(); ++j)
        {
            if (mesh == model.meshes_[j])
            {
                URHO3D_LOGERROR("Warning: same mesh found multiple times");
                break;
            }
        }

        model.meshIndices_.Insert(node->mMeshes[i]);
        model.meshes_.Push(mesh);
        model.meshNodes_.Push(node);
        model.totalVertices_ += mesh->mNumVertices;
        model.totalIndices_ += GetNumValidFaces(mesh) * 3;
    }

    for (unsigned i = 0; i < node->mNumChildren; ++i)
        CollectMeshes(model, node->mChildren[i]);
}

void EditorImport::CollectBones(OutModel& model, bool animationOnly)
{
    HashSet<aiNode*> necessary;
    HashSet<aiNode*> rootNodes;

    bool haveSkinnedMeshes = false;
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        if (model.meshes_[i]->HasBones())
        {
            haveSkinnedMeshes = true;
            break;
        }
    }

    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        aiMesh* mesh = model.meshes_[i];
        aiNode* meshNode = model.meshNodes_[i];
        aiNode* meshParentNode = meshNode->mParent;
        aiNode* rootNode = nullptr;

        for (unsigned j = 0; j < mesh->mNumBones; ++j)
        {
            aiBone* bone = mesh->mBones[j];
            String boneName(FromAIString(bone->mName));
            aiNode* boneNode = GetNode(boneName, scene_->mRootNode, true);
            // if (!boneNode)
            //     ErrorExit("Could not find scene node for bone " + boneName);
            necessary.Insert(boneNode);
            rootNode = boneNode;

            for (;;)
            {
                boneNode = boneNode->mParent;
                if (!boneNode || ((boneNode == meshNode || boneNode == meshParentNode) && !animationOnly))
                    break;
                rootNode = boneNode;
                necessary.Insert(boneNode);
            }

            if (rootNodes.Find(rootNode) == rootNodes.End())
                rootNodes.Insert(rootNode);
        }

        // When model is partially skinned, include the attachment nodes of the rigid meshes in the skeleton
        if (haveSkinnedMeshes && !mesh->mNumBones)
        {
            aiNode* boneNode = meshNode;
            necessary.Insert(boneNode);
            rootNode = boneNode;

            for (;;)
            {
                boneNode = boneNode->mParent;
                if (!boneNode || ((boneNode == meshNode || boneNode == meshParentNode) && !animationOnly))
                    break;
                rootNode = boneNode;
                necessary.Insert(boneNode);
            }

            if (rootNodes.Find(rootNode) == rootNodes.End())
                rootNodes.Insert(rootNode);
        }
    }


    // If we find multiple root nodes, try to remedy by going back in the parent chain and finding a common parent
    if (rootNodes.Size() > 1)
    {
        for (HashSet<aiNode*>::Iterator i = rootNodes.Begin(); i != rootNodes.End(); ++i)
        {
            aiNode* commonParent = (*i);

            while (commonParent)
            {
                unsigned found = 0;
                for (HashSet<aiNode*>::Iterator j = rootNodes.Begin(); j != rootNodes.End(); ++j)
                {
                    if (i == j)
                        continue;
                    aiNode* parent = *j;
                    while (parent)
                    {
                        if (parent == commonParent)
                        {
                            ++found;
                            break;
                        }
                        parent = parent->mParent;
                    }
                }

                if (found >= rootNodes.Size() - 1)
                {
                    URHO3D_LOGERROR("Multiple roots initially found, using new root node " + FromAIString(commonParent->mName));
                    rootNodes.Clear();
                    rootNodes.Insert(commonParent);
                    necessary.Insert(commonParent);
                    break;
                }

                commonParent = commonParent->mParent;
            }

            if (rootNodes.Size() == 1)
                break; // Succeeded
        }
        // if (rootNodes.Size() > 1)
        //     ErrorExit("Skeleton with multiple root nodes found, not supported");
    }

    if (rootNodes.Empty())
        return;

    model.rootBone_ = *rootNodes.Begin();

    // Move the model to bind pose now if requested
    // if (moveToBindPose_)
    // {
    //     URHO3D_LOGERROR("Moving bones to bind pose");
    //     MoveToBindPose(model, model.rootBone_);
    // }

    CollectBonesFinal(model.bones_, necessary, model.rootBone_);
    // Initialize the bone collision info
    model.boneRadii_.Resize(model.bones_.Size());
    model.boneHitboxes_.Resize(model.bones_.Size());
    for (unsigned i = 0; i < model.bones_.Size(); ++i)
    {
        model.boneRadii_[i] = 0.0f;
        model.boneHitboxes_[i] = BoundingBox(0.0f, 0.0f);
    }
}

void EditorImport::MoveToBindPose(OutModel& model, aiNode* current)
{
    String nodeName(FromAIString(current->mName));
    Matrix3x4 bindWorldTransform = GetOffsetMatrix(model, nodeName).Inverse();
    // Skip if we get an identity offset matrix (bone lookup failed)
    if (!bindWorldTransform.Equals(Matrix3x4::IDENTITY))
    {
        if (current->mParent && current != model.rootNode_)
        {
            aiMatrix4x4 parentWorldTransform = GetDerivedTransform(current->mParent, model.rootNode_, true);
            Matrix3x4 parentInverse = ToMatrix3x4(parentWorldTransform).Inverse();
            current->mTransformation = ToAIMatrix4x4(parentInverse * bindWorldTransform);
        }
        else
            current->mTransformation = ToAIMatrix4x4(bindWorldTransform);
    }

    for (unsigned i = 0; i < current->mNumChildren; ++i)
        MoveToBindPose(model, current->mChildren[i]);
}

void EditorImport::CollectBonesFinal(PODVector<aiNode*>& dest, const HashSet<aiNode*>& necessary, aiNode* node)
{
    bool includeBone = necessary.Find(node) != necessary.End();
    String boneName = FromAIString(node->mName);

    // Check include/exclude filters for non-skinned bones
    if (!includeBone && includeNonSkinningBones_)
    {
        // If no includes specified, include by default but check for excludes
        if (nonSkinningBoneIncludes_.Empty())
            includeBone = true;

        // Check against includes/excludes
        for (unsigned i = 0; i < nonSkinningBoneIncludes_.Size(); ++i)
        {
            if (boneName.Contains(nonSkinningBoneIncludes_[i], false))
            {
                includeBone = true;
                break;
            }
        }
        for (unsigned i = 0; i < nonSkinningBoneExcludes_.Size(); ++i)
        {
            if (boneName.Contains(nonSkinningBoneExcludes_[i], false))
            {
                includeBone = false;
                break;
            }
        }

        if (includeBone)
            URHO3D_LOGERROR("Including non-skinning bone " + boneName);
    }

    if (includeBone)
        dest.Push(node);

    for (unsigned i = 0; i < node->mNumChildren; ++i)
        CollectBonesFinal(dest, necessary, node->mChildren[i]);
}

void EditorImport::CollectAnimations(OutModel* model)
{
    const aiScene* scene = scene_;
    for (unsigned i = 0; i < scene->mNumAnimations; ++i)
    {
        aiAnimation* anim = scene->mAnimations[i];
        if (allAnimations_.Contains(anim))
            continue;

        if (model)
        {
            bool modelBoneFound = false;
            for (unsigned j = 0; j < anim->mNumChannels; ++j)
            {
                aiNodeAnim* channel = anim->mChannels[j];
                String channelName = FromAIString(channel->mNodeName);
                if (GetBoneIndex(*model, channelName) != M_MAX_UNSIGNED)
                {
                    modelBoneFound = true;
                    break;
                }
            }
            if (modelBoneFound)
            {
                model->animations_.Push(anim);
                allAnimations_.Insert(anim);
            }
        }
        else
        {
            sceneAnimations_.Push(anim);
            allAnimations_.Insert(anim);
        }
    }

    /// \todo Vertex morphs are ignored for now
}

void EditorImport::BuildBoneCollisionInfo(OutModel& model)
{
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        aiMesh* mesh = model.meshes_[i];
        for (unsigned j = 0; j < mesh->mNumBones; ++j)
        {
            aiBone* bone = mesh->mBones[j];
            String boneName = FromAIString(bone->mName);
            unsigned boneIndex = GetBoneIndex(model, boneName);
            if (boneIndex == M_MAX_UNSIGNED)
                continue;
            for (unsigned k = 0; k < bone->mNumWeights; ++k)
            {
                float weight = bone->mWeights[k].mWeight;
                // Require skinning weight to be sufficiently large before vertex contributes to bone hitbox
                if (weight > 0.33f)
                {
                    aiVector3D vertexBoneSpace = bone->mOffsetMatrix * mesh->mVertices[bone->mWeights[k].mVertexId];
                    Vector3 vertex = ToVector3(vertexBoneSpace);
                    float radius = vertex.Length();
                    if (radius > model.boneRadii_[boneIndex])
                        model.boneRadii_[boneIndex] = radius;
                    model.boneHitboxes_[boneIndex].Merge(vertex);
                }
            }
        }
    }
}

void EditorImport::BuildAndSaveModel(OutModel& model)
{
    if (!model.rootNode_)
    {
        URHO3D_LOGERROR("Null root node for model, skipping model save");
        return;
    }

    String rootNodeName = FromAIString(model.rootNode_->mName);
    if (!model.meshes_.Size())
    {
        URHO3D_LOGERROR("No geometries found starting from node " + rootNodeName + ", skipping model save");
        return;
    }

    URHO3D_LOGERROR("Writing model " + rootNodeName);

    SharedPtr<Model> outModel(new Model(context_));
    Vector<PODVector<unsigned> > allBoneMappings;
    BoundingBox box;

    unsigned numValidGeometries = 0;

    bool combineBuffers = true;
    // Check if buffers can be combined (same vertex elements, under 65535 vertices)
    PODVector<VertexElement> elements = GetVertexElements(model.meshes_[0], model.bones_.Size() > 0);
    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        if (GetNumValidFaces(model.meshes_[i]))
        {
            ++numValidGeometries;
            if (i > 0 && GetVertexElements(model.meshes_[i], model.bones_.Size() > 0) != elements)
                combineBuffers = false;
        }
    }

    // Check if keeping separate buffers allows to avoid 32-bit indices
    if (combineBuffers && model.totalVertices_ > 65535)
    {
        bool allUnder65k = true;
        for (unsigned i = 0; i < model.meshes_.Size(); ++i)
        {
            if (GetNumValidFaces(model.meshes_[i]))
            {
                if (model.meshes_[i]->mNumVertices > 65535)
                    allUnder65k = false;
            }
        }
        if (allUnder65k == true)
            combineBuffers = false;
    }

    SharedPtr<IndexBuffer> ib;
    SharedPtr<VertexBuffer> vb;
    Vector<SharedPtr<VertexBuffer> > vbVector;
    Vector<SharedPtr<IndexBuffer> > ibVector;
    unsigned startVertexOffset = 0;
    unsigned startIndexOffset = 0;
    unsigned destGeomIndex = 0;
    bool isSkinned = model.bones_.Size() > 0;

    outModel->SetNumGeometries(numValidGeometries);

    for (unsigned i = 0; i < model.meshes_.Size(); ++i)
    {
        aiMesh* mesh = model.meshes_[i];
        PODVector<VertexElement> elements = GetVertexElements(mesh, isSkinned);
        unsigned validFaces = GetNumValidFaces(mesh);
        if (!validFaces)
            continue;

        bool largeIndices;
        if (combineBuffers)
            largeIndices = model.totalIndices_ > 65535;
        else
            largeIndices = mesh->mNumVertices > 65535;

        // Create new buffers if necessary
        if (!combineBuffers || vbVector.Empty())
        {
            vb = new VertexBuffer(context_);
            vb->SetShadowed(true);
            ib = new IndexBuffer(context_);
            ib->SetShadowed(true);

            if (combineBuffers)
            {
                ib->SetSize(model.totalIndices_, largeIndices);
                vb->SetSize(model.totalVertices_, elements);
            }
            else
            {
                ib->SetSize(validFaces * 3, largeIndices);
                vb->SetSize(mesh->mNumVertices, elements);
            }

            vbVector.Push(vb);
            ibVector.Push(ib);
            startVertexOffset = 0;
            startIndexOffset = 0;
        }

        // Get the world transform of the mesh for baking into the vertices
        Matrix3x4 vertexTransform;
        Matrix3 normalTransform;
        Vector3 pos, scale;
        Quaternion rot;
        GetPosRotScale(GetMeshBakingTransform(model.meshNodes_[i], model.rootNode_), pos, rot, scale);
        vertexTransform = Matrix3x4(pos, rot, scale);
        normalTransform = rot.RotationMatrix();

        SharedPtr<Geometry> geom(new Geometry(context_));

        URHO3D_LOGERROR("Writing geometry " + String(i) + " with " + String(mesh->mNumVertices) + " vertices " +
            String(validFaces * 3) + " indices");

        if (model.bones_.Size() > 0 && !mesh->HasBones())
            URHO3D_LOGERROR("Warning: model has bones but geometry " + String(i) + " has no skinning information");

        unsigned char* vertexData = vb->GetShadowData();
        unsigned char* indexData = ib->GetShadowData();

        // Build the index data
        if (!largeIndices)
        {
            unsigned short* dest = (unsigned short*)indexData + startIndexOffset;
            for (unsigned j = 0; j < mesh->mNumFaces; ++j)
                WriteShortIndices(dest, mesh, j, startVertexOffset);
        }
        else
        {
            unsigned* dest = (unsigned*)indexData + startIndexOffset;
            for (unsigned j = 0; j < mesh->mNumFaces; ++j)
                WriteLargeIndices(dest, mesh, j, startVertexOffset);
        }

        // Build the vertex data
        // If there are bones, get blend data
        Vector<PODVector<unsigned char> > blendIndices;
        Vector<PODVector<float> > blendWeights;
        PODVector<unsigned> boneMappings;
        if (model.bones_.Size())
            GetBlendData(model, mesh, model.meshNodes_[i], boneMappings, blendIndices, blendWeights);

        auto* dest = (float*)((unsigned char*)vertexData + startVertexOffset * vb->GetVertexSize());
        for (unsigned j = 0; j < mesh->mNumVertices; ++j)
            WriteVertex(dest, mesh, j, isSkinned, box, vertexTransform, normalTransform, blendIndices, blendWeights);

        // Calculate the geometry center
        Vector3 center = Vector3::ZERO;
        if (validFaces)
        {
            for (unsigned j = 0; j < mesh->mNumFaces; ++j)
            {
                if (mesh->mFaces[j].mNumIndices == 3)
                {
                    center += vertexTransform * ToVector3(mesh->mVertices[mesh->mFaces[j].mIndices[0]]);
                    center += vertexTransform * ToVector3(mesh->mVertices[mesh->mFaces[j].mIndices[1]]);
                    center += vertexTransform * ToVector3(mesh->mVertices[mesh->mFaces[j].mIndices[2]]);
                }
            }

            center /= (float)validFaces * 3;
        }

        // Define the geometry
        geom->SetIndexBuffer(ib);
        geom->SetVertexBuffer(0, vb);
        geom->SetDrawRange(TRIANGLE_LIST, startIndexOffset, validFaces * 3, true);
        outModel->SetNumGeometryLodLevels(destGeomIndex, 1);
        outModel->SetGeometry(destGeomIndex, 0, geom);
        outModel->SetGeometryCenter(destGeomIndex, center);
        if (model.bones_.Size() > maxBones_)
            allBoneMappings.Push(boneMappings);

        startVertexOffset += mesh->mNumVertices;
        startIndexOffset += validFaces * 3;
        ++destGeomIndex;
    }

    // Define the model buffers and bounding box
    PODVector<unsigned> emptyMorphRange;
    outModel->SetVertexBuffers(vbVector, emptyMorphRange, emptyMorphRange);
    outModel->SetIndexBuffers(ibVector);
    outModel->SetBoundingBox(box);

    // Build skeleton if necessary
    if (model.bones_.Size() && model.rootBone_)
    {
        URHO3D_LOGERROR("Writing skeleton with " + String(model.bones_.Size()) + " bones, rootbone " +
            FromAIString(model.rootBone_->mName));

        Skeleton skeleton;
        Vector<Bone>& bones = skeleton.GetModifiableBones();

        for (unsigned i = 0; i < model.bones_.Size(); ++i)
        {
            aiNode* boneNode = model.bones_[i];
            String boneName(FromAIString(boneNode->mName));

            Bone newBone;
            newBone.name_ = boneName;

            aiMatrix4x4 transform = boneNode->mTransformation;
            // Make the root bone transform relative to the model's root node, if it is not already
            // (in case there are nodes between that are not accounted for otherwise)
            if (boneNode == model.rootBone_)
                transform = GetDerivedTransform(boneNode, model.rootNode_, false);

            GetPosRotScale(transform, newBone.initialPosition_, newBone.initialRotation_, newBone.initialScale_);

            // Get offset information if exists
            newBone.offsetMatrix_ = GetOffsetMatrix(model, boneName);
            newBone.radius_ = model.boneRadii_[i];
            newBone.boundingBox_ = model.boneHitboxes_[i];
            newBone.collisionMask_ = BONECOLLISION_SPHERE | BONECOLLISION_BOX;
            newBone.parentIndex_ = i;
            bones.Push(newBone);
        }
        // Set the bone hierarchy
        for (unsigned i = 1; i < model.bones_.Size(); ++i)
        {
            String parentName = FromAIString(model.bones_[i]->mParent->mName);
            for (unsigned j = 0; j < bones.Size(); ++j)
            {
                if (bones[j].name_ == parentName)
                {
                    bones[i].parentIndex_ = j;
                    break;
                }
            }
        }

        outModel->SetSkeleton(skeleton);
        if (model.bones_.Size() > maxBones_)
            outModel->SetGeometryBoneMappings(allBoneMappings);
    }

    File outFile(context_);
    String outFilename = resourcePath_ + (useSubdirs_ ? "Data/Models/" : "" ) + model.outName_;
    if (!outFile.Open(outFilename, FILE_WRITE))
    {
        URHO3D_LOGERROR("Could not open output file " + outFilename);
        return;
    }
        
    outModel->Save(outFile);

    // If exporting materials, also save material list for use by the editor
    if (!noMaterials_ && saveMaterialList_)
    {
        String materialListName = ReplaceExtension(outFilename, ".txt");
        File listFile(context_);
        if (listFile.Open(materialListName, FILE_WRITE))
        {
            for (unsigned i = 0; i < model.meshes_.Size(); ++i)
                listFile.WriteLine(GetMeshMaterialName(model.meshes_[i]));
        }
        else
            URHO3D_LOGERROR("Warning: could not write material list file " + materialListName);
    }
}

void EditorImport::ExportMaterials(HashSet<String>& usedTextures)
{
    if (useSubdirs_)
        context_->GetSubsystem<FileSystem>()->CreateDir(resourcePath_ + "Materials");

    for (unsigned i = 0; i < scene_->mNumMaterials; ++i)
        BuildAndSaveMaterial(scene_->mMaterials[i], usedTextures);
}

void EditorImport::BuildAndSaveMaterial(aiMaterial* material, HashSet<String>& usedTextures)
{
    aiString matNameStr;
    material->Get(AI_MATKEY_NAME, matNameStr);
    String matName = SanitateAssetName(FromAIString(matNameStr));
    if (matName.Trimmed().Empty())
        matName = GenerateMaterialName(material);

    // Do not actually create a material instance, but instead craft an xml file manually
    XMLFile outMaterial(context_);
    XMLElement materialElem = outMaterial.CreateRoot("material");

    String diffuseTexName;
    String normalTexName;
    String specularTexName;
    String lightmapTexName;
    String emissiveTexName;
    Color diffuseColor = Color::WHITE;
    Color specularColor;
    Color emissiveColor = Color::BLACK;
    bool hasAlpha = false;
    bool twoSided = false;
    float specPower = 1.0f;

    aiString stringVal;
    float floatVal;
    int intVal;
    aiColor3D colorVal;

    if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), stringVal) == AI_SUCCESS)
        diffuseTexName = GetFileNameAndExtension(FromAIString(stringVal));
    if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), stringVal) == AI_SUCCESS)
        normalTexName = GetFileNameAndExtension(FromAIString(stringVal));
    if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), stringVal) == AI_SUCCESS)
        specularTexName = GetFileNameAndExtension(FromAIString(stringVal));
    if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0), stringVal) == AI_SUCCESS)
        lightmapTexName = GetFileNameAndExtension(FromAIString(stringVal));
    if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), stringVal) == AI_SUCCESS)
        emissiveTexName = GetFileNameAndExtension(FromAIString(stringVal));
    if (!noMaterialDiffuseColor_)
    {
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, colorVal) == AI_SUCCESS)
            diffuseColor = Color(colorVal.r, colorVal.g, colorVal.b);
    }
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, colorVal) == AI_SUCCESS)
        specularColor = Color(colorVal.r, colorVal.g, colorVal.b);
    if (!emissiveAO_)
    {
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, colorVal) == AI_SUCCESS)
            emissiveColor = Color(colorVal.r, colorVal.g, colorVal.b);
    }
    if (material->Get(AI_MATKEY_OPACITY, floatVal) == AI_SUCCESS)
    {
        /// \hack New Assimp behavior - some materials may return 0 opacity, which is invisible.
        /// Revert to full opacity in that case
        if (floatVal < M_EPSILON)
            floatVal = 1.0f;

        if (floatVal < 1.0f)
            hasAlpha = true;
        diffuseColor.a_ = floatVal;
    }
    if (material->Get(AI_MATKEY_SHININESS, floatVal) == AI_SUCCESS)
        specPower = floatVal;
    if (material->Get(AI_MATKEY_TWOSIDED, intVal) == AI_SUCCESS)
        twoSided = (intVal != 0);

    String techniqueName = "Techniques/NoTexture";
    if (!diffuseTexName.Empty())
    {
        techniqueName = "Techniques/Diff";
        if (!normalTexName.Empty())
            techniqueName += "Normal";
        if (!specularTexName.Empty())
            techniqueName += "Spec";
        // For now lightmap does not coexist with normal & specular
        if (normalTexName.Empty() && specularTexName.Empty() && !lightmapTexName.Empty())
            techniqueName += "LightMap";
        if (lightmapTexName.Empty() && !emissiveTexName.Empty())
            techniqueName += emissiveAO_ ? "AO" : "Emissive";
    }
    if (hasAlpha)
        techniqueName += "Alpha";

    XMLElement techniqueElem = materialElem.CreateChild("technique");
    techniqueElem.SetString("name", techniqueName + ".xml");

    if (!diffuseTexName.Empty())
    {
        XMLElement diffuseElem = materialElem.CreateChild("texture");
        diffuseElem.SetString("unit", "diffuse");
        diffuseElem.SetString("name", GetMaterialTextureName(diffuseTexName));
        usedTextures.Insert(diffuseTexName);
    }
    if (!normalTexName.Empty())
    {
        XMLElement normalElem = materialElem.CreateChild("texture");
        normalElem.SetString("unit", "normal");
        normalElem.SetString("name", GetMaterialTextureName(normalTexName));
        usedTextures.Insert(normalTexName);
    }
    if (!specularTexName.Empty())
    {
        XMLElement specularElem = materialElem.CreateChild("texture");
        specularElem.SetString("unit", "specular");
        specularElem.SetString("name", GetMaterialTextureName(specularTexName));
        usedTextures.Insert(specularTexName);
    }
    if (!lightmapTexName.Empty())
    {
        XMLElement lightmapElem = materialElem.CreateChild("texture");
        lightmapElem.SetString("unit", "emissive");
        lightmapElem.SetString("name", GetMaterialTextureName(lightmapTexName));
        usedTextures.Insert(lightmapTexName);
    }
    if (!emissiveTexName.Empty())
    {
        XMLElement emissiveElem = materialElem.CreateChild("texture");
        emissiveElem.SetString("unit", "emissive");
        emissiveElem.SetString("name", GetMaterialTextureName(emissiveTexName));
        usedTextures.Insert(emissiveTexName);
    }

    XMLElement diffuseColorElem = materialElem.CreateChild("parameter");
    diffuseColorElem.SetString("name", "MatDiffColor");
    diffuseColorElem.SetColor("value", diffuseColor);
    XMLElement specularElem = materialElem.CreateChild("parameter");
    specularElem.SetString("name", "MatSpecColor");
    specularElem.SetVector4("value", Vector4(specularColor.r_, specularColor.g_, specularColor.b_, specPower));
    XMLElement emissiveColorElem = materialElem.CreateChild("parameter");
    emissiveColorElem.SetString("name", "MatEmissiveColor");
    emissiveColorElem.SetColor("value", emissiveColor);

    if (twoSided)
    {
        XMLElement cullElem = materialElem.CreateChild("cull");
        XMLElement shadowCullElem = materialElem.CreateChild("shadowcull");
        cullElem.SetString("value", "none");
        shadowCullElem.SetString("value", "none");
    }

    auto* fileSystem = context_->GetSubsystem<FileSystem>();

    String outFileName = resourcePath_ + (useSubdirs_ ? "Data/Materials/" : "" ) + matName + ".xml";
    if (noOverwriteMaterial_ && fileSystem->FileExists(outFileName))
    {
        URHO3D_LOGERROR("Skipping save of existing material " + matName);
        return;
    }

    URHO3D_LOGINFO("Writing material " + matName);

    File outFile(context_);
    if (!outFile.Open(outFileName, FILE_WRITE))
    {
        // ErrorExit("Could not open output file " + outFileName);
        URHO3D_LOGERROR("Could not open output file " + outFileName);
        return;
    }
        
    outMaterial.Save(outFile);
}

void EditorImport::CopyTextures(const HashSet<String>& usedTextures, const String& sourcePath)
{
    auto* fileSystem = context_->GetSubsystem<FileSystem>();

    if (useSubdirs_)
        fileSystem->CreateDir(resourcePath_ + "Textures");

    for (HashSet<String>::ConstIterator i = usedTextures.Begin(); i != usedTextures.End(); ++i)
    {
        // Handle assimp embedded textures
        if (i->Length() && i->At(0) == '*')
        {
            unsigned texIndex = ToInt(i->Substring(1));
            if (texIndex >= scene_->mNumTextures)
                URHO3D_LOGERROR("Skipping out of range texture index " + String(texIndex));
            else
            {
                aiTexture* tex = scene_->mTextures[texIndex];
                String fullDestName = resourcePath_ + GenerateTextureName(texIndex);
                bool destExists = fileSystem->FileExists(fullDestName);
                if (destExists && noOverwriteTexture_)
                {
                    URHO3D_LOGERROR("Skipping copy of existing embedded texture " + GetFileNameAndExtension(fullDestName));
                    continue;
                }
                // Encoded texture
                if (!tex->mHeight)
                {
                    URHO3D_LOGERROR("Saving embedded texture " + GetFileNameAndExtension(fullDestName));
                    File dest(context_, fullDestName, FILE_WRITE);
                    dest.Write((const void*)tex->pcData, tex->mWidth);
                }
                // RGBA8 texture
                else
                {
                    URHO3D_LOGERROR("Saving embedded RGBA texture " + GetFileNameAndExtension(fullDestName));
                    Image image(context_);
                    image.SetSize(tex->mWidth, tex->mHeight, 4);
                    memcpy(image.GetData(), (const void*)tex->pcData, (size_t)tex->mWidth * tex->mHeight * 4);
                    image.SavePNG(fullDestName);
                }
            }
        }
        else
        {
            String fullSourceName = sourcePath + *i;
            String fullDestName = resourcePath_ + (useSubdirs_ ? "Data/Textures/" : "") + *i;

            if (!fileSystem->FileExists(fullSourceName))
            {
                URHO3D_LOGERROR("Skipping copy of nonexisting material texture " + *i);
                continue;
            }
            {
                File test(context_, fullSourceName);
                if (!test.GetSize())
                {
                    URHO3D_LOGERROR("Skipping copy of zero-size material texture " + *i);
                    continue;
                }
            }

            bool destExists = fileSystem->FileExists(fullDestName);
            if (destExists && noOverwriteTexture_)
            {
                URHO3D_LOGERROR("Skipping copy of existing texture " + *i);
                continue;
            }
            if (destExists && noOverwriteNewerTexture_ && fileSystem->GetLastModifiedTime(fullDestName) >
                fileSystem->GetLastModifiedTime(fullSourceName))
            {
                URHO3D_LOGERROR("Skipping copying of material texture " + *i + ", destination is newer");
                continue;
            }

            URHO3D_LOGERROR("Copying material texture " + *i);
            fileSystem->Copy(fullSourceName, fullDestName);
        }
    }
}

void EditorImport::BuildAndSaveAnimations(OutModel* model)
{
    // extrapolate anim
    ExtrapolatePivotlessAnimation(model);

    // build and save anim
    const PODVector<aiAnimation*>& animations = model ? model->animations_ : sceneAnimations_;

    for (unsigned i = 0; i < animations.Size(); ++i)
    {
        aiAnimation* anim = animations[i];

        auto duration = (float)anim->mDuration;
        String animName = FromAIString(anim->mName);
        String animOutName;

        float thisImportEndTime = importEndTime_;
        float thisImportStartTime = importStartTime_;

        // If no animation split specified, set the end time to duration
        if (thisImportEndTime == 0.0f)
            thisImportEndTime = duration;

        if (animName.Empty())
            animName = "Anim" + String(i + 1);
        if (model)
            animOutName = GetPath(model->outName_) + GetFileName(model->outName_) + "_" + SanitateAssetName(animName) + ".ani";
        else
            animOutName = outPath_ + GetFileName(outName_) + "_" + SanitateAssetName(animName) + ".ani";

        auto ticksPerSecond = (float)anim->mTicksPerSecond;
        // If ticks per second not specified, it's probably a .X file. In this case use the default tick rate
        if (ticksPerSecond < M_EPSILON)
            ticksPerSecond = defaultTicksPerSecond_;
        float tickConversion = 1.0f / ticksPerSecond;

        // Find out the start time of animation from each channel's first keyframe for adjusting the keyframe times
        // to start from zero
        float startTime = duration;
        for (unsigned j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* channel = anim->mChannels[j];
            if (channel->mNumPositionKeys > 0)
                startTime = Min(startTime, (float)channel->mPositionKeys[0].mTime);
            if (channel->mNumRotationKeys > 0)
                startTime = Min(startTime, (float)channel->mRotationKeys[0].mTime);
            if (channel->mNumScalingKeys > 0)
                startTime = Min(startTime, (float)channel->mScalingKeys[0].mTime);
        }
        if (startTime > thisImportStartTime)
            thisImportStartTime = startTime;
        duration = thisImportEndTime - thisImportStartTime;

        SharedPtr<Animation> outAnim(new Animation(context_));
        outAnim->SetAnimationName(animName);
        outAnim->SetLength(duration * tickConversion);

        URHO3D_LOGINFO("Writing animation " + animName + " length " + String(outAnim->GetLength()));
        for (unsigned j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* channel = anim->mChannels[j];
            String channelName = FromAIString(channel->mNodeName);
            aiNode* boneNode = nullptr;

            if (model)
            {
                unsigned boneIndex;
                unsigned pos = channelName.Find("_$AssimpFbx$");

                if (!suppressFbxPivotNodes_ || pos == String::NPOS)
                {
                    boneIndex = GetBoneIndex(*model, channelName);
                    if (boneIndex == M_MAX_UNSIGNED)
                    {
                        URHO3D_LOGINFO("Warning: skipping animation track " + channelName + " not found in model skeleton");
                        outAnim->RemoveTrack(channelName);
                        continue;
                    }
                    boneNode = model->bones_[boneIndex];
                }
                else
                {
                    channelName = channelName.Substring(0, pos);

                    // every first $fbx animation channel for a bone will consolidate other $fbx animation to a single channel
                    // skip subsequent $fbx animation channel for the same bone
                    if (outAnim->GetTrack(channelName) != nullptr)
                        continue;

                    boneIndex = GetPivotlessBoneIndex(*model, channelName);
                    if (boneIndex == M_MAX_UNSIGNED)
                    {
                        URHO3D_LOGINFO("Warning: skipping animation track " + channelName + " not found in model skeleton");
                        outAnim->RemoveTrack(channelName);
                        continue;
                    }

                    boneNode = model->pivotlessBones_[boneIndex];
                }
            }
            else
            {
                boneNode = GetNode(channelName, scene_->mRootNode);
                if (!boneNode)
                {
                    URHO3D_LOGINFO("Warning: skipping animation track " + channelName + " whose scene node was not found");
                    outAnim->RemoveTrack(channelName);
                    continue;
                }
            }

            // To export single frame animation, check if first key frame is identical to bone transformation
            aiVector3D bonePos, boneScale;
            aiQuaternion boneRot;
            boneNode->mTransformation.Decompose(boneScale, boneRot, bonePos);

            bool posEqual = true;
            bool scaleEqual = true;
            bool rotEqual = true;

            if (channel->mNumPositionKeys > 0 && !ToVector3(bonePos).Equals(ToVector3(channel->mPositionKeys[0].mValue)))
                posEqual = false;
            if (channel->mNumScalingKeys > 0 && !ToVector3(boneScale).Equals(ToVector3(channel->mScalingKeys[0].mValue)))
                scaleEqual = false;
            if (channel->mNumRotationKeys > 0 && !ToQuaternion(boneRot).Equals(ToQuaternion(channel->mRotationKeys[0].mValue)))
                rotEqual = false;

            AnimationTrack* track = outAnim->CreateTrack(channelName);

            // Check which channels are used
            track->channelMask_ = CHANNEL_NONE;
            if (channel->mNumPositionKeys > 1 || !posEqual)
                track->channelMask_ |= CHANNEL_POSITION;
            if (channel->mNumRotationKeys > 1 || !rotEqual)
                track->channelMask_ |= CHANNEL_ROTATION;
            if (channel->mNumScalingKeys > 1 || !scaleEqual)
                track->channelMask_ |= CHANNEL_SCALE;
            // Check for redundant identity scale in all keyframes and remove in that case
            if (track->channelMask_ & CHANNEL_SCALE)
            {
                bool redundantScale = true;
                for (unsigned k = 0; k < channel->mNumScalingKeys; ++k)
                {
                    float SCALE_EPSILON = 0.000001f;
                    Vector3 scaleVec = ToVector3(channel->mScalingKeys[k].mValue);
                    if (fabsf(scaleVec.x_ - 1.0f) >= SCALE_EPSILON || fabsf(scaleVec.y_ - 1.0f) >= SCALE_EPSILON ||
                        fabsf(scaleVec.z_ - 1.0f) >= SCALE_EPSILON)
                    {
                        redundantScale = false;
                        break;
                    }
                }
                if (redundantScale)
                    track->channelMask_ &= ~CHANNEL_SCALE;
            }

            if (!track->channelMask_)
            {
                URHO3D_LOGINFO("Warning: skipping animation track " + channelName + " with no keyframes");
                outAnim->RemoveTrack(channelName);
                continue;
            }

            // Currently only same amount of keyframes is supported
            // Note: should also check the times of individual keyframes for match
            if ((channel->mNumPositionKeys > 1 && channel->mNumRotationKeys > 1 && channel->mNumPositionKeys != channel->mNumRotationKeys) ||
                (channel->mNumPositionKeys > 1 && channel->mNumScalingKeys > 1 && channel->mNumPositionKeys != channel->mNumScalingKeys) ||
                (channel->mNumRotationKeys > 1 && channel->mNumScalingKeys > 1 && channel->mNumRotationKeys != channel->mNumScalingKeys))
            {
                URHO3D_LOGINFO("Warning: differing amounts of channel keyframes, skipping animation track " + channelName);
                outAnim->RemoveTrack(channelName);
                continue;
            }

            unsigned keyFrames = channel->mNumPositionKeys;
            if (channel->mNumRotationKeys > keyFrames)
                keyFrames = channel->mNumRotationKeys;
            if (channel->mNumScalingKeys > keyFrames)
                keyFrames = channel->mNumScalingKeys;

            for (unsigned k = 0; k < keyFrames; ++k)
            {
                AnimationKeyFrame kf;
                kf.time_ = 0.0f;
                kf.position_ = Vector3::ZERO;
                kf.rotation_ = Quaternion::IDENTITY;
                kf.scale_ = Vector3::ONE;

                // Get time for the keyframe. Adjust with animation's start time
                if (track->channelMask_ & CHANNEL_POSITION && k < channel->mNumPositionKeys)
                    kf.time_ = ((float)channel->mPositionKeys[k].mTime - startTime);
                else if (track->channelMask_ & CHANNEL_ROTATION && k < channel->mNumRotationKeys)
                    kf.time_ = ((float)channel->mRotationKeys[k].mTime - startTime);
                else if (track->channelMask_ & CHANNEL_SCALE && k < channel->mNumScalingKeys)
                    kf.time_ = ((float)channel->mScalingKeys[k].mTime - startTime);

                // Make sure time stays positive
                kf.time_ = Max(kf.time_, 0.0f);

                // Start with the bone's base transform
                aiMatrix4x4 boneTransform = boneNode->mTransformation;
                aiVector3D pos, scale;
                aiQuaternion rot;
                boneTransform.Decompose(scale, rot, pos);
                // Then apply the active channels
                if (track->channelMask_ & CHANNEL_POSITION && k < channel->mNumPositionKeys)
                    pos = channel->mPositionKeys[k].mValue;
                if (track->channelMask_ & CHANNEL_ROTATION && k < channel->mNumRotationKeys)
                    rot = channel->mRotationKeys[k].mValue;
                if (track->channelMask_ & CHANNEL_SCALE && k < channel->mNumScalingKeys)
                    scale = channel->mScalingKeys[k].mValue;

                // If root bone, transform with nodes in between model root node (if any)
                if (model && boneNode == model->rootBone_)
                {
                    aiMatrix4x4 transMat, scaleMat, rotMat;
                    aiMatrix4x4::Translation(pos, transMat);
                    aiMatrix4x4::Scaling(scale, scaleMat);
                    rotMat = aiMatrix4x4(rot.GetMatrix());
                    aiMatrix4x4 tform = transMat * rotMat * scaleMat;
                    aiMatrix4x4 tformOld = tform;
                    tform = GetDerivedTransform(tform, boneNode, model->rootNode_, false);
                    // Do not decompose if did not actually change
                    if (tform != tformOld)
                        tform.Decompose(scale, rot, pos);
                }

                if (track->channelMask_ & CHANNEL_POSITION)
                    kf.position_ = ToVector3(pos);
                if (track->channelMask_ & CHANNEL_ROTATION)
                    kf.rotation_ = ToQuaternion(rot);
                if (track->channelMask_ & CHANNEL_SCALE)
                    kf.scale_ = ToVector3(scale);
                if (kf.time_ >= thisImportStartTime && kf.time_ <= thisImportEndTime)
                {
                    kf.time_ = (kf.time_ - thisImportStartTime) * tickConversion;
                    track->keyFrames_.Push(kf);
                }
            }
        }

        File outFile(context_);
        if (!outFile.Open(animOutName, FILE_WRITE))
        {
            // ErrorExit("Could not open output file " + animOutName);
            URHO3D_LOGERROR("Could not open output file " + animOutName);
            return;
        }
            
        outAnim->Save(outFile);
    }
}

void EditorImport::ExtrapolatePivotlessAnimation(OutModel* model)
{
    if (suppressFbxPivotNodes_ && model)
    {
        URHO3D_LOGINFO("Suppressing $fbx nodes");

        // Construct new bone structure from suppressed $fbx pivot nodes
        CreatePivotlessFbxBoneStruct(*model);

        // Extrapolate anim
        const PODVector<aiAnimation *> &animations = model->animations_;
        for (unsigned i = 0; i < animations.Size(); ++i)
        {
            aiAnimation* anim = animations[i];
            Vector<String> mainBoneCompleteList;
            mainBoneCompleteList.Clear();

            for (unsigned j = 0; j < anim->mNumChannels; ++j)
            {
                aiNodeAnim* channel = anim->mChannels[j];
                String channelName = FromAIString(channel->mNodeName);
                unsigned pos = channelName.Find("_$AssimpFbx$");

                if (pos != String::NPOS)
                {
                    // Every first $fbx animation channel for a bone will consolidate other $fbx animation to a single channel
                    // skip subsequent $fbx animation channel for the same bone
                    String mainBoneName = channelName.Substring(0, pos);

                    if (mainBoneCompleteList.Find(mainBoneName) != mainBoneCompleteList.End())
                        continue;

                    mainBoneCompleteList.Push(mainBoneName);
                    unsigned boneIdx = GetBoneIndex(*model, mainBoneName);

                    // This condition exists if a geometry, not a bone, has a key animation
                    if (boneIdx == M_MAX_UNSIGNED)
                        continue;

                    // Init chain indices and fill transforms
                    aiMatrix4x4 mainboneTransform = model->bones_[boneIdx]->mTransformation;
                    aiMatrix4x4 chain[TransformationComp_MAXIMUM];
                    int channelIndices[TransformationComp_MAXIMUM];

                    InitAnimatedChainTransformIndices(anim, j, mainBoneName, &channelIndices[0]);
                    std::fill_n(chain, static_cast<unsigned int>(TransformationComp_MAXIMUM), aiMatrix4x4());
                    FillChainTransforms(*model, &chain[0], mainBoneName);

                    unsigned keyFrames = channel->mNumPositionKeys;
                    if (channel->mNumRotationKeys > keyFrames)
                        keyFrames = channel->mNumRotationKeys;
                    if (channel->mNumScalingKeys  > keyFrames)
                        keyFrames = channel->mNumScalingKeys;

                    for (unsigned k = 0; k < keyFrames; ++k)
                    {
                        double frameTime = 0.0;
                        aiMatrix4x4 finalTransform;

                        // Chain transform animated values
                        for (unsigned l = 0; l < TransformationComp_MAXIMUM; ++l)
                        {
                            // It's either the chain transform or animation channel transform
                            if (channelIndices[l] != -1)
                            {
                                aiMatrix4x4 animtform, tempMat;
                                aiNodeAnim* animchannel = anim->mChannels[channelIndices[l]];

                                if (k < animchannel->mNumPositionKeys)
                                {
                                    aiMatrix4x4::Translation(animchannel->mPositionKeys[k].mValue, tempMat);
                                    animtform = animtform * tempMat;
                                    frameTime = Max(animchannel->mPositionKeys[k].mTime, frameTime);
                                }
                                if (k < animchannel->mNumRotationKeys)
                                {
                                    tempMat = aiMatrix4x4(animchannel->mRotationKeys[k].mValue.GetMatrix());
                                    animtform = animtform * tempMat;
                                    frameTime = Max(animchannel->mRotationKeys[k].mTime, frameTime);
                                }
                                if (k < animchannel->mNumScalingKeys)
                                {
                                    aiMatrix4x4::Scaling(animchannel->mScalingKeys[k].mValue, tempMat);
                                    animtform = animtform * tempMat;
                                    frameTime = Max(animchannel->mScalingKeys[k].mTime, frameTime);
                                }

                                finalTransform = finalTransform * animtform;
                            }
                            else
                                finalTransform = finalTransform * chain[l];
                        }

                        aiVector3D animPos, animScale;
                        aiQuaternion animRot;
                        finalTransform = finalTransform * mainboneTransform;
                        finalTransform.Decompose(animScale, animRot, animPos);

                        // New values
                        if (k < channel->mNumPositionKeys)
                        {
                            channel->mPositionKeys[k].mValue = animPos;
                            channel->mPositionKeys[k].mTime = frameTime;
                        }

                        if (k < channel->mNumRotationKeys)
                        {
                            channel->mRotationKeys[k].mValue = animRot;
                            channel->mRotationKeys[k].mTime = frameTime;
                        }

                        if (k < channel->mNumScalingKeys)
                        {
                            channel->mScalingKeys[k].mValue = animScale;
                            channel->mScalingKeys[k].mTime = frameTime;
                        }
                    }
                }
            }
        }
    }
}

unsigned EditorImport::GetPivotlessBoneIndex(OutModel& model, const String& boneName)
{
    for (unsigned i = 0; i < model.pivotlessBones_.Size(); ++i)
    {
        if (boneName == model.pivotlessBones_[i]->mName.data)
            return i;
    }
    return M_MAX_UNSIGNED;
}

void EditorImport::CreatePivotlessFbxBoneStruct(OutModel &model)
{
    // Init
    model.pivotlessBones_.Clear();
    aiMatrix4x4 chains[TransformationComp_MAXIMUM];

    for (unsigned i = 0; i < model.bones_.Size(); ++i)
    {
        String mainBoneName = String(model.bones_[i]->mName.data);

        // Skip $fbx nodes
        if (mainBoneName.Find("$AssimpFbx$") != String::NPOS)
            continue;

        std::fill_n(chains, static_cast<unsigned int>(TransformationComp_MAXIMUM), aiMatrix4x4());
        FillChainTransforms(model, &chains[0], mainBoneName);

        // Calculate chained transform
        aiMatrix4x4 finalTransform;
        for (const auto& chain : chains)
            finalTransform = finalTransform * chain;

        // New bone node
        auto*pnode = new aiNode;
        pnode->mName = model.bones_[i]->mName;
        pnode->mTransformation = finalTransform * model.bones_[i]->mTransformation;

        model.pivotlessBones_.Push(pnode);
    }
}

void EditorImport::FillChainTransforms(OutModel &model, aiMatrix4x4 *chain, const String& mainBoneName)
{
    for (unsigned j = 0; j < TransformationComp_MAXIMUM; ++j)
    {
        String transfBoneName = mainBoneName + "_$AssimpFbx$_" + String(transformSuffix[j]);

        for (unsigned k = 0; k < model.bones_.Size(); ++k)
        {
            String boneName = String(model.bones_[k]->mName.data);

            if (boneName == transfBoneName)
            {
                chain[j] = model.bones_[k]->mTransformation;
                break;
            }
        }
    }
}

void EditorImport::InitAnimatedChainTransformIndices(aiAnimation* anim, unsigned mainChannel, const String& mainBoneName, int *channelIndices)
{
    int numTransforms = 0;

    for (unsigned j = 0; j < TransformationComp_MAXIMUM; ++j)
    {
        String transfBoneName = mainBoneName + "_$AssimpFbx$_" + String(transformSuffix[j]);
        channelIndices[j] = -1;

        for (unsigned k = 0; k < anim->mNumChannels; ++k)
        {
            aiNodeAnim* channel = anim->mChannels[k];
            String channelName = FromAIString(channel->mNodeName);

            if (channelName == transfBoneName)
            {
                ++numTransforms;
                channelIndices[j] = k;
                break;
            }
        }
    }

    // resize animated channel key size
    if (numTransforms > 1)
        ExpandAnimatedChannelKeys(anim, mainChannel, channelIndices);
}

void EditorImport::ExpandAnimatedChannelKeys(aiAnimation* anim, unsigned mainChannel, const int *channelIndices)
{
    aiNodeAnim* channel = anim->mChannels[mainChannel];
    unsigned int poskeyFrames = channel->mNumPositionKeys;
    unsigned int rotkeyFrames = channel->mNumRotationKeys;
    unsigned int scalekeyFrames = channel->mNumScalingKeys;

    // Get max key frames
    for (unsigned i = 0; i < TransformationComp_MAXIMUM; ++i)
    {
        if (channelIndices[i] != -1 && channelIndices[i] != mainChannel)
        {
            aiNodeAnim* channel2 = anim->mChannels[channelIndices[i]];

            if (channel2->mNumPositionKeys > poskeyFrames)
                poskeyFrames = channel2->mNumPositionKeys;
            if (channel2->mNumRotationKeys > rotkeyFrames)
                rotkeyFrames = channel2->mNumRotationKeys;
            if (channel2->mNumScalingKeys  > scalekeyFrames)
                scalekeyFrames = channel2->mNumScalingKeys;
        }
    }

    // Resize and init vector key array
    if (poskeyFrames > channel->mNumPositionKeys)
    {
        auto* newKeys  = new aiVectorKey[poskeyFrames];
        for (unsigned i = 0; i < poskeyFrames; ++i)
        {
            if (i < channel->mNumPositionKeys )
                newKeys[i] = aiVectorKey(channel->mPositionKeys[i].mTime, channel->mPositionKeys[i].mValue);
            else
                newKeys[i].mValue = aiVector3D(0.0f, 0.0f, 0.0f);
        }
        delete[] channel->mPositionKeys;
        channel->mPositionKeys = newKeys;
        channel->mNumPositionKeys = poskeyFrames;
    }
    if (rotkeyFrames > channel->mNumRotationKeys)
    {
        auto* newKeys  = new aiQuatKey[rotkeyFrames];
        for (unsigned i = 0; i < rotkeyFrames; ++i)
        {
            if (i < channel->mNumRotationKeys)
                newKeys[i] = aiQuatKey(channel->mRotationKeys[i].mTime, channel->mRotationKeys[i].mValue);
            else
                newKeys[i].mValue = aiQuaternion();
        }
        delete[] channel->mRotationKeys;
        channel->mRotationKeys = newKeys;
        channel->mNumRotationKeys = rotkeyFrames;
    }
    if (scalekeyFrames > channel->mNumScalingKeys)
    {
        auto* newKeys  = new aiVectorKey[scalekeyFrames];
        for (unsigned i = 0; i < scalekeyFrames; ++i)
        {
            if ( i < channel->mNumScalingKeys)
                newKeys[i] = aiVectorKey(channel->mScalingKeys[i].mTime, channel->mScalingKeys[i].mValue);
            else
                newKeys[i].mValue = aiVector3D(1.0f, 1.0f, 1.0f);
        }
        delete[] channel->mScalingKeys;
        channel->mScalingKeys = newKeys;
        channel->mNumScalingKeys = scalekeyFrames;
    }
}


}