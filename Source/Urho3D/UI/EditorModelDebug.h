#pragma once

#include "../Graphics/Drawable.h"

namespace Urho3D
{

class Drawable;
class Model;
class VertexBuffer;
class IndexBuffer;

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

    void UpdateBatches(const FrameInfo& frame) override;

    void SetModel(Model* model);

    void SetMaterial(Material* material);

    void SetBoundingBox(const BoundingBox& box);

protected:
     virtual void OnWorldBoundingBoxUpdate();

    PrimitiveType primitiveType_;

    SharedPtr<Model> model_;

    SharedPtr<Geometry> geometry_;

    SharedPtr<VertexBuffer> vertexBuffer_;

    SharedPtr<IndexBuffer> indexBuffer_;

    PODVector<VertexElement> vertexElements_;

    Matrix3x4 transform_;

    PODVector<Matrix3x4> worldTransforms_;

private:
    /// Handle model reload finished.
    void HandleModelReloadFinished(StringHash eventType, VariantMap& eventData);
};

}
