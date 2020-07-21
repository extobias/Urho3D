#pragma once


#include "../Math/Ray.h"
#include "../UI/ImGuiElement.h"
#include "../UI/EditorWindow.h"
#include "../Graphics/CustomGeometry.h"

namespace Urho3D
{

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

    void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor) override;

    void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement) override;

    void OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor) override;

    void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    bool IsWheelHandler() const override { return true; }

    void SetCurrentOperation(unsigned currentOperation) { currentOperation_ = currentOperation; }

    void SetCurrentMode(unsigned currentMode) { currentMode_ = currentMode; }

    void SetCurrentEditMode(EditorMode mode) { currentEditMode_ = mode; }

    void SetCameraNode(Node* node) { cameraNode_ = node; }

    void HandleNodeSelected(StringHash eventType, VariantMap& eventData);

    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    void SetScene(Scene* scene);

    void SetEditorSelection(EditorSelection* selection) { selection_ = selection; }

    int buttons_;

protected:


private:

    Ray GetCameraRay();

    Vector3 GetWorldPosition();

    RayQueryResult SelectObject(const IntVector2& position);

    void SelectObjects(const BoundingBox& boundingBox);

    void SelectVertex(const IntRect& screenRect);

    PODVector<IntVector2> SelectVertex(const IntVector2& position);

    void CreateBrush();

    void CalculateHitPoint(const IntVector2 &position);

    void RenderVerticesPoint();

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
};

}
