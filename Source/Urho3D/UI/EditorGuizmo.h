#pragma once


#include "../Math/Ray.h"
#include "../UI/ImGuiElement.h"
#include "../ThirdParty/ImGui/ImGuizmo.h"

namespace Urho3D
{

static const StringHash E_GUIZMO_NODE_SELECTED("GUIZMO_NODE_SELECTED");
static const StringHash P_GUIZMO_NODE_SELECTED("GUIZMO_NODE_SELECTED_ID");
static const StringHash P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX("GUIZMO_NODE_SELECTED_SUBELEMENTINDEX");
static const StringHash P_GUIZMO_NODE_SELECTED_POSITION("GUIZMO_NODE_SELECTED_POSITION");

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

	void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition,
		int button, int buttons, int qualifiers, Cursor* cursor);

	void SetSelectedNode(unsigned selectedNode) { selectedNode_ = selectedNode;	}

	void SetCurrentOperation(ImGuizmo::OPERATION currentOperation) { currentOperation_ = currentOperation; }

	void SetCurrentMode(ImGuizmo::MODE currentMode) { currentMode_ = currentMode; }

	void SetCameraNode(Node* node) { cameraNode_ = node; }

private:

	Ray GetCameraRay();

	unsigned selectedNode_;

	Node* cameraNode_;

	ImGuizmo::OPERATION currentOperation_;

	ImGuizmo::MODE currentMode_;

        bool editNode_ { true };
};

}
