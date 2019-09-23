#include "../UI/EditorWindow.h"
#include "../UI/EditorGuizmo.h"

#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Camera.h"
#include "../Graphics/ParticleEffect.h"
#include "../Graphics/ParticleEmitter.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Geometry.h"
#include "../Math/Polyhedron.h"

#include "imgui.h"

namespace Urho3D
{

//class URHO3D_API EditorBrush : public Object
//{
//public:
//    explicit EditorBrush(Context* context);

//    ~EditorBrush();

//    void Update(StaticModel* model, const Vector3& position, float radius);

//private:
//    SharedPtr<Node> node_;
//    SharedPtr<CustomGeometry> customGeometry_;
//    bool addedToOctree_{ false };
//};

//EditorBrush::EditorBrush(Context *context)
//    : Object(context)
//{

//}

//EditorBrush::~EditorBrush() = default;

//void EditorBrush::Update(StaticModel* model, const Vector3& position, float radius)
//{

//}

extern const char* UI_CATEGORY;

EditorWindow::EditorWindow(Context* context) :
	ImGuiElement(context),
	guizmo_(nullptr),
	selectedNode_(0),
	currentModel_(2)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	FileSystem dir(context_);

	const Vector<String>& dirs = cache->GetResourceDirs();
	for (int i = 0; i < dirs.Size(); i++)
	{
		String dirPath = dirs.At(i);
		URHO3D_LOGERRORF("dir <%s>", dirPath.CString());

		Vector<String> result;
		dir.ScanDir(result, dirPath, "*.*", SCAN_FILES, true);
		for (int j = 0; j < result.Size(); j++)
		{
			String prefix, name, ext;
			SplitPath(result.At(j), prefix, name, ext);

			ext.Erase(0, 1);

			ResourceFile resFile;
			resFile.prefix = prefix;
			resFile.path = dirPath;
			resFile.ext = ext;
			resFile.name = result.At(j);

			if (!resources_.Contains(prefix))
			{
				Vector<ResourceFile> list;
				list.Push(resFile);
				resources_.Insert(Pair<String, Vector<ResourceFile>>(prefix, list));
			}
			else
			{
				resources_[prefix].Push(resFile);
			}
		}
	}

	StringVector keys = resources_.Keys();
	URHO3D_LOGERRORF("keys size <%i>", keys.Size());
	ResourceMap modelResources;
	for (int i = 0; i < keys.Size(); i++)
	{
		Vector<ResourceFile> list = resources_[keys.At(i)];
		// URHO3D_LOGERRORF("mapkey <%s> size <%i>", keys.At(i).CString(), list.Size());
		for (int l = 0; l < list.Size(); l++)
		{
			ResourceFile file = list.At(l);
			String modelsFilter("mdl");
			if (file.ext == modelsFilter)
			{
				URHO3D_LOGERRORF("	file <%s> dirPath <%s> path <%s> ext <%s>", file.name.CString(), file.prefix.CString(), file.path.CString(), file.ext.CString());

				modelResources_[file.name] = file;
			}
			else if (file.prefix.Contains("materials", false) && (file.ext == "xml" || file.ext == "material" || file.ext == "json"))
			{
				materialResources_[file.name] = file;
			}
		}
	}

	modelResourcesString_.Join(modelResources_.Keys(), "@");
	modelResourcesString_.Replace('@', '\0');
	modelResourcesString_.Append('\0');

	materialResourcesString_.Join(materialResources_.Keys(), "@");
	materialResourcesString_.Replace('@', '\0');
	materialResourcesString_.Append('\0');

	SubscribeToEvent(E_GUIZMO_NODE_SELECTED, URHO3D_HANDLER(EditorWindow, HandleNodeSelected));
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::RegisterObject(Context* context)
{
	context->RegisterFactory<EditorWindow>(UI_CATEGORY);
}

void EditorWindow::HandleNodeSelected(StringHash eventType, VariantMap& eventData)
{
    selectedNode_ = eventData[P_GUIZMO_NODE_SELECTED].GetUInt();

    // URHO3D_LOGERRORF("editwindow: node selected <%i>", selectedNode_);

    selectedSubElementIndex_ = eventData[P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX].GetUInt();
    hitPosition_ = eventData[P_GUIZMO_NODE_SELECTED_POSITION].GetVector3();
}

void EditorWindow::Render(float timeStep)
{
	static bool closable = true;
	ImGui::Begin("Scene Inspector", &closable);

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

	ImGui::Separator();

	ImGui::BeginGroup();
	ImGui::BeginChild("Nodes", ImVec2(ImGui::GetContentRegionAvail().x, 200.0f), true);

	auto children = scene_->GetChildren();
	for (Node* child : children)
	{
        bool isOpen = ImGui::TreeNode((void*)(intptr_t)i, "%s - %i - %i", child->GetName().CString(), child->GetID(), child->GetChildren().Size());
		if (ImGui::IsItemClicked())
		{
			selectedNode_ = child->GetID();

            VariantMap eventData;
            eventData[P_EDITOR_NODE_SELECTED] = selectedNode_;

            SendEvent(E_EDITOR_NODE_SELECTED, eventData);
		}
        if (isOpen)
        {
            auto grantchidren = child->GetChildren();
            for (Node* grantchild : grantchidren)
                DrawChild(grantchild, i);

            ImGui::TreePop();
        }
		i++;
	}

	ImGui::EndChild();
	ImGui::EndGroup();

	// ImGui::Separator();

	if (selectedNode_)
	{
		Node* node = scene_->GetNode(selectedNode_);
		if (node)
		{
			// Node* cameraNode = scene_->GetChild("Camera");
			Node* cameraNode = cameraNode_;
            if(!cameraNode)
                return;

			Camera* camera = cameraNode->GetComponent<Camera>();
            if(!camera)
                return;

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
			bool modified = false;
			ImGui::InputFloat3("Tr", matrixTranslation, 3);
			ImGui::InputFloat3("Rt", matrixRotation, 3);
			ImGui::InputFloat3("Sc", matrixScale, 3);
			ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, &nodeTransform.m00_);

			node->SetTransform(Matrix3x4(nodeTransform.Transpose()));
			
            // components
			ImGui::BeginGroup();
			ImGui::BeginChild("Components", ImVec2(ImGui::GetContentRegionAvail().x, 200.0f), true);

            auto childComponents = node->GetComponents();
            for (Component* c : childComponents)
            {
                // ImGui::Text("%s", c->GetTypeName().CString());
                if (ImGui::CollapsingHeader(c->GetTypeName().CString()))
                {
                    // AttributeEdit(c);
                }

                StaticModel* staticModel = dynamic_cast<StaticModel*>(c);
                if (staticModel)
				{
					DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
                    // staticModel->DrawDebugGeometry(debugRenderer, true);

                    Model* model = staticModel->GetModel();
                    ImGui::Text("geom <%i> vb <%i> ib <%i> olod <%i>", model->GetNumGeometries(),
                                model->GetVertexBuffers().Size(), model->GetIndexBuffers().Size(), staticModel->GetOcclusionLodLevel());

                    for(unsigned int i = 0; i < model->GetIndexBuffers().Size(); i++)
                    {
                        IndexBuffer* ib = model->GetIndexBuffers().At(i);
                        ImGui::Text("ib size <%i> count <%i>", ib->GetIndexSize(), ib->GetIndexCount());
                    }

                    Matrix3x4 transform = node->GetWorldTransform();
                    const BoundingBox& modelBoundingBox = model->GetBoundingBox();

                    for(unsigned int i = 0; i < model->GetVertexBuffers().Size(); i++)
                    {
                        VertexBuffer* vb = model->GetVertexBuffers().At(i);
                        ImGui::Text("vb size <%i> count <%i> shadow <%i>", vb->GetVertexSize(), vb->GetVertexCount(), vb->IsShadowed());

//                        const SharedArrayPtr<unsigned char>& vertexData = vb->GetShadowDataShared();
//                        const auto* srcData = (const unsigned char*)&vertexData[0];
//                        unsigned vertexSize = vb->GetVertexSize();

//                        Sphere sphere;
//                        sphere.radius_ = 0.01f;

//                        for (unsigned j = 0; j < vb->GetVertexCount(); j++)
//                        {
//                            Vector3 v0 = transform * *((const Vector3*)(&srcData[j * vertexSize]));

//                            sphere.center_ = v0;
//                            BoundingBox bb(sphere);
//                            Polyhedron poly(bb);

//                            debugRenderer->AddPolyhedron(poly, Color::RED);
//                        }
                    }      

//                    for(unsigned int i = 0; i < model->GetNumGeometries(); i++)
//                    {
//                        unsigned int lodLevel = model->GetNumGeometryLodLevels(i);
//                        for (unsigned int j = 0; j < lodLevel ; j++)
//                        {
//                            Geometry* geom = model->GetGeometry(i, j);
//                            ImGui::Text("geom <%i> lod <%i> vb <%i> ib <%i> is <%i> ic <%i>", i, j,
//                                        geom->GetVertexCount(), geom->GetIndexCount(), geom->GetIndexStart(), geom->GetIndexCount());
//                        }
//                    }

                    const Vector<SharedPtr<VertexBuffer> >& vb = model->GetVertexBuffers();
                    for(unsigned i = 0; i < vb.Size(); i++)
                    {
                        VertexBuffer* vertexBuffer = vb.At(i);
                        const PODVector<VertexElement>& ves = vertexBuffer->GetElements();
                        for(unsigned j = 0; j < ves.Size(); j++)
                        {
                            const VertexElement& ve = ves.At(j);
                            ImGui::Text("vertexbuffer <%i> vertexlement <%i> index <%i> offset <%i> semantic <%i> type <%i>", i, j,
                                        ve.index_, ve.offset_, ve.semantic_, ve.type_);
                        }
                    }

                    Geometry* geom = model->GetGeometry(0, 0);
                    SharedArrayPtr<unsigned char> vertexData;
                    SharedArrayPtr<unsigned char> indexData;
                    unsigned vertexSize;
                    unsigned indexSize;
                    const PODVector<VertexElement>* elements;

                    geom->GetRawDataShared(vertexData, vertexSize, indexData, indexSize, elements);
                    Color color = Color::YELLOW;
                    ImGui::Text("geom vsize <%i> isize <%i> is <%i> ic <%i>", vertexSize, indexSize, geom->GetIndexStart(), geom->GetIndexCount());

                    if(selectedSubElementIndex_ != M_MAX_UNSIGNED)
                    {
                        auto* vertices = &vertexData[0];
                        unsigned short* index = ((unsigned short*)&indexData[0]) + geom->GetIndexStart() + selectedSubElementIndex_;

                        const Vector3& v0 = *((const Vector3*)(&vertices[index[0] * vertexSize]));
                        const Vector3& v1 = *((const Vector3*)(&vertices[index[1] * vertexSize]));
                        const Vector3& v2 = *((const Vector3*)(&vertices[index[2] * vertexSize]));

                        const Matrix3x4& worldTransform = transform; //node->GetWorldTransform();
                        // debugRenderer->AddTriangle(worldTransform * v0, worldTransform * v1, worldTransform * v2, Color::CYAN);

//                        float d0 = (worldTransform * v0).DistanceToPoint(hitPosition_);
//                        float d1 = (worldTransform * v1).DistanceToPoint(hitPosition_);
//                        float d2 = (worldTransform * v2).DistanceToPoint(hitPosition_);

//                        Sphere sphere;
//                        sphere.radius_ = 0.02f;
//                        Color sphereColor;
//                        sphereColor.FromHSL(39.0f, 100.0f, 50.0f);

//                        if(d0 < d1 && d0 < d2)
//                        {
//                            sphere.center_ = (worldTransform * v0);
//                            debugRenderer->AddSphere(sphere, sphereColor);
//                        }
//                        else if(d1 < d0 && d1 < d2)
//                        {
//                            sphere.center_ = (worldTransform * v1);
//                            debugRenderer->AddSphere(sphere, sphereColor);
//                        }
//                        else
//                        {
//                            sphere.center_ = (worldTransform * v2);
//                            debugRenderer->AddSphere(sphere, sphereColor);
//                        }
                    }

                    // debugRenderer->AddTriangleMesh(&vertexData[0], vertexSize, &indexData[0], indexSize, geom->GetIndexStart(), geom->GetIndexCount(), transform, color);
				}
            }

			ImGui::EndChild();
			ImGui::EndGroup();
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

void EditorWindow::DrawChild(Node* node, int& i)
{
    bool isOpen = ImGui::TreeNode((void*)(intptr_t)i, "%s - %i - %i", node->GetName().CString(), node->GetID(), node->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        selectedNode_ = node->GetID();
        if (guizmo_)
        {
            guizmo_->SetSelectedNode(selectedNode_);
            URHO3D_LOGERRORF("item selected <%i>", node->GetID());
        }
    }
    if (isOpen)
    {
        auto grantchidren = node->GetChildren();
        for (Node* grantchild : grantchidren)
            DrawChild(grantchild, i);

        ImGui::TreePop();
    }
    i++;
}

void EditorWindow::AttributeEdit(Component* c)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
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
		case VAR_COLOR:
		{
			float col[4];
			Color color = c->GetAttribute(info.name_).GetColor();
			memcpy(col, color.Data(), sizeof(col));
			ImGui::ColorEdit4(info.name_.CString(), (float*)&col);
			c->SetAttribute(info.name_, Color(col));
		}
		break;
		case VAR_VECTOR3:
		{
			float v[3];
			Vector3 value = c->GetAttribute(info.name_).GetVector3();
			memcpy(v, value.Data(), sizeof(v));
			ImGui::InputFloat3(info.name_.CString(), (float*)&v);
			c->SetAttribute(info.name_, Vector3(v));
		}
		break;
		case VAR_RESOURCEREF:
		{
			//resourcePickers.Push(ResourcePicker("Model", "*.mdl", ACTION_PICK));
			//resourcePickers.Push(ResourcePicker("Material", materialFilters, ACTION_PICK | ACTION_OPEN | ACTION_EDIT));
			//resourcePickers.Push(ResourcePicker("ParticleEffect", "*.xml", ACTION_PICK | ACTION_OPEN | ACTION_EDIT));
			//resourcePickers.Push(ResourcePicker("Animation", "*.ani", ACTION_PICK | ACTION_TEST));

			//resourcePickers.Push(ResourcePicker("Font", fontFilters));
			//resourcePickers.Push(ResourcePicker("Image", imageFilters));
			//resourcePickers.Push(ResourcePicker("LuaFile", luaFileFilters));
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
			currentModel_ = FindModel(v.name_);
			// ImGui::Text("type <%s> name <%s> currentid <%i>", info.defaultValue_.GetTypeName().CString(), v.name_.CString(), currentModel_);
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
			else if (v.type_ == StringHash("Model"))
			{
				if (ImGui::Combo("Model", &currentModel_, modelResourcesString_.CString()))
				{
					ResourceFile resource = modelResources_.Values().At(currentModel_);
					v.name_ = resource.path + resource.name;
					c->SetAttribute(info.name_, v);
				}
			}
		}
		break;
		case VAR_RESOURCEREFLIST:
		{
			ResourceRefList v = c->GetAttribute(info.name_).GetResourceRefList();
			// ImGui::Text("type <%s> name <%s>", info.defaultValue_.GetTypeName().CString(), info.name_.CString());
			currentMaterialList_.Resize(v.names_.Size());
			int *currentMaterialList = currentMaterialList_.Buffer();
			for (int i = 0; i < v.names_.Size(); i++)
			{
				if (v.type_ == StringHash("Material"))
				{
					currentMaterialList[i] = FindMaterial(v.names_.At(i));
					char comboName[10];
					sprintf(comboName, "Material%i", i);
					if (ImGui::Combo(comboName, &currentMaterialList[i], materialResourcesString_.CString()))
					{
						int pos = currentMaterialList[i];
						ResourceFile resource = materialResources_.Values().At(pos);
						v.names_[i] = resource.path + resource.name;
					}
				}
			}
			c->SetAttribute(info.name_, v);
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

int EditorWindow::FindModel(const String& name)
{
	int index = 0;
	for (auto it = modelResources_.Begin(); it != modelResources_.End(); it++, index++)
	{
		ResourceFile resource = (*it).second_;
		String s = resource.name;
		if (name == s)
			return index;
	}
	return -1;
}

int EditorWindow::FindMaterial(const String& name)
{
	int index = 0;
	for (auto it = materialResources_.Begin(); it != materialResources_.End(); it++, index++)
	{
		ResourceFile resource = (*it).second_;
		String s = resource.name;
		if (name == s)
			return index;
	}
	return -1;
}

}
