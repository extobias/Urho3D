#pragma once


#include "../Math/Ray.h"
#include "../UI/ImGuiElement.h"
#include "../UI/EditorWindow.h"
#include "../Graphics/CustomGeometry.h"
#include "../ThirdParty/ImGui/ImGuizmo.h"

namespace Urho3D
{

static const StringHash E_GUIZMO_NODE_SELECTED("GUIZMO_NODE_SELECTED");
static const StringHash P_GUIZMO_NODE_SELECTED("GUIZMO_NODE_SELECTED_ID");
static const StringHash P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX("GUIZMO_NODE_SELECTED_SUBELEMENTINDEX");
static const StringHash P_GUIZMO_NODE_SELECTED_POSITION("GUIZMO_NODE_SELECTED_POSITION");

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

	virtual void Render(float timeStep);

    void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor);

    void OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor);

	void SetSelectedNode(unsigned selectedNode) { selectedNode_ = selectedNode;	}

	void SetCurrentOperation(ImGuizmo::OPERATION currentOperation) { currentOperation_ = currentOperation; }

	void SetCurrentMode(ImGuizmo::MODE currentMode) { currentMode_ = currentMode; }

    void SetCurrentEditMode(EditorMode mode) { currentEditMode_ = mode; }

	void SetCameraNode(Node* node) { cameraNode_ = node; }

    void HandleNodeSelected(StringHash eventType, VariantMap& eventData);

    void SetScene(Scene* scene) override;

protected:


private:

	Ray GetCameraRay();

    Vector3 GetWorldPosition();

    RayQueryResult SelectObject(const IntVector2& position);

    void SelectVertex(const IntRect& screenRect);

    void CreateBrush();

	unsigned selectedNode_;

    WeakPtr<Node> cameraNode_;

	ImGuizmo::OPERATION currentOperation_;

	ImGuizmo::MODE currentMode_;

    EditorMode currentEditMode_;

    Ray ray_;

    Frustum selectFrustum_;

    Vector<Vector3> hitPositions_;

    SharedPtr<EditorBrush> brush_;
};

}
