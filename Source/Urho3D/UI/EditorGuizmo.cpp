#include "../UI/EditorGuizmo.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/Camera.h"

#include "imgui.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorGuizmo::EditorGuizmo(Context* context) :
	ImGuiElement(context)
{
}

EditorGuizmo::~EditorGuizmo() = default;

void EditorGuizmo::RegisterObject(Context* context)
{
	context->RegisterFactory<EditorGuizmo>(UI_CATEGORY);
}

void EditorGuizmo::Render(float timeStep)
{
	Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();

	ImGuizmo::BeginFrame();
	ImGuizmo::SetRect(0, 0, g->GetWidth(), g->GetHeight());
	SetPosition(0, 0);
	SetSize(g->GetWidth(), g->GetHeight());

	// Node* cameraNode = scene_->GetNode(cameraNode_);
	Node* cameraNode = cameraNode_;
	if (!cameraNode)
		return;
	Camera* camera = cameraNode->GetComponent<Camera>();
	if (!camera)
		return;

	Node* node = scene_->GetNode(selectedNode_);
	if (node)
	{
		Matrix4 identity = Matrix4::IDENTITY;
		Matrix4 projection = camera->GetProjection().Transpose();
		Matrix4 view = camera->GetView().ToMatrix4().Transpose();
		Matrix4 nodeTransform = node->GetTransform().ToMatrix4().Transpose();

		ImGuizmo::Manipulate(&view.m00_, &projection.m00_, currentOperation_, currentMode_, &nodeTransform.m00_);

		node->SetTransform(Matrix3x4(nodeTransform.Transpose()));
	}
}

}