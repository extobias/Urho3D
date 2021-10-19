#pragma once

#include "../Editor/EditorWindow.h"
#include "../Math/Ray.h"
#include "../UI/ImGuiElement.h"
#include "../Graphics/CustomGeometry.h"

namespace Urho3D
{

class CollisionPolygon2D;
class EditorSelection;
class EditorBrush;

class URHO3D_API EditorGuizmo : public ImGuiElement
{
    URHO3D_OBJECT(EditorGuizmo, ImGuiElement);

public:
    /// Construct.
    explicit EditorGuizmo(Context* context);
    /// Destruct.
    ~EditorGuizmo() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    virtual void Render(float timeStep) override;

    void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, UIElement* beginElement) override;

    void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    bool IsWheelHandler() const override { return true; }

    void SetCurrentOperation(unsigned currentOperation) { currentOperation_ = currentOperation; }

    void SetCurrentMode(unsigned currentMode) { currentMode_ = currentMode; }

    void SetCurrentEditMode(EditorMode mode) { currentEditMode_ = mode; }

    void SetCameraNode(Node* node) { cameraNode_ = node; }

    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    void SetScene(Scene* scene);

    void SetSelection(class EditorSelection* selection) { selection_ = selection; }

    EditorSelection* GetSelection() const { return selection_; }

    int buttons_;

protected:


private:

    Ray GetCameraRay();

    Vector3 GetWorldPosition();

    RayQueryResult SelectObject(const IntVector2& position);

    void SelectObjects(const BoundingBox& boundingBox);

    void SelectVertex(const IntRect& screenRect);

    bool SelectVertex(const IntVector2& position, PODVector<IntVector2>& faces, PODVector<Vector2>& texUV);

    void CreateBrush();

    void CalculateHitPoint(const IntVector2 &position);

    void RenderVerticesPoint(Node* node);

    WeakPtr<Node> cameraNode_;

    WeakPtr<EditorSelection> selection_;

    unsigned currentOperation_;

    unsigned currentMode_;

    EditorMode currentEditMode_;

    Ray ray_;

    Vector<Vector3> hitPositions_;

    Vector3 hitPoint_;

    SharedPtr<EditorBrush> brush_;

    IntVector2 clickStart_;

    bool clicked_;

    bool editPointHover_;
};

}
