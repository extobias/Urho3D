#include "../UI/EditorWindow.h"
#include "../UI/EditorGuizmo.h"

#include "../IO/Log.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Camera.h"
#include "../Graphics/ParticleEffect.h"
#include "../Graphics/ParticleEmitter.h"
#include "../Graphics/Material.h"

#include "imgui.h"
// #include "ImGuizmo.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorWindow::EditorWindow(Context* context) :
	ImGuiElement(context),
	guizmo_(nullptr),
	selectedNode_(0)
{
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::RegisterObject(Context* context)
{
	context->RegisterFactory<EditorWindow>(UI_CATEGORY);
}

void EditorWindow::Render(float timeStep)
{
	ImGui::Begin("Scene Inspector");

	// Guizmo stuff
	static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::ROTATE);
	static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

	if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
		currentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
		currentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
		currentGizmoOperation = ImGuizmo::SCALE;

	if (currentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL))
			currentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD))
			currentGizmoMode = ImGuizmo::WORLD;
	}

	if (guizmo_)
	{
		guizmo_->SetCurrentOperation(currentGizmoOperation);
		guizmo_->SetCurrentMode(currentGizmoMode);
	}

	ImGui::Separator();

	int i = 0;
	auto components = scene_->GetComponents();
	for (Component* c : components)
	{
		bool isOpen = ImGui::TreeNode((void*)(intptr_t)i, "%s", c->GetTypeName().CString());
		if (ImGui::IsItemClicked())
		{
			URHO3D_LOGERRORF("item selected <%i>", i);
		}
		if (isOpen)
		{
			// ImGui::Text("blah blah");
			// ImGui::SameLine();
			// if (ImGui::SmallButton("button")) {};

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
			if (guizmo_)
			{
				guizmo_->SetSelectedNode(selectedNode_);
				URHO3D_LOGERRORF("item selected <%i>", child->GetID());
			}
		}
        if (isOpen)
        {
//			auto childComponents = child->GetComponents();
//			for (Component* c : childComponents)
//			{
//				// ImGui::Text("%s", c->GetTypeName().CString());
//				if (ImGui::CollapsingHeader(c->GetTypeName().CString()))
//				{
//					AttributeEdit(c);
//				}
//			}
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
			Node* cameraNode = scene_->GetChild("Camera");
			Camera* camera = cameraNode->GetComponent<Camera>();

			Matrix4 identity = Matrix4::IDENTITY;
			Matrix4 projection = camera->GetProjection().Transpose();
			Matrix4 view = camera->GetView().ToMatrix4().Transpose();
			Matrix4 nodeTransform = node->GetTransform().ToMatrix4().Transpose();
			// Matrix4 nodeTransform = Matrix4::IDENTITY.Transpose();
			Matrix4 delta;

			const unsigned len = node->GetName().Length();
			char name[512];
			sprintf(name, node->GetName().CString());
			ImGui::InputText("Name", name, node->GetName().Length());

			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(&nodeTransform.m00_, matrixTranslation, matrixRotation, matrixScale);
			ImGui::InputFloat3("Tr", matrixTranslation, 3);
			ImGui::InputFloat3("Rt", matrixRotation, 3);
			ImGui::InputFloat3("Sc", matrixScale, 3);
			ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, &nodeTransform.m00_);

            // components
            auto childComponents = node->GetComponents();
            for (Component* c : childComponents)
            {
                // ImGui::Text("%s", c->GetTypeName().CString());
                if (ImGui::CollapsingHeader(c->GetTypeName().CString()))
                {
                    AttributeEdit(c);
                }
            }
		}
		else
		{
			URHO3D_LOGERRORF("node <%u> not found!", selectedNode_);
		}
	}

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImVec2 windowPos = ImGui::GetWindowPos();
	SetPosition(windowPos.x, windowPos.y);

	ImVec2 windowSize = ImGui::GetWindowSize();
	SetSize(windowSize.x, windowSize.y);

	ImGui::End();
}

void EditorWindow::AttributeEdit(Component* c)
{
	// URHO3D_LOGERRORF("name <%s> type <%s> default <%f>", info.name_.CString(), var.defaultValue_.GetTypeName().CString(), var.defaultValue_.GetFloat());

	const Vector<AttributeInfo>* attr = c->GetAttributes();
	for (auto var : (*attr))
	{
		const AttributeInfo& info = var;
		switch (info.type_)
		{
		case VAR_BOOL:
		{
			bool v = c->GetAttribute(info.name_).GetBool();
			ImGui::Checkbox(info.name_.CString(), &v);
			c->SetAttribute(info.name_, v);
		}
		break;
		case VAR_INT:
		{
			int v = c->GetAttribute(info.name_).GetInt();
			ImGui::InputInt(info.name_.CString(), &v);
			c->SetAttribute(info.name_, v);
		}
		break;
		case VAR_FLOAT:
		{
			float v = c->GetAttribute(info.name_).GetFloat();
			ImGui::InputFloat(info.name_.CString(), &v);
			c->SetAttribute(info.name_, v);
		}
		break;
		case VAR_STRING:
		{
			String v = c->GetAttribute(info.name_).GetString();
			char buffer[512];
			strncpy(buffer, v.CString(), v.Length());
			ImGui::InputText(info.name_.CString(), buffer, 512);
			c->SetAttribute(info.name_, buffer);
		}
		break;
		case VAR_RESOURCEREF:
		{
			//resourcePickers.Push(ResourcePicker("Animation", "*.ani", ACTION_PICK | ACTION_TEST));
			//resourcePickers.Push(ResourcePicker("Font", fontFilters));
			//resourcePickers.Push(ResourcePicker("Image", imageFilters));
			//resourcePickers.Push(ResourcePicker("LuaFile", luaFileFilters));
			//resourcePickers.Push(ResourcePicker("Material", materialFilters, ACTION_PICK | ACTION_OPEN | ACTION_EDIT));
			//resourcePickers.Push(ResourcePicker("Model", "*.mdl", ACTION_PICK));
			//resourcePickers.Push(ResourcePicker("ParticleEffect", "*.xml", ACTION_PICK | ACTION_OPEN | ACTION_EDIT));
			//resourcePickers.Push(ResourcePicker("ScriptFile", scriptFilters));
			//resourcePickers.Push(ResourcePicker("Sound", soundFilters));
			//resourcePickers.Push(ResourcePicker("Technique", "*.xml"));
			//resourcePickers.Push(ResourcePicker("Texture2D", textureFilters));
			//resourcePickers.Push(ResourcePicker("TextureCube", "*.xml"));
			//resourcePickers.Push(ResourcePicker("Texture3D", "*.xml"));
			//resourcePickers.Push(ResourcePicker("XMLFile", "*.xml"));
			//resourcePickers.Push(ResourcePicker("JSONFile", "*.json"));
			//resourcePickers.Push(ResourcePicker("Sprite2D", textureFilters, ACTION_PICK | ACTION_OPEN));
			//resourcePickers.Push(ResourcePicker("AnimationSet2D", anmSetFilters, ACTION_PICK | ACTION_OPEN));
			//resourcePickers.Push(ResourcePicker("ParticleEffect2D", pexFilters, ACTION_PICK | ACTION_OPEN));
			//resourcePickers.Push(ResourcePicker("TmxFile2D", tmxFilters, ACTION_PICK | ACTION_OPEN));

			ResourceRef v = c->GetAttribute(info.name_).GetResourceRef();
			ImGui::Text("type <%s> name <%s>", info.defaultValue_.GetTypeName().CString(), v.name_.CString());
			if (v.type_ == StringHash("ParticleEffect"))
			{
				ParticleEmitter* emitter = dynamic_cast<ParticleEmitter*>(c);
				if (emitter)
				{
					ParticleEffect* effect = emitter->GetEffect();

					float constantForce[3];
					Vector3 econstantForce = effect->GetConstantForce();
					memcpy(constantForce, econstantForce.Data(), sizeof(constantForce));
					ImGui::InputFloat3("Constant Force", constantForce);
					effect->SetConstantForce(Vector3(constantForce));

					float directionMin[3];
					Vector3 edirectionMin = effect->GetMinDirection();
					memcpy(directionMin, edirectionMin.Data(), sizeof(directionMin));
					ImGui::InputFloat3("Direction (Min)", directionMin);
					effect->SetMinDirection(Vector3(directionMin));

					float directionMax[3];
					Vector3 edirectionMax = effect->GetMaxDirection();
					memcpy(directionMax, edirectionMax.Data(), sizeof(directionMax));
					ImGui::InputFloat3("Direction (Max)", directionMax);
					effect->SetMaxDirection(Vector3(directionMax));

					float dampingForce = effect->GetDampingForce();
					ImGui::InputFloat("Damping Force", &dampingForce);
					effect->SetDampingForce(dampingForce);

					float activeTime = effect->GetActiveTime();
					ImGui::InputFloat("Active Time", &activeTime);
					effect->SetActiveTime(activeTime);

					float minParticleSize[2];
					Vector2 eminParticleSize = effect->GetMinParticleSize();
					memcpy(minParticleSize, eminParticleSize.Data(), sizeof(minParticleSize));
					ImGui::InputFloat2("Particle Size (Min)", minParticleSize);
					effect->SetMinParticleSize(Vector2(minParticleSize));

					float maxParticleSize[2];
					Vector2 emaxParticleSize = effect->GetMaxParticleSize();
					memcpy(maxParticleSize, emaxParticleSize.Data(), sizeof(maxParticleSize));
					ImGui::InputFloat2("Particle Size (Max)", maxParticleSize);
					effect->SetMaxParticleSize(Vector2(maxParticleSize));

					float timeToLive[2];
					timeToLive[0] = effect->GetMinTimeToLive();
					timeToLive[1] = effect->GetMaxTimeToLive();
					ImGui::InputFloat2("Time to Live", timeToLive);
					effect->SetMinTimeToLive(timeToLive[0]);
					effect->SetMaxTimeToLive(timeToLive[1]);
					
					float velocity[2];
					velocity[0] = effect->GetMinVelocity();
					velocity[1] = effect->GetMaxVelocity();
					ImGui::InputFloat2("Velocity", velocity);
					effect->SetMinVelocity(velocity[0]);
					effect->SetMaxVelocity(velocity[1]);

					float rotation[2];
					rotation[0] = effect->GetMinRotation();
					rotation[1] = effect->GetMaxRotation();
					ImGui::InputFloat2("Rotation", rotation);
					effect->SetMinRotation(rotation[0]);
					effect->SetMaxRotation(rotation[1]);

					float rotationSpeed[2];
					rotationSpeed[0] = effect->GetMinRotationSpeed();
					rotationSpeed[1] = effect->GetMaxRotationSpeed();
					ImGui::InputFloat2("Rotation Speed", rotationSpeed);
					effect->SetMinRotationSpeed(rotationSpeed[0]);
					effect->SetMaxRotationSpeed(rotationSpeed[1]);

					ImGui::Separator(); ImGui::Text("Variation");

					float sizeAdd = effect->GetSizeAdd();
					ImGui::InputFloat("Size Add", &sizeAdd);
					effect->SetSizeAdd(sizeAdd);

					float sizeMul = effect->GetSizeMul();
					ImGui::InputFloat("Size Multiply", &sizeMul);
					effect->SetSizeMul(sizeMul);

					float animBias = effect->GetAnimationLodBias();
					ImGui::InputFloat("Animation LOD", &animBias);
					effect->SetAnimationLodBias(animBias);

					ImGui::Separator(); ImGui::Text("Emitter");

					int numParticles = effect->GetNumParticles();
					ImGui::InputInt("Num Particles", &numParticles);
					effect->SetNumParticles(numParticles);
					emitter->ApplyEffect();

					float emitterSize[3];
					Vector3 eemitterSize = effect->GetEmitterSize();
					memcpy(emitterSize, eemitterSize.Data(), sizeof(emitterSize));
					effect->SetEmitterSize(Vector3(emitterSize));

					float emissionRate[2];
					emissionRate[0] = effect->GetMinEmissionRate();
					emissionRate[1] = effect->GetMaxEmissionRate();
					ImGui::InputFloat2("Emission Rate", emissionRate);
					effect->SetMinEmissionRate(emissionRate[0]);
					effect->SetMaxEmissionRate(emissionRate[1]);

					const char* emitterTypes[] = { "Sphere", "Box", "SphereVolume", "Cylinder", "Ring" };
					int emitterType = effect->GetEmitterType();
					ImGui::Combo("Emitter Type", &emitterType, emitterTypes, IM_ARRAYSIZE(emitterTypes));
					effect->SetEmitterType((EmitterType)emitterType);

					ImGui::Separator(); ImGui::Text("Renderer");

					Material* material = effect->GetMaterial();
					ImGui::Text("Material", material->GetName().CString());

					bool relative = effect->IsRelative();
					ImGui::Checkbox("Relative Transform", &relative);
					effect->SetRelative(relative);
					emitter->ApplyEffect();

					bool scaled = effect->IsScaled();
					ImGui::Checkbox("Scaled", &scaled);
					effect->SetScaled(scaled);
					emitter->ApplyEffect();

					bool sorted = effect->IsSorted();
					ImGui::Checkbox("Sorted", &sorted);
					effect->SetSorted(sorted);
					emitter->ApplyEffect();

					bool fixedScreen = effect->IsFixedScreenSize();
					ImGui::Checkbox("Fixed Screen", &fixedScreen);
					effect->SetFixedScreenSize(fixedScreen);
					emitter->ApplyEffect();

					const char* faceCameraModes[] = { "None", "Rotate XYZ", "Rotate Y", "LookAt XYZ", "LookAt Y", "LookAt Mixed", "Direction" };
					int cameraMode = effect->GetFaceCameraMode();
					ImGui::Combo("Face Mode", &cameraMode, faceCameraModes, IM_ARRAYSIZE(faceCameraModes));
					effect->SetFaceCameraMode((FaceCameraMode)cameraMode);
					emitter->ApplyEffect();
				}
				
			}
			//strncpy(buffer, v.CString(), v.Length());
			//ImGui::InputText(info.name_.CString(), buffer, 512);
			//c->SetAttribute(info.name_, buffer);
		}
		break;
		default:
		{
			ImGui::Text("type <%s> name <%s>", info.defaultValue_.GetTypeName().CString(), info.name_.CString());
		}
		break;
		}
	}
}

}
