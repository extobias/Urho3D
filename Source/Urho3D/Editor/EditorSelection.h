#pragma once

#include "../Core/Object.h"

namespace Urho3D
{
class Node;
class Scene;

class URHO3D_API EditorSelection : public Object
{
    URHO3D_OBJECT(EditorSelection, Object);

public:

    explicit EditorSelection(Context* context);

    ~EditorSelection() override;

    void Add(Node* node);

    void Clear();

    bool IsEmpty() const { return !selectedNodes_.Size(); }

    String ToString();

    void SetScene(Scene* scene) { scene_ = scene; }

    void SetTransform(const Matrix3x4& matrix);

    void SetDelta(const Matrix4 &matrix, unsigned operation);

    void Render();

    // const PODVector<Node*>& GetSelectedNodes() const { return selectedNodes_; }

    const Vector<SharedPtr<Node> >& GetSelectedNodes() const { return selectedNodes_; }

    const Matrix3x4& GetTransform() const { return transform_; }

private:

    void UpdateTransform();

    bool PointAboveLine(Vector3 point, Vector3 p1, Vector3 p2);

    void PoligonPoints(Vector<Vector3>& points);

    Vector3 CalculateCentroid(const Vector<Vector3>& points);

    // PODVector<Node*> selectedNodes_;

    Vector<SharedPtr<Node> > selectedNodes_;

    Matrix3x4 transform_;

    Scene* scene_;

//        unsigned selectedSubElementIndex_{ M_MAX_UNSIGNED };
};

}
