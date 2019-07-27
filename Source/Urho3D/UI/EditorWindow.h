#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{

class EditorGuizmo;

class URHO3D_API EditorWindow : public ImGuiElement
{
	URHO3D_OBJECT(EditorWindow, ImGuiElement);

public:
	/// Construct.
	explicit EditorWindow(Context* context);
	/// Destruct.
	~EditorWindow() override;
	/// Register object factory.
	static void RegisterObject(Context* context);

	virtual void Render(float timeStep);

	void SetGuizmo(EditorGuizmo* guizmo) { guizmo_ = guizmo; }

private:
    void AttributeEdit(Component* c);

	unsigned selectedNode_;

	EditorGuizmo* guizmo_;
};

}
