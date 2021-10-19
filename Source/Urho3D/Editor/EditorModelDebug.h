#pragma once

#include "../Graphics/Drawable.h"

namespace Urho3D
{

class Drawable;
class Model;
class VertexBuffer;
class IndexBuffer;
class Texture2D;
class Image;

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

    void AddSelectedFaces(const PODVector<IntVector2>& faces, const PODVector<Vector2>& texUV, bool end = false);

    void AddPointUV(const IntVector2& screenPosition, const Vector2& texUV, Image* image, bool end);

    void DrawLine(const IntVector2& p1, const IntVector2& p2);

    void DrawLine2(const IntVector2& p1, const IntVector2& p2, Image* img, unsigned color);

    void DrawFaces(const PODVector<IntVector2>& faces);

    const PODVector<int>& GetSelectedVertex() const { return selectedIndex_; }

    const PODVector<IntVector2>& GetSelectedFaces() const { return selectedFaces_; }

    Model* GetModel() const { return model_; }

    void UpdateFacesIndexes();

    void ApplyVertexCollisionMask();

    void ApplyVertexUV(const PODVector<Vector2>& texUV);

    void AddVertexElement(unsigned vertexIndex, unsigned vertexElementIndex);

    void SaveModel();

    void SelectAll();

    void SetModelAttr(const ResourceRef& value);

    ResourceRef GetModelAttr() const;

    VertexCollisionMaskFlags GetCurrentMask() { return GetFaceCollisionMask(currentFace_ * 3); }

    IntVector3 GetCurrentIndex() const { return currentIndex_; }

    unsigned GetCurrentFace() const { return currentFace_; }

    unsigned GetFacesCount() const;

    void SetBrushColor(const Color& color) { brushColor_ = color; }

    void SetBrushSize(unsigned size) { brushSize_ = size; }
    
    void SaveImage(const String& name);

    bool SetSize(int width, int height, bool threaded);

    void CheckVertexVisibility(Camera* camera, Image* image);

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

    PODVector<IntVector3> selectedFacesIndex_;

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

    void CalculateViewport();

    inline Vector4 ModelTransform(const Matrix4& transform, const Vector3& vertex) const;

    inline Vector3 ViewportTransform(const Vector4& vertex) const;

    void DrawTriangle(Vector4* vertices, unsigned threadIndex, Image* image, Vector2* uvs);

    inline float SignedArea(const Vector3& v0, const Vector3& v1, const Vector3& v2) const;

    void ClipVertices(const Vector4& plane, Vector4* vertices, bool* triangles, unsigned& numTriangles);

    inline Vector4 ClipEdge(const Vector4& v0, const Vector4& v1, float d0, float d1) const;

    void DrawTriangle2D(const Vector3* vertices, bool clockwise, unsigned threadIndex, Image* img, Vector2* uvs);

    void DrawPoint(int x, int y, Image* image, unsigned color);

    bool IsInsideTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector2& pos);

    unsigned FindTriangleIndex(const Vector2& pos);

    unsigned vertexMaskType_;

    float vertexScale_;

    unsigned currentFace_;

    IntVector3 currentIndex_;

    WeakPtr<Texture2D> texture_;

    WeakPtr<Image> img_;

    WeakPtr<Image> imgAux_;

    unsigned* imgData_;

    IntVector2 lastUVPoint_;

    IntVector2 lastScreenPoint_;

    Color brushColor_;

    unsigned brushSize_;
    /// cullings vars
    PODVector<Vector3> vertices_;

    PODVector<Vector2> uvs_;

    CullMode cullMode_{CULL_CCW};

    Matrix4 projection_;
    /// Buffer width.
    int width_{};
    /// Buffer height.
    int height_{};
    /// X scaling for viewport transform.
    float scaleX_{};
    /// Y scaling for viewport transform.
    float scaleY_{};
    /// X offset for viewport transform.
    float offsetX_{};
    /// Y offset for viewport transform.
    float offsetY_{};
    /// Combined X projection and viewport transform.
    float projOffsetScaleX_{};
    /// Combined Y projection and viewport transform.
    float projOffsetScaleY_{};
    
    unsigned numTriangles_{};
};

}
