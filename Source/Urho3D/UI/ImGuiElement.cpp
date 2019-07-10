#include "../UI/ImGuiElement.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Texture2D.h"
#include "../IO/Log.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Core/CoreEvents.h"
#include "../Core/ProcessUtils.h"
#include "../Scene/Component.h"
#include "../Graphics/Camera.h"
#include "../Input/Input.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"

#include <SDL/SDL.h>

#include "imgui.h"
#include "ImGuizmo.h"

#define VERTEX_SIZE 6

static const int KEY_QUAL_OFFSET = 0x8000;
/*
 Cosas a resolver
 - el Context global no es una buena idea.
 - crear tantas texturas tampoco es buena idea.
 - el teclado tiene que andar en android
        - la solucion es medio chota
 */


static Context *g_context = 0;
static String g_programDir;

namespace Urho3D
{

extern const char* UI_CATEGORY;

/*------------ TBElement ------------*/
ImGuiElement::ImGuiElement(Context* context)
    : UIElement(context),
      selectedNode_(0)
{
    SetName("ImGuiElement");

    SetEnabled(true);
    SetFocusMode(FM_FOCUSABLE);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// ImGui::StyleColorsClassic();
	ImGui::StyleColorsDark();
	// ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10.0f);
	// ImGuiStyle& style = ImGui::GetStyle();
	// style.WindowBorderSize = 1.0f;
	// style.Alpha = 1.0f;

	// Create texture
	texture_ = new Texture2D(context_);
	texture_->SetMipsToSkip(QUALITY_LOW, 0);
	texture_->SetNumLevels(1);

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	URHO3D_LOGINFOF("get data <%u, %u>", width, height);

	if (!texture_->SetSize(width, height, Graphics::GetRGBAFormat()))
		URHO3D_LOGINFOF("tex <%p> error setsize!", texture_);

	if (!texture_->SetData(0, 0, 0, width, height, pixels))
		URHO3D_LOGINFOF("tex <%p> error setdata!", texture_);

	texture_->SetAddressMode(COORD_U, ADDRESS_WRAP);
	texture_->SetAddressMode(COORD_V, ADDRESS_WRAP);

	CreateKeyMap();

	SubscribeToEvents();
}

ImGuiElement::~ImGuiElement()
{
}

void ImGuiElement::SubscribeToEvents()
{
	SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(ImGuiElement, HandleScreenMode));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(ImGuiElement, HandlePostUpdate));

	SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(ImGuiElement, HandleBeginFrame));

	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(ImGuiElement, HandleKeyDown));
	SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(ImGuiElement, HandleKeyUp));

	SubscribeToEvent(E_SDLRAWINPUT, URHO3D_HANDLER(ImGuiElement, HandleRawEvent));

	//    SubscribeToEvent(this, E_FOCUSED, URHO3D_HANDLER(ImGuiElement, HandleFocused));
	//    SubscribeToEvent(this, E_DEFOCUSED, URHO3D_HANDLER(LineEdit, HandleDefocused));
}

void ImGuiElement::RegisterObject(Context* context)
{
    g_context = context;

    context->RegisterFactory<ImGuiElement>(UI_CATEGORY);
}

void ImGuiElement::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
	ImDrawData* draw_data = ImGui::GetDrawData();
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			// Project scissor/clipping rectangles into framebuffer space
			ImVec4 clip_rect;
			clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

			if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
			{
				unsigned offset1 = 0;
				IntRect scissor(clip_rect.x - offset1, clip_rect.y - offset1, clip_rect.z + offset1, clip_rect.w + offset1);
				// IntRect scissor((int)clip_rect.x, (int)clip_rect.y, (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
				// IntRect scissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

				UIBatch batch(this, BLEND_ALPHA, scissor, texture_, &vertexData);

				unsigned vertexSize = VERTEX_SIZE * pcmd->ElemCount;
				batch.vertexStart_ = vertexData.Size();
				batch.vertexEnd_ = batch.vertexStart_ + vertexSize;
				vertexData.Resize(batch.vertexStart_ + vertexSize);
				float* dest = &(vertexData.At(batch.vertexStart_));

				unsigned offset = 0;
				for (int i = 0; i < pcmd->ElemCount; i++, offset += VERTEX_SIZE)
				{
					const ImDrawVert* v = &cmd_list->VtxBuffer[cmd_list->IdxBuffer[pcmd->IdxOffset + i]];
					dest[offset + 0] = v->pos.x;
					dest[offset + 1] = v->pos.y;
					dest[offset + 2] = 0.0f;
					((unsigned&)dest[offset + 3]) = (unsigned)v->col;
					dest[offset + 4] = v->uv.x;
					dest[offset + 5] = v->uv.y;
				}

				UIBatch::AddOrMerge(batch, batches);
			}
		 }
	}
}

void ImGuiElement::CreateKeyMap()
{
}

int ImGuiElement::FindKeyMap(int key)
{
	return -1;
}

bool ImGuiElement::IsWithinScissor(const IntRect& currentScissor)
{
	return true;
}

void ImGuiElement::OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(position.x_, position.y_);
}

void ImGuiElement::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition,
                                int button, int buttons, int qualifiers, Cursor* cursor)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[0] = true;
}

void ImGuiElement::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition,
                              int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[0] = false;
}

void ImGuiElement::OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
}

void ImGuiElement::OnPositionSet(const IntVector2& newPosition)
{
}

void ImGuiElement::Update(float timeStep)
{
	static float f = 0.0f;
	static int counter = 0;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);

	static bool show_demo_window = true;
	static bool show_another_window = true;

	//ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32_WHITE);
	//ImGui::PopStyleColor();
	// if (show_demo_window)
	//  ImGui::ShowDemoWindow(&show_demo_window);
	
	auto components = scene_->GetComponents();
	ImGui::Begin("Scene Inspector");

	int i = 0;
	for(Component* c : components)
	{
		bool isOpen = ImGui::TreeNode((void*)(intptr_t)i, "%s", c->GetTypeName().CString());
		if (ImGui::IsItemClicked())
		{
			URHO3D_LOGERRORF("item selected <%i>", i);
		}
		if (isOpen)
		{
			ImGui::Text("blah blah");
			// ImGui::SameLine();
			if (ImGui::SmallButton("button")) {};
		
			ImGui::TreePop();
		}
		i++;
	}

	auto children = scene_->GetChildren();
	for (Node* child : children)
	{
		bool isOpen = ImGui::TreeNode((void*)(intptr_t)i, "%s - %i", child->GetName().CString(), child->GetID());
		if (ImGui::IsItemClicked())
		{
            selectedNode_ = child->GetID();
			URHO3D_LOGERRORF("item selected <%i>", child->GetID());
		}
		if (isOpen)
		{
			//ImGui::Text("blah blah");
			//ImGui::SameLine();
			//if (ImGui::SmallButton("button")) 
			//{
			//	URHO3D_LOGERROR("button down!");
			//}
			ImGui::TreePop();
		}
		i++;
	}

	ImGui::Separator();

    if (selectedNode_)
	{
        Node* node = scene_->GetNode(selectedNode_);
		if (node)
		{
            const Vector<AttributeInfo>* attr = node->GetAttributes();
            for (auto var : (*attr))
            {
                //URHO3D_LOGERRORF("name <%s> type <%s> default <%f>",
                //    var.name_.CString(), var.defaultValue_.GetTypeName().CString(), var.defaultValue_.GetFloat());

                ImGui::Text("name. %s type %s", var.name_.CString(), var.defaultValue_.GetTypeName().CString());
                // AddEditField(var);
            }
		}
		else
		{
            URHO3D_LOGERRORF("node <%u> not found!", selectedNode_);
		}
	}

	//ImGui::Checkbox("Demo Window", &show_demo_window);
	//ImGui::Checkbox("Another Window", &show_another_window);

	//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
	//ImGui::ColorEdit3("clear color", (float*)&clear_color);

	//if (ImGui::Button("Button"))
	//	counter++;
	//ImGui::SameLine();
	//ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::End();

    Node* node = scene_->GetNode(selectedNode_);
    if (node)
    {
        Node* cameraNode = scene_->GetChild("Camera");
        Camera* camera = cameraNode->GetComponent<Camera>();

        Matrix4 projection = camera->GetProjection();
        Matrix4 view = camera->GetView().ToMatrix4().Transpose();
        Matrix4 nodeTransform = node->GetTransform().ToMatrix4().Transpose();
        Matrix4 delta;

        ImGuizmo::Enable(true);
        //ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
        // ImGuizmo::DrawCube(view.Data(), projection.Data(), objectMatrix);
        ImGuizmo::Manipulate(&view.m00_, &projection.m00_, ImGuizmo::ROTATE, ImGuizmo::LOCAL, &nodeTransform.m00_);
    }
}

void ImGuiElement::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	// Rendering
	ImGui::Render();
}

void ImGuiElement::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();
	w = g->GetWidth();
	h = g->GetHeight();
	//glfwGetWindowSize(g_Window, &w, &h);
	//glfwGetFramebufferSize(g_Window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	// if (w > 0 && h > 0)
	//	io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
	using namespace BeginFrame;
	float timeStep = eventData[P_TIMESTEP].GetFloat();
	io.DeltaTime = timeStep < M_EPSILON ? 1.0f / 60.0f : timeStep;

	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
}

void ImGuiElement::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
	using namespace ScreenMode;
	int w = eventData[P_WIDTH].GetInt();
	int h = eventData[P_HEIGHT].GetInt();

	SetSize(w, h);
}

void ImGuiElement::HandleFocused(StringHash /*eventType*/, VariantMap& eventData)
{
}

void ImGuiElement::HandleKeyDown(StringHash eventType, VariantMap &eventData)
{
}

void ImGuiElement::HandleKeyUp(StringHash eventType, VariantMap &eventData)
{
}

void ImGuiElement::HandleRawEvent(StringHash eventType, VariantMap& args)
{
}

} // end namespace Urho3D

