#pragma once

#include "../UI/ImGuiElement.h"

namespace Urho3D
{

class EditorGuizmo;

struct ResourceFile
{
	String prefix;
	String path;
	String name;
	String ext;
};

using ResourceDir = HashMap<String, Vector<ResourceFile>>;

using ResourceMap = HashMap<String, ResourceFile>;

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

	void SetCameraNode(Node* node) { cameraNode_ = node; }

private:
    void AttributeEdit(Component* c);

	int FindModel(const String& name);

	int FindMaterial(const String& name);

	unsigned selectedNode_;

	Node* cameraNode_;

	EditorGuizmo* guizmo_;

	ResourceDir resources_;

	ResourceMap modelResources_;
	String modelResourcesString_;

	ResourceMap materialResources_;
	String materialResourcesString_;

	int currentModel_;

	PODVector<int> currentMaterialList_;
};

}
