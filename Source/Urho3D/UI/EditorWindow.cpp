#include "../UI/EditorWindow.h"

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
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../Math/Polyhedron.h"
#include "../Resource/ResourceCache.h"
#include "../UI/EditorGuizmo.h"
#include "../UI/EditorModelDebug.h"

#include "imgui.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorWindow::EditorWindow(Context* context) :
    ImGuiElement(context),
    guizmo_(nullptr),
    selectedNode_(0),
    currentModel_(2),
    debugText_("")
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    FileSystem dir(context_);

    const Vector<String>& dirs = cache->GetResourceDirs();
    for (int i = 0; i < dirs.Size(); i++)
    {
        String dirPath = dirs.At(i);
        // URHO3D_LOGERRORF("dir <%s>", dirPath.CString());

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
                // URHO3D_LOGERRORF("    file <%s> dirPath <%s> path <%s> ext <%s>", file.name.CString(), file.prefix.CString(), file.path.CString(), file.ext.CString());

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

    static const char* modeTypes[] = { "Select Object", "Select Vertex" };
    ImGui::Combo("Mode", (int*)&mode_, modeTypes, IM_ARRAYSIZE(modeTypes));

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

    // FIXME? manejar esto con eventos
    int guizmoBtn = 0;
    if (guizmo_)
    {
        guizmo_->SetCurrentOperation(currentGizmoOperation);
        guizmo_->SetCurrentMode(currentGizmoMode);
        guizmo_->SetCurrentEditMode(mode_);
        guizmoBtn = guizmo_->buttons_;
    }

    ImGui::Separator();

    if(ImGui::Button("Save scene"))
    {
        String fileNameWrite = "SaveTest";
        File fileWrite(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + fileNameWrite + ".xml", FILE_WRITE);
        SharedPtr<XMLFile> xml(new XMLFile(context_));
        scene_->SaveXML(fileWrite);
    }

    // ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,80));
    // ImGui::PlotLines("Lines", );

    int i = 0;
    static int nodeClicked = -1;

    ImGui::Text("Node clicked <%i> selected <%u> guizmoBtn <%i>", nodeClicked, selectedNode_, guizmoBtn);
    ImGui::SameLine();
    if(ImGui::Button("\+Local"))
    {
        scene_->CreateChild(String::EMPTY, LOCAL);
    }
    ImGui::SameLine();
    if(ImGui::Button("\+Replicated"))
    {
        scene_->CreateChild();
    }

    ImGui::BeginGroup();
    ImGui::BeginChild("Nodes", ImVec2(ImGui::GetContentRegionAvail().x, 200.0f), true);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (nodeClicked == i)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    String sceneName = scene_->GetName();
    if (sceneName.Empty())
        sceneName = "Scene";

    bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s - %i - %i", sceneName.CString(), scene_->GetID(), scene_->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        nodeClicked = i;
        // select node
        if(mode_ == SELECT_OBJECT)
        {
            selectedNode_ = scene_->GetID();

            VariantMap eventData;
            eventData[P_EDITOR_NODE_SELECTED] = selectedNode_;

            SendEvent(E_EDITOR_NODE_SELECTED, eventData);
        }
        else
        {
            URHO3D_LOGERRORF("mode <%i>", mode_);
        }
    }
    i++;
    if (isOpen)
    {
        auto grantchidren = scene_->GetChildren();
        for (Node* grantchild : grantchidren)
            DrawChild(grantchild, i, nodeClicked);

        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::Separator();

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

            AttributeEdit(node);

//            Matrix4 identity = Matrix4::IDENTITY;
//            Matrix4 projection = camera->GetProjection().Transpose();
//            Matrix4 view = camera->GetView().ToMatrix4().Transpose();
//            Matrix4 nodeTransform = node->GetTransform().ToMatrix4().Transpose();
//            // Matrix4 nodeTransform = Matrix4::IDENTITY.Transpose();
//            Matrix4 delta;

//            const unsigned len = node->GetName().Length();
//            char name[512];
//            sprintf(name, node->GetName().CString());
//            ImGui::InputText("Name", name, node->GetName().Length());

//            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
//            ImGuizmo::DecomposeMatrixToComponents(&nodeTransform.m00_, matrixTranslation, matrixRotation, matrixScale);
//            bool modified = false;
//            ImGui::InputFloat3("Tr", matrixTranslation, 3);
//            ImGui::InputFloat3("Rt", matrixRotation, 3);
//            ImGui::InputFloat3("Sc", matrixScale, 3);
//            ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, &nodeTransform.m00_);

//            node->SetTransform(Matrix3x4(nodeTransform.Transpose()));

            // add component
            if (ImGui::Button("Add component"))
                ImGui::OpenPopup("new_component_popup");
            if (ImGui::BeginPopup("new_component_popup"))
            {
                AddComponentMenu(node);
                ImGui::EndPopup();
            }

            // components
            ImGui::BeginGroup();
            ImGui::BeginChild("Components", ImVec2(ImGui::GetContentRegionAvail().x, 200.0f), true);

            auto childComponents = node->GetComponents();
            for (Component* c : childComponents)
            {
                // ImGui::Text("%s", c->GetTypeName().CString());
                static bool headerOpen = true;
                ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader(c->GetTypeName().CString(), &headerOpen, headerFlags))
                {
                    AttributeEdit(c);

                    EditorModelDebug* modelDebug = dynamic_cast<EditorModelDebug*>(c);
                    EditModelDebug(modelDebug);
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

    ImGui::Separator();

    ImGui::Text("%s", debugText_.CString());

    ImVec2 windowPos = ImGui::GetWindowPos();
    SetPosition(windowPos.x, windowPos.y);

    ImVec2 windowSize = ImGui::GetWindowSize();
    SetSize(windowSize.x, windowSize.y);

    ImGui::End();
}

void EditorWindow::DrawChild(Node* node, int& i, int& nodeClicked)
{
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (nodeClicked == i)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s - %i - %i", node->GetName().CString(), node->GetID(), node->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        selectedNode_ = node->GetID();
        nodeClicked = i;
        if (guizmo_)
        {
            guizmo_->SetSelectedNode(selectedNode_);
            URHO3D_LOGERRORF("item selected <%i>", node->GetID());
        }
    }
    i++;
    if (isOpen)
    {
        auto grantchidren = node->GetChildren();
        for (Node* grantchild : grantchidren)
            DrawChild(grantchild, i, nodeClicked);

        ImGui::TreePop();
    }
}

void EditorWindow::AttributeEdit(Serializable* c)
{
    const Vector<AttributeInfo>* attr = c->GetAttributes();
    if(!attr)
        return;

    for (auto var : (*attr))
    {
        const AttributeInfo& info = var;
        switch (info.type_)
        {
        case VAR_BOOL:
        {
            bool v = c->GetAttribute(info.name_).GetBool();
            if (ImGui::Checkbox(info.name_.CString(), &v))
            {
                c->SetAttribute(info.name_, v);
            }
        }
        break;
        case VAR_INT:
        {
            unsigned int v = c->GetAttribute(info.name_).GetUInt();
            if (info.enumNames_)
            {
                const char** name = info.enumNames_;
                Vector<String> enumNames;
                while (*name)
                {
                    enumNames.Push(*name);
                    name++;
                }
                String enumName;
                enumName.Join(enumNames, "@");
                enumName.Replace('@', '\0');
                enumName.Append('\0');


                if (ImGui::Combo(info.name_.CString(), (int*)&v, enumName.CString()))
                {
                    // URHO3D_LOGERRORF("imgui return val <%u>", v);
                    c->SetAttribute(info.name_, v);
                }
            }
            else
            {
                if (ImGui::InputInt(info.name_.CString(), (int*)&v))
                {
                    c->SetAttribute(info.name_, v);
                }
            }
        }
        break;
        case VAR_FLOAT:
        {
            float v = c->GetAttribute(info.name_).GetFloat();
            if (ImGui::InputFloat(info.name_.CString(), &v))
            {
                c->SetAttribute(info.name_, v);
            }
        }
        break;
        case VAR_STRING:
        {
            String v = c->GetAttribute(info.name_).GetString();
            char buffer[512];
            strncpy(buffer, v.CString(), v.Length());
            buffer[v.Length()] = '\0';
            if (ImGui::InputText(info.name_.CString(), buffer, 512))
            {
                c->SetAttribute(info.name_, buffer);
            }
        }
        break;
        case VAR_COLOR:
        {
            float col[4];
            Color color = c->GetAttribute(info.name_).GetColor();
            memcpy(col, color.Data(), sizeof(col));
            if (ImGui::ColorEdit4(info.name_.CString(), (float*)&col))
            {
                c->SetAttribute(info.name_, Color(col));
            }
        }
        break;
        case VAR_VECTOR3:
        {
            float v[3];
            Vector3 value = c->GetAttribute(info.name_).GetVector3();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat3(info.name_.CString(), (float*)&v))
            {
                c->SetAttribute(info.name_, Vector3(v));
            }
        }
        break;
        case VAR_VECTOR4:
        {
            float v[4];
            Vector4 value = c->GetAttribute(info.name_).GetVector4();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat4(info.name_.CString(), (float*)&v))
            {
                c->SetAttribute(info.name_, Vector3(v));
            }
        }
        case VAR_QUATERNION:
        {
            float q[4];
            Quaternion value = c->GetAttribute(info.name_).GetQuaternion();
            memcpy(q, value.Data(), sizeof(q));
            if (ImGui::InputFloat4(info.name_.CString(), (float*)&q))
            {
                c->SetAttribute(info.name_, Quaternion(q));
            }
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
                EditParticleEmitter(emitter);
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
            bool changed = false;
            for (int i = 0; i < v.names_.Size(); i++)
            {
                if (v.type_ == StringHash("Material"))
                {
                    currentMaterialList[i] = FindMaterial(v.names_.At(i));
                    char comboName[32];
                    sprintf(comboName, "Material%i", i);
                    if (ImGui::Combo(comboName, &currentMaterialList[i], materialResourcesString_.CString()))
                    {
                        int pos = currentMaterialList[i];
                        ResourceFile resource = materialResources_.Values().At(pos);
                        v.names_[i] = resource.path + resource.name;

                        changed = true;
                    }
                }
            }
            if (changed)
            {
                c->SetAttribute(info.name_, v);
            }
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

void EditorWindow::AddComponentMenu(Node* node)
{
    const HashMap<String, Vector<StringHash> >& objectCat = context_->GetObjectCategories();
    Vector<String> components;

    auto it = objectCat.Begin();
    for(; it != objectCat.End(); it++)
    {
        if (ImGui::BeginMenu(it->first_.CString()))
        {
            const HashMap<StringHash, SharedPtr<ObjectFactory> >& factories = context_->GetObjectFactories();
            const Vector<StringHash>& factoryHashes = it->second_;
            components.Reserve(factoryHashes.Size());

            for (unsigned j = 0; j < factoryHashes.Size(); ++j)
            {
                HashMap<StringHash, SharedPtr<ObjectFactory> >::ConstIterator k = factories.Find(factoryHashes[j]);
                if (k != factories.End())
                    components.Push(k->second_->GetTypeName());
            }

            Vector<String>::ConstIterator subIt = components.Begin();
            unsigned i = 0;
            for (; subIt != components.End(); subIt++, i++)
            {
                if (ImGui::MenuItem(subIt->CString()))
                {
                    URHO3D_LOGERRORF("menu selected <%s>", subIt->CString());
                    node->CreateComponent(factoryHashes.At(i));
                }
            }
            ImGui::EndMenu();
        }
    }
}

void EditorWindow::EditParticleEmitter(ParticleEmitter* emitter)
{
    if (!emitter)
        return;

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

void EditorWindow::EditModelDebug(EditorModelDebug *modelDebug)
{
    if (!modelDebug)
        return;

    Model* model = modelDebug->GetModel();
    if (!model)
        return;

    int addIndex = -1;
    int vertexIndex = -1;
    const Vector<SharedPtr<VertexBuffer> >& vb = model->GetVertexBuffers();
    for(unsigned i = 0; i < vb.Size(); i++)
    {
        VertexBuffer* vertexBuffer = vb.At(i);
        const PODVector<VertexElement>& ves = vertexBuffer->GetElements();
        for(unsigned j = 0; j < ves.Size(); j++)
        {
            const VertexElement& ve = ves.At(j);
            ImGui::Text("ve <%i> index <%i> offset <%i> semantic <%i> type <%i>", j,
                        ve.index_, ve.offset_, ve.semantic_, ve.type_);
            ImGui::SameLine();
            char label[8];
            sprintf(label,"\+##%i", j);
            if(ImGui::Button(label))
            {
                vertexIndex = i;
                addIndex = j;
            }
        }
    }
    IntVector3 currentIndex = modelDebug->GetCurrentIndex();
    ImGui::Text("total faces <%u>", modelDebug->GetFacesCount());
    ImGui::Text("selected faces <%i>", modelDebug->GetSelectedFaces().Size());
    ImGui::Text("current face <%u> indexs <%u, %u, %u> mask <%u>", modelDebug->GetCurrentFace(), currentIndex.x_, currentIndex.y_, currentIndex.z_, modelDebug->GetCurrentMask());
    if (ImGui::Button("Save Model"))
    {
        modelDebug->SaveModel();
    }
    ImGui::SameLine();
    if (ImGui::Button("Select all"))
    {
        modelDebug->SelectAll();
    }

    if(addIndex > -1 && vertexIndex > -1)
    {
        modelDebug->AddVertexElement(vertexIndex, addIndex);
    }

/*
                StaticModel* staticModel = dynamic_cast<StaticModel*>(c);
                if (staticModel)
                {
//                    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
                    // staticModel->DrawDebugGeometry(debugRenderer, true);

                    Model* model = staticModel->GetModel();
                    ImGui::Text("geom <%i> vb <%i> ib <%i> olod <%i>", model->GetNumGeometries(),
                                model->GetVertexBuffers().Size(), model->GetIndexBuffers().Size(), staticModel->GetOcclusionLodLevel());

                    for(unsigned int i = 0; i < model->GetIndexBuffers().Size(); i++)
                    {
                        IndexBuffer* ib = model->GetIndexBuffers().At(i);
                        ImGui::Text("ib size <%i> count <%i>", ib->GetIndexSize(), ib->GetIndexCount());
                    }

//                    Matrix3x4 transform = node->GetWorldTransform();
//                    const BoundingBox& modelBoundingBox = model->GetBoundingBox();

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

//                    ImGui::Text("geom vsize <%i> isize <%i> is <%i> ic <%i>", vertexSize, indexSize, geom->GetIndexStart(), geom->GetIndexCount());
                }
                */
}

void EditorWindow::DebugModelSubPart()
{
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
