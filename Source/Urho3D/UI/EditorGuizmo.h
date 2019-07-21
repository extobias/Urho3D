#pragma once

#include "../UI/ImGuiElement.h"
#include "../ThirdParty/ImGui/ImGuizmo.h"

namespace Urho3D
{

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

	void SetSelectedNode(unsigned selectedNode) { selectedNode_ = selectedNode;	}

	void SetCurrentOperation(ImGuizmo::OPERATION currentOperation) { currentOperation_ = currentOperation; }

	void SetCurrentMode(ImGuizmo::MODE currentMode) { currentMode_ = currentMode; }

private:

	unsigned selectedNode_;

	ImGuizmo::OPERATION currentOperation_;
	ImGuizmo::MODE currentMode_;
};

}