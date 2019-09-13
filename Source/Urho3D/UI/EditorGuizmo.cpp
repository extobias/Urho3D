#include "../UI/EditorGuizmo.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/View.h"
#include "../Graphics/DebugRenderer.h"
#include "../Input/Input.h"
#include "../Resource/ResourceCache.h"
#include "../UI/UI.h"
#include "../IO/Log.h"

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

    if(editNode_)
    {
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
    else
    {

    }
}

Ray EditorGuizmo::GetCameraRay()
{
	Renderer* renderer = GetSubsystem<Renderer>();
	Viewport* viewport = renderer->GetViewport(0);
	View* view = viewport->GetView();
	if (!view)
		return Ray();

	IntRect rect = view->GetViewRect();
	/*URHO3D_LOGERRORF("editorgizmo.getcameraray: rect <%f, %f, %f, %f>",
		rect.Top(), rect.Left(), rect.Width(), rect.Height());*/

	UI* ui = GetSubsystem<UI>();
	IntVector2 cursorPos = ui->GetCursorPosition();

	Node* cameraNode = cameraNode_;
	if (!cameraNode)
		return Ray();
	Camera* camera = cameraNode->GetComponent<Camera>();
	float x = float(cursorPos.x_ - rect.Left()) / rect.Width();
	float y = float(cursorPos.y_ - rect.Top()) / rect.Height();
	// URHO3D_LOGERRORF("editorgizmo.getcameraray: pos <%f, %f>", x, y);
	return camera->GetScreenRay(x, y);
}

void EditorGuizmo::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition,
	int button, int buttons, int qualifiers, Cursor* cursor)
{
	ImGuiElement::OnClickBegin(position, screenPosition, button, buttons, qualifiers, cursor);

	URHO3D_LOGERRORF("editorguizmo.onclickbegin: position <%u, %u>", position.x_, position.y_);
	// check new selected node
	if (!ImGuizmo::IsOver() && button & MOUSEB_LEFT)
	{
		Octree* octree = scene_->GetComponent<Octree>();
		Renderer* renderer = GetSubsystem<Renderer>();
		Viewport* v0 = renderer->GetViewport(0);
		IntVector2 mousePos = position;
		Ray ray = v0->GetScreenRay(mousePos.x_, mousePos.y_);
        DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();

		PODVector<RayQueryResult> result;
		RayOctreeQuery query(result, ray);
		octree->Raycast(query);

		if (result.Size())
		{
			// int i = 0;
			for (int i = 0; i < result.Size(); i++)
			{
				RayQueryResult r = result[i];
				Node* hitNode = r.drawable_->GetNode();

				if (hitNode)
				{
					String name = hitNode->GetName();
					if (name == "Zone")
						continue;

					selectedNode_ = hitNode->GetID();

					VariantMap eventData;
					eventData[P_GUIZMO_NODE_SELECTED] = selectedNode_;
                    eventData[P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX] = r.subObjectElementIndex_;
                    eventData[P_GUIZMO_NODE_SELECTED_POSITION] = r.position_;
					SendEvent(E_GUIZMO_NODE_SELECTED, eventData);

                    URHO3D_LOGERRORF("hit node <%s> subObject <%i>", name.CString(), r.subObjectElementIndex_);
					break;
				}
			}
		}
	}
}

}
