#pragma once

#include "../Graphics/Drawable.h"

namespace Urho3D
{

class Drawable;
class Model;
class VertexBuffer;
class IndexBuffer;

enum VertexCollisionMask
{
    NONE = 0x00,
    TRACK_ROAD = 0x01,
    TRACK_BORDER = 0x02,
    OFFTRACK_WEAK = 0x04,
    OFFTRACK_HEAVY = 0x08
};
URHO3D_FLAGSET(VertexCollisionMask, VertexCollisionMaskFlags);

class URHO3D_API EditorModelDebug : public Drawable
{
    URHO3D_OBJECT(EditorModelDebug, Drawable);

public:
    /// Construct.
    explicit EditorModelDebug(Context* context);
    /// Destruct.
    ~EditorModelDebug() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results) override;

    void UpdateBatches(const FrameInfo& frame) override;

    void UpdateGeometry(const FrameInfo& frame) override;

    void ApplyAttributes() override;

    void OnSetAttribute(const AttributeInfo& attr, const Variant& src) override;

    void SetModel(Model* model);

    void SetMaterial(Material* material);

    void SetBoundingBox(const BoundingBox& box);

    void SelectVertex(const Frustum& frustum);

    void SelectFaces(const PODVector<IntVector2>& faces);

    void AddSelectedFaces(const PODVector<IntVector2>& faces);

    void DrawFaces(const PODVector<IntVector2>& faces);

    const PODVector<int>& GetSelectedVertex() const { return selectedIndex_; }

    const PODVector<IntVector2>& GetSelectedFaces() const { return selectedFaces_; }

    Model* GetModel() const { return model_; }

    void UpdateFacesIndexes();

    void ApplyVertexCollisionMask();

    void AddVertexElement(unsigned vertexIndex, unsigned vertexElementIndex);

    void SaveModel();

    void SelectAll();

    void SetModelAttr(const ResourceRef& value);

    ResourceRef GetModelAttr() const;

    VertexCollisionMaskFlags GetCurrentMask() { return GetFaceCollisionMask(currentFace_ * 3); }

    IntVector3 GetCurrentIndex() const { return currentIndex_; }

    unsigned GetCurrentFace() const { return currentFace_; }

    unsigned GetFacesCount() const;

protected:

    virtual void OnWorldBoundingBoxUpdate() override;

    PrimitiveType primitiveType_;

    SharedPtr<Model> model_;

    SharedPtr<Geometry> geometry_;

    SharedPtr<Geometry> geometryFaces_;

    SharedPtr<VertexBuffer> vertexBuffer_;

    SharedPtr<IndexBuffer> indexBuffer_;

    SharedPtr<VertexBuffer> vertexBufferFaces_;

    SharedPtr<IndexBuffer> indexBufferFaces_;

    PODVector<VertexElement> vertexElements_;

    Matrix3x4 transform_;

    PODVector<Matrix3x4> worldTransforms_;

    PODVector<Matrix3x4> worldTransformsFaces_;

    PODVector<Vector3> vertexOffset_;

    PODVector<int> selectedIndex_;

    PODVector<IntVector2> selectedFaces_;

    unsigned numWorldTransforms_{};

    unsigned numWorldTransformsFaces_{};

private:
    /// Handle model reload finished.
    void HandleModelReloadFinished(StringHash eventType, VariantMap& eventData);

    Color GetFaceColor();

    Color GetFaceColor(Urho3D::VertexCollisionMaskFlags mask);

    VertexCollisionMask GetCollisionMask(unsigned int mask);

    void CreateVertexInstances();

    void CreateFaceInstances();

    void UpdateFacesColor();

    void CalcMatrixFaces();

    Vector<Vector3> GetFacePoints(unsigned face);

    VertexCollisionMask GetFaceCollisionMask(unsigned face);

    void DebugList(const PODVector<IntVector2>& list);

    unsigned vertexMaskType_;

    float vertexScale_;

    unsigned currentFace_;

    IntVector3 currentIndex_;
};

}
