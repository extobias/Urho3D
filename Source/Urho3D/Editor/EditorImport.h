#pragma once

#include "../Core/Object.h"
#include "../Container/HashSet.h"
#include "../Math/BoundingBox.h"

#include "assimp/config.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

struct OutModel;
// class aiNode;
// class aiMesh;
// class aiString;
// class aiVector2D;
// class aiVector3D;

namespace Urho3D
{
class Node;
class Scene;
struct VertexElement;

class URHO3D_API EditorImport : public Object
{
    URHO3D_OBJECT(EditorImport, Object);

public:

    explicit EditorImport(Context* context);

    virtual ~EditorImport() override;

    unsigned GetNumValidFaces(aiMesh* mesh);

    bool ImportModel(const String& outName);

    bool ExportModel(const String& outName, bool animationOnly);

    aiNode* GetNode(const String& name, aiNode* rootNode, bool caseSensitive = true);

    unsigned GetBoneIndex(OutModel& model, const String& boneName);

    void WriteShortIndices(unsigned short*& dest, aiMesh* mesh, unsigned index, unsigned offset);

    void WriteLargeIndices(unsigned*& dest, aiMesh* mesh, unsigned index, unsigned offset);
    
    void WriteVertex(float*& dest, aiMesh* mesh, unsigned index, bool isSkinned, BoundingBox& box, 
        const Matrix3x4& vertexTransform, const Matrix3& normalTransform, Vector<PODVector<unsigned char> >& blendIndices,
        Vector<PODVector<float> >& blendWeights);

    PODVector<VertexElement> GetVertexElements(aiMesh* mesh, bool isSkinned);

    void CollectMeshes(OutModel& model, aiNode* node);

    void CollectBones(OutModel& model, bool animationOnly);

    void CollectAnimations(OutModel* model = nullptr);

    void MoveToBindPose(OutModel& model, aiNode* current);

    void CollectBonesFinal(PODVector<aiNode*>& dest, const HashSet<aiNode*>& necessary, aiNode* node);

    String FromAIString(const aiString& str);

    Vector2 ToVector2(const aiVector2D& vec);

    Vector3 ToVector3(const aiVector3D& vec);

    Quaternion ToQuaternion(const aiQuaternion& quat);

    Matrix3x4 ToMatrix3x4(const aiMatrix4x4& mat);

    aiMatrix4x4 ToAIMatrix4x4(const Matrix3x4& mat);

    String SanitateAssetName(const String& name);

    Matrix3x4 GetOffsetMatrix(OutModel& model, const String& boneName);

    void GetBlendData(OutModel& model, aiMesh* mesh, aiNode* meshNode, PODVector<unsigned>& boneMappings, Vector<PODVector<unsigned char> >&
        blendIndices, Vector<PODVector<float> >& blendWeights);

    String GetMeshMaterialName(aiMesh* mesh);

    String GenerateMaterialName(aiMaterial* material);

    String GetMaterialTextureName(const String& nameIn);

    String GenerateTextureName(unsigned texIndex);

    aiMatrix4x4 GetDerivedTransform(aiNode* node, aiNode* rootNode, bool rootInclusive = true);

    aiMatrix4x4 GetDerivedTransform(aiMatrix4x4 transform, aiNode* node, aiNode* rootNode, bool rootInclusive = true);

    aiMatrix4x4 GetMeshBakingTransform(aiNode* meshNode, aiNode* modelRootNode);

    void GetPosRotScale(const aiMatrix4x4& transform, Vector3& pos, Quaternion& rot, Vector3& scale);

    void BuildBoneCollisionInfo(OutModel& model);

    void BuildAndSaveModel(OutModel& model);

    void ExportMaterials(HashSet<String>& usedTextures);
    void BuildAndSaveMaterial(aiMaterial* material, HashSet<String>& usedTextures);
    void CopyTextures(const HashSet<String>& usedTextures, const String& sourcePath);

    void BuildAndSaveAnimations(OutModel* model = nullptr);
    void ExtrapolatePivotlessAnimation(OutModel* model);
    unsigned GetPivotlessBoneIndex(OutModel& model, const String& boneName);
    void CreatePivotlessFbxBoneStruct(OutModel &model);
    void FillChainTransforms(OutModel &model, aiMatrix4x4 *chain, const String& mainBoneName);
    void InitAnimatedChainTransformIndices(aiAnimation* anim, unsigned mainChannel, const String& mainBoneName, int *channelIndices);
    void ExpandAnimatedChannelKeys(aiAnimation* anim, unsigned mainChannel, const int *channelIndices);

private:

    const aiScene* scene_ { nullptr };
    aiNode* rootNode_ { nullptr };
    String inputName_;
    String resourcePath_;
    String modelSubdir_;
    String animSubdir_;
    String materialSubdir_;
    String textureSubDir_;
    String outPath_;
    String outName_;
    bool useSubdirs_ { true };
    bool localIDs_ { false };
    bool saveBinary_ { false };
    bool saveJson_ { false };
    bool createZone_ { true };
    bool noAnimations_ { false };
    bool noHierarchy_ { false };
    bool noMaterials_ { false };
    bool noTextures_ { false };
    bool noMaterialDiffuseColor_ { false };
    bool noEmptyNodes_ { false };
    bool saveMaterialList_ { false };
    bool includeNonSkinningBones_ { false };
    bool verboseLog_ { false };
    bool emissiveAO_ { false };
    bool noOverwriteMaterial_ { false };
    bool noOverwriteTexture_ { false };
    bool noOverwriteNewerTexture_ { false };
    bool checkUniqueModel_ { true };
    bool moveToBindPose_ { false };
    unsigned maxBones_ { 64 };
    Vector<String> nonSkinningBoneIncludes_;
    Vector<String> nonSkinningBoneExcludes_;

    HashSet<aiAnimation*> allAnimations_;
    PODVector<aiAnimation*> sceneAnimations_;

    float defaultTicksPerSecond_ { 4800.0f };
    // For subset animation import usage
    float importStartTime_ { 0.0f };
    float importEndTime_ { 0.0f };
    bool suppressFbxPivotNodes_ { true };
};

}