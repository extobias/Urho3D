#include "../UI/EditorWindow.h"

#include "../Core/CoreEvents.h"
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
#include "../Input/Input.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../IO/MemoryBuffer.h"
#include "../Math/Polyhedron.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/SceneEvents.h"
#include "../UI/EditorGuizmo.h"
#include "../UI/EditorSelection.h"
#include "../UI/EditorModelDebug.h"
#include "../UI/UI.h"
#include "../Urho2D/Drawable2D.h"
#include "../Urho2D/ParticleEffect2D.h"
#include "../Urho2D/ParticleEmitter2D.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorWindow::EditorWindow(Context* context) :
    ImGuiElement(context),
    cameraNode_(new Node(context)),
    selection_(new EditorSelection(context)),
//    debugText_(""),
    guizmo_(nullptr),
    currentModel_(0),
    currentSprite_(0),
    yaw_(0.0f),
    pitch_(0.0f),
    sceneLoading_(false)
{
    for (int i = 0; i < 4; i++)
    {
        plotVarsOffset_[i] = 0;
    }

    cameraNode_->SetName("EditorCamera");
    cameraNode_->SetTemporary(true);
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    // LoadResources();

    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(EditorWindow, HandleUpdate));

    SubscribeToEvent(E_ASYNCLOADFINISHED, URHO3D_HANDLER(EditorWindow, HandleSceneLoaded));
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorWindow>(UI_CATEGORY);
}

//void EditorWindow::SetCameraNode(Node* node)
//{
//    cameraNode_ = node;

//    if (guizmo_)
//        guizmo_->SetCameraNode(node);
//}

void EditorWindow::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    ImGuiElement::HandlePostUpdate(eventType, eventData);

    if (!visible_)
        return;

    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void EditorWindow::HandleSceneLoaded(StringHash eventType, VariantMap& eventData)
{
    sceneLoading_ = false;
}

void EditorWindow::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    UIElement* focusElement = GetSubsystem<UI>()->GetFocusElement();
    if (focusElement && focusElement->IsVisible())
        return;

    if (!cameraNode_)
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float moveSpeed = 10.0f;
    // Mouse sensitivity as degrees per pixel
    const float mouseSensitivity = 0.1f;
    // wheel speed
    float wheelSpeed = 0.15f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
        IntVector2 mouseMove = input->GetMouseMove();

        yaw_ += (float)mouseSensitivity * mouseMove.x_;
        pitch_ += (float)mouseSensitivity * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }
    else
    {
//        Quaternion camRot = cameraNode_->GetRotation();
//        pitch_ = camRot.PitchAngle();
//        yaw_ = camRot.YawAngle();
//        cameraDistance_ += input->GetMouseMoveWheel();
//        Vector3 lookAt = cameraNode_->GetDirection(); // pos - rot * Vector3(0.0f, 0.0f, -3.0f);
//        Quaternion dir(yaw_, Vector3::UP);
//        dir = dir * Quaternion(pitch_, Vector3::RIGHT);
//        Vector3 cameraTargetPos = lookAt - dir * Vector3(0.0f, 0.0f, cameraDistance_);
//        cameraNode_->SetPosition(cameraTargetPos);
    }

    if (input->GetKeyDown(KEY_SHIFT))
    {
        moveSpeed *= 10.0f;
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * moveSpeed * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * moveSpeed * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * moveSpeed * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * moveSpeed * timeStep);
    if (input->GetKeyDown(KEY_E))
        cameraNode_->Translate(Vector3::UP * moveSpeed * timeStep);
    if (input->GetKeyDown(KEY_Q))
        cameraNode_->Translate(Vector3::DOWN * moveSpeed * timeStep);

    // Mouse input
    Camera* camera = cameraNode_->GetComponent<Camera>();
    // wheel zoom
    int delta = input->GetMouseMoveWheel();
    if (input->GetKeyDown(KEY_CTRL))
    {
        if (delta > 0)
        {
            camera->SetZoom(camera->GetZoom() * (1.0f + wheelSpeed));
        }

        if (delta < 0)
        {
            camera->SetZoom(camera->GetZoom() * (1.0f - wheelSpeed));
        }
    }
    else
    {
        if (input->GetKeyDown(KEY_SHIFT))
        {
            if (delta > 0)
                cameraNode_->Translate(Vector3::LEFT * moveSpeed * timeStep);
            else if (delta < 0)
                cameraNode_->Translate(Vector3::RIGHT * moveSpeed * timeStep);
        }
        else
        {
            if (delta > 0)
                cameraNode_->Translate(Vector3::UP * moveSpeed * timeStep);
            else if (delta < 0)
                cameraNode_->Translate(Vector3::DOWN * moveSpeed * timeStep);
        }
    }

}

void EditorWindow::ConfigResources()
{
    Vector<String> materialFilters = {"xml", "material", "json"};
    Vector<String> textureFilters = {"dds", "png", "jpg", "bmp", "tga", "ktx", "pvr", "hdr"};

    resourcesContainer_.Push(ResourceContainer("Model", "Models", "mdl"));
    resourcesContainer_.Push(ResourceContainer("Material", "Materials", materialFilters));
    resourcesContainer_.Push(ResourceContainer("Sprite2D", "Urho2D"));
    resourcesContainer_.Push(ResourceContainer("Object", "Objects", "xml"));
    resourcesContainer_.Push(ResourceContainer("Scene", "Scenes", "xml"));

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
}

bool EditorWindow::SaveScene()
{
    const char* hideLabel = "##hidelabel";

    ImGui::PushID("save_scene");
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("Save scene");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 4.0f);

    static char buf[32] = "";
    ImGui::InputText("##edit", buf, IM_ARRAYSIZE(buf));
    ImGui::SameLine();
    bool ret = false;
    if (ImGui::Button("Save"))
    {
        String fileName(buf);
        if (fileName.Length())
        {
            fileName += ".xml";
            File fileWrite(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + fileName, FILE_WRITE);
            SharedPtr<XMLFile> xml(new XMLFile(context_));
            ret = scene_->SaveXML(fileWrite);
        }
    }

    ImGui::PopID();

    return ret;

}

bool EditorWindow::LoadScene()
{
    if (!resourcesContainer_.Size())
        return false;
        
    const char* hideLabel = "##hidelabel";

    ImGui::PushID("load_scene");
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("Scene");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 2.0f);

    ResourceContainer& rc = FindContainer("Scene");
    int ci = rc.currentIndex_;

    bool ret = false;
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (ImGui::Combo(hideLabel, &ci, rc.resourcesString_.CString()))
    {
        ResourceFile resource = rc.resources_.Values().At(ci);
        SharedPtr<File> file = cache->GetFile(resource.path + resource.name);

        // cameraNode_ = nullptr;
        selection_->Clear();

        if (scene_->LoadAsyncXML(file))
        {
            sceneLoading_ = true;
            rc.currentIndex_ = ci;
            ret = true;
        }
    }
    ImGui::PopID();

    return ret;
}

bool EditorWindow::SavePrefab(Node* node)
{
//    if (ImGui::Button("Save prefab"))
//    {
//        // node->SavePrefab(true);
//        if ()
//        {
//            ImGui::BeginTooltip();
//            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//            ImGui::TextUnformatted("Saved ok!");
//            ImGui::PopTextWrapPos();
//            ImGui::EndTooltip();
//        }
//        else
//        {
//            ImGui::BeginTooltip();
//            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//            ImGui::TextUnformatted("Cannot save!");
//            ImGui::PopTextWrapPos();
//            ImGui::EndTooltip();
//        }
//    }

    const char* hideLabel = "##hidelabel";

    ImGui::PushID("save_object");
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("Save object");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 4.0f);

    static char buf[32] = "";
    ImGui::InputText("##edit", buf, IM_ARRAYSIZE(buf));
    ImGui::SameLine();
    bool ret = false;
    if (ImGui::Button("Save"))
    {
        String fileName(buf);// = "NodeTest.xml";
        if (fileName.Length())
        {
            fileName += ".xml";
            File fileWrite(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Objects/" + fileName, FILE_WRITE);
            SharedPtr<XMLFile> xml(new XMLFile(context_));
            ret = node->SaveXML(fileWrite);
        }
    }

    ImGui::PopID();

    return ret;
}

bool EditorWindow::LoadPrefab(Node *node)
{
    if (!resourcesContainer_.Size())
        return false;

    const char* hideLabel = "##hidelabel";

    ImGui::PushID("load_object");
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("Object");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 2.0f);

    ResourceContainer& rc = FindContainer("Object");
    int ci = rc.currentIndex_;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (ImGui::Combo(hideLabel, &ci, rc.resourcesString_.CString()))
    {
        ResourceFile resource = rc.resources_.Values().At(ci);
        rc.currentIndex_ = ci;
        // URHO3D_LOGERRORF("EditorWindow: prefab <%s>", resource.name.CString());
        XMLFile* file = cache->GetResource<XMLFile>(resource.path + resource.name);

        if (file != nullptr)
        {
            if (node->LoadXML(file->GetRoot()))
            {
                rc.currentIndex_ = -1;
                URHO3D_LOGERRORF("node loaded!");
            }
            else
                URHO3D_LOGERRORF("cant load node!");
        }
    }
    ImGui::PopID();

    return true;
}

void EditorWindow::LoadResources()
{
    ConfigResources();

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    FileSystem dir(context_);

    // load all resources dirs
    const Vector<String>& dirs = cache->GetResourceDirs();
    for (unsigned i = 0; i < dirs.Size(); i++)
    {
        String dirPath = dirs.At(i);
//        URHO3D_LOGERRORF("dir <%s>", dirPath.CString());

        Vector<String> result;
        dir.ScanDir(result, dirPath, "*.*", SCAN_FILES, true);
        for (unsigned j = 0; j < result.Size(); j++)
        {
            String prefix, name, ext;
            SplitPath(result.At(j), prefix, name, ext);

//            URHO3D_LOGERRORF("  prefix <%s> name <%s> ext <%s>", prefix.CString(), name.CString(), ext.CString());

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

            // find on wich container goes
            for(ResourceContainer& rc: resourcesContainer_)
            {
                // URHO3D_LOGERRORF("Container Name: <%s> filename <%s> ext <%s> contains <%i>", rc.name_.CString(), resFile.name.CString(), resFile.ext.CString(), rc.extFilter_.Contains(resFile.ext));
                if (!rc.dirFilter_.Empty())
                {
                    if (rc.extFilter_.Size())
                    {
                        if (resFile.prefix.Contains(rc.dirFilter_, false) && rc.extFilter_.Contains(resFile.ext))
                        {
                            rc.resources_[resFile.name] = resFile;
                        }
                    }
                    else
                    {
                        if (resFile.prefix.Contains(rc.dirFilter_, false))
                        {
                            rc.resources_[resFile.name] = resFile;
                        }
                    }
                }
                else
                {
                    if (rc.extFilter_.Contains(resFile.ext))
                    {
                        rc.resources_[resFile.name] = resFile;
                    }
                }
            }
        }
    }

    for(ResourceContainer& rc: resourcesContainer_)
    {
        StringVector keys = rc.resources_.Keys();
        // URHO3D_LOGERRORF("Container Name: <%s> Size <%i>", rc.name_.CString(), rc.resources_.Size());

        for(String key: keys)
        {
            ResourceFile rf = rc.resources_[key];
            // URHO3D_LOGERRORF("  resource: key <%s> file <%s>", key.CString(), rf.name.CString());
        }

        rc.resourcesString_.Join(rc.resources_.Keys(), "@");
        rc.resourcesString_.Replace('@', '\0');
        rc.resourcesString_.Append('\0');
    }

    // Categorize them
    StringVector keys = resources_.Keys();
    for (unsigned i = 0; i < keys.Size(); i++)
    {
        Vector<ResourceFile> list = resources_[keys.At(i)];
//        URHO3D_LOGERRORF("mapkey <%s> size <%i>", keys.At(i).CString(), list.Size());

        for (unsigned l = 0; l < list.Size(); l++)
        {
            ResourceFile file = list.At(l);
            String modelsFilter("mdl");
//            URHO3D_LOGERRORF("    file <%s> dirPath <%s> path <%s> ext <%s>", file.name.CString(), file.prefix.CString(), file.path.CString(), file.ext.CString());

            if (file.ext == modelsFilter)
            {
                modelResources_[file.name] = file;
            }
            else if (file.prefix.Contains("materials", false) && (file.ext == "xml" || file.ext == "material" || file.ext == "json"))
            {
                materialResources_[file.name] = file;
            }
            else if (file.prefix.Contains("urho2d", false) && (file.ext == "png"))
            {
                spriteResources_[file.name] = file;
            }
        }
    }

    modelResourcesString_.Join(modelResources_.Keys(), "@");
    modelResourcesString_.Replace('@', '\0');
    modelResourcesString_.Append('\0');

    materialResourcesString_.Join(materialResources_.Keys(), "@");
    materialResourcesString_.Replace('@', '\0');
    materialResourcesString_.Append('\0');

    spriteResourcesString_.Join(spriteResources_.Keys(), "@");
    spriteResourcesString_.Replace('@', '\0');
    spriteResourcesString_.Append('\0');
}

void EditorWindow::CreateGuizmo()
{
    guizmo_ = new EditorGuizmo(context_);
    guizmo_->SetName("guizmo");
    guizmo_->SetFocusMode(FM_NOTFOCUSABLE);
    guizmo_->SetPosition(0, 0);
    guizmo_->SetEditorSelection(selection_);

    if (parent_)
        parent_->AddChild(guizmo_);

    if (scene_)
    {
        guizmo_->SetScene(scene_);
        selection_->SetScene(scene_);
    }

    if (cameraNode_)
        guizmo_->SetCameraNode(cameraNode_);
}

void EditorWindow::SetVisible(bool visible)
{
    UIElement::SetVisible(visible);

    if (guizmo_)
        guizmo_->SetVisible(visible);
}

void EditorWindow::SetPlotVar(int index, float value, float min, float max)
{
    plotVars_[index][plotVarsOffset_[index]] = value;
    plotVarsOffset_[index] = (plotVarsOffset_[index] + 1) % (int)(sizeof(plotVars_[index])/sizeof(*plotVars_[index]));
    plotVarsRange_[index][0] = min;
    plotVarsRange_[index][1] = max;
}

void EditorWindow::SetScene(Scene* scene)
{
    scene_ = scene;

    selection_->SetScene(scene);

    if (guizmo_)
    {
        guizmo_->SetScene(scene);
    }

    AttachCamera();
}

void EditorWindow::SetOrthographic(bool orthographic)
{
    Camera* camera = cameraNode_->GetComponent<Camera>();
    camera->SetOrthographic(orthographic);
}

void EditorWindow::SetCameraPosition(const Vector3& position)
{
    cameraNode_->SetPosition(position);
}

void EditorWindow::SetCameraRotation(const Quaternion& rotation)
{
    cameraNode_->SetRotation(rotation);
}

void EditorWindow::AttachCamera()
{
    // editor_->SetCameraNode(gCameraNode);
    Graphics* graphics = GetSubsystem<Graphics>();

    cameraNode_->SetParent(scene_);
    Camera* camera = cameraNode_->GetComponent<Camera>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.0f);

    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
    unsigned index = renderer->GetNumViewports();
    
    renderer->SetViewport(1, viewport);
}

void EditorWindow::Render(float timeStep)
{
    if (!scene_ || sceneLoading_)
        return;

    static bool closable = true;
    ImGui::Begin("Scene Inspector", &closable);

    // Guizmo stuff
    static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

    static const char* modeTypes[] = { "Object", "Mesh Vertex", "Polygon Vertex" };

    // selection mode
    const char* hideLabel = "##hidelabel";
    ImGui::PushID("Mode");
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("Mode");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 2.0f);

    ImGui::Combo(hideLabel, (int*)&mode_, modeTypes, IM_ARRAYSIZE(modeTypes));
    ImGui::PopID();

    // current operation
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

    SaveScene();

    if (LoadScene())
    {
        guizmo_->SetCameraNode(nullptr);
        ImGui::End();
        return;
    }

//    if(ImGui::Button("Save scene"))
//    {
//        String fileNameWrite = "SaveTest";
//        File fileWrite(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + fileNameWrite + ".xml", FILE_WRITE);
//        SharedPtr<XMLFile> xml(new XMLFile(context_));
//        scene_->SaveXML(fileWrite);
//    }

    // ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,80));
    // ImGui::PlotLines("Lines", );
    const Vector<SharedPtr<Node> >& s = selection_->GetSelectedNodes();

    ImGui::Text("Node selected <%s> guizmoBtn <%i>", selection_->ToString().CString(), guizmoBtn);
    // ImGui::SameLine();
    if(ImGui::Button("+Local"))
    {
        if (s.Size() == 1)
        {
            Node* node = s.At(0);
            if (node)
            {
                node->CreateChild(String::EMPTY, LOCAL);
            }
        }
        else
        {
            scene_->CreateChild(String::EMPTY, LOCAL);
        }

    }
    ImGui::SameLine();
    if(ImGui::Button("+Replicated"))
    {
        if (s.Size() == 1)
        {
            Node* node = s.At(0);
            if (node)
            {
                node->CreateChild(String::EMPTY);
            }
        }
        else
        {
            scene_->CreateChild(String::EMPTY);
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("-Remove"))
    {
        for(unsigned i = 0; i < selection_->GetSelectedNodes().Size(); i++)
        {
            Node* node = selection_->GetSelectedNodes().At(i);
            node->Remove();
        }
        selection_->Clear();
    }

    DrawNodeTree();

    ImGui::Separator();

    DrawNodeSelected();

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();

    if (ImGui::Button("Play"))
        scene_->SetUpdateEnabled(true);

    ImGui::SameLine();

    if (ImGui::Button("Pause"))
        scene_->SetUpdateEnabled(false);

    ImGui::Separator();

    // FIXME make these dinamyc
    for(int i = 0; i < 4; i++)
    {
        char buf[32];
        int curIndex = (plotVarsOffset_[i] - 1) % (int)(sizeof(plotVars_[i])/sizeof(*plotVars_[i]));
        sprintf(buf, "var <%i> <%.4f>", i, plotVars_[i][curIndex]);
//        URHO3D_LOGERRORF("editor-render timestep <%f> value <%f>", timeStep, plotVars_[i][curIndex]);
        ImGui::PlotLines(buf, plotVars_[i], IM_ARRAYSIZE(plotVars_[i]), plotVarsOffset_[i], NULL, plotVarsRange_[i][0], plotVarsRange_[i][1], ImVec2(0, 60));
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    SetPosition(windowPos.x, windowPos.y);

    ImVec2 windowSize = ImGui::GetWindowSize();
    SetSize(windowSize.x, windowSize.y);

    ImGui::End();
}

void EditorWindow::DrawChild(Node* node, int& i)
{
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    const Vector<SharedPtr<Node> >& s = selection_->GetSelectedNodes();
    // selected node
    // if (selectedNode_ == node->GetID())
    if (s.Contains(SharedPtr<Node>(node)))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    // open if child is selected
    auto children = node->GetChildren();
    for (Node* child : children)
    {
        if (s.Size() == 1 && s.Contains(SharedPtr<Node>(child)))
        {
            nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
            break;
        }
    }

    bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s - %i - %i", node->GetName().CString(), node->GetID(), node->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        selection_->Add(node);
    }
    i++;
    if (isOpen)
    {
        auto children = node->GetChildren();
        for (Node* child : children)
            DrawChild(child, i);

        ImGui::TreePop();
    }
}

void EditorWindow::DrawNodeTree()
{
    int i = 0;

    ImGui::BeginGroup();
    ImGui::BeginChild("Nodes", ImVec2(ImGui::GetContentRegionAvail().x, 200.0f), true);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

    const Vector<SharedPtr<Node> >& s = selection_->GetSelectedNodes();
    if (s.Size() == 1 && s.Contains(SharedPtr<Node>(scene_)))
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    String sceneName = scene_->GetName();
    if (sceneName.Empty())
        sceneName = "Scene";

    bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s - ID %i - Size %i", sceneName.CString(), scene_->GetID(), scene_->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        // select scene
        if(mode_ == SELECT_OBJECT)
        {
            selection_->Add(scene_);
        }
        else
        {
            URHO3D_LOGERRORF("EditorWindow: mode <%i>", mode_);
        }
    }
    i++;
    if (isOpen)
    {
        auto grantchidren = scene_->GetChildren();
        for (Node* grantchild : grantchidren)
            DrawChild(grantchild, i);

        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void EditorWindow::DrawNodeSelected()
{
    const Vector<SharedPtr<Node> >& s = selection_->GetSelectedNodes();
    if (s.Size() != 1)
        return;

    Node* node = s.At(0);
    if (!node)
    {
        URHO3D_LOGERRORF("EditorWindow: node <%p> not found!", node);
        return;
    }

//    Node* cameraNode = cameraNode_;
//    if(!cameraNode)
//        return;

//    Camera* camera = cameraNode->GetComponent<Camera>();
//    if(!camera)
//        return;

    AttributeEdit(node);

//    Variant vars = node->GetAttribute("Variables");
//    VariantMap* map = vars.GetVariantMapPtr();
//    Vector<StringHash> keys = map->Keys();
//    ImGui::Text("keys size <%i> map size <%i>", keys.Size(), map->Size());
//    for(unsigned i = 0; i < map->Size(); i++)
//    {
//        ImGui::Text(" <%i> isize <%i> is <%i> ic <%i>", vertexSize, indexSize, geom->GetIndexStart(), geom->GetIndexCount());
//    }

    // prefabs
    SavePrefab(node);

    // ImGui::SameLine();
    LoadPrefab(node);

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
    ImGui::BeginChild("Components", ImVec2(ImGui::GetContentRegionAvail().x, 400.0f), true);

    // static bool headerOpen = true;
    auto childComponents = node->GetComponents();
    // FIXME
    static bool headerOpen[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    unsigned compCount = 0;
    for (Component* c : childComponents)
    {
        String label = c->GetTypeName();
        label.AppendWithFormat("-%i", compCount);

        ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::CollapsingHeader(label.CString(), &headerOpen[compCount], headerFlags))
        {
            AttributeEdit(c);

            EditorModelDebug* modelDebug = dynamic_cast<EditorModelDebug*>(c);
            EditModelDebug(modelDebug);
        }
        else
        {
            // component removed
            if (!headerOpen[compCount])
            {
                headerOpen[compCount] = true;
//                    childComponents.Erase(compCount);
                node->RemoveComponent(c);
            }
//                for(unsigned i = 0; i < 10; i++)
//                    URHO3D_LOGERRORF("EditorWindow: headerOpen <%i>", headerOpen[i]);
        }

        compCount++;
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

void EditorWindow::AttributeEdit(Serializable* c)
{
    const Vector<AttributeInfo>* attr = c->GetAttributes();
    if(!attr)
        return;

    const char* hideLabel = "##hidelabel";
    for (auto var : (*attr))
    {
        const AttributeInfo& info = var;

        if (info.mode_ & AM_NOEDIT)
            continue;

        ImGui::PushID(info.name_.CString());
        int total_w = (int)ImGui::GetContentRegionAvail().x;
        ImGui::Text("%s", info.name_.CString());
        ImGui::SameLine(total_w / 2.0f);

        switch (info.type_)
        {
        case VAR_BOOL:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            bool v = c->GetAttribute(info.name_).GetBool();
            if (ImGui::Checkbox(hideLabel, &v))
            {
                c->SetAttribute(info.name_, v);
            }
        }
        break;
        case VAR_INT:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
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

                if (ImGui::Combo(hideLabel, (int*)&v, enumName.CString()))
                {
                    c->SetAttribute(info.name_, v);
                }
            }
            else
            {
                if (ImGui::InputInt(hideLabel, (int*)&v))
                {
                    c->SetAttribute(info.name_, v);
                }
            }
        }
        break;
        case VAR_FLOAT:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float v = c->GetAttribute(info.name_).GetFloat();
            if (ImGui::InputFloat(hideLabel, &v))
            {
                c->SetAttribute(info.name_, v);
            }
        }
        break;
        case VAR_STRING:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            String v = c->GetAttribute(info.name_).GetString();
            char buffer[512];
            strncpy(buffer, v.CString(), v.Length());
            buffer[v.Length()] = '\0';
            if (ImGui::InputText(hideLabel, buffer, 512))
            {
                c->SetAttribute(info.name_, buffer);
            }
        }
        break;
        case VAR_COLOR:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float col[4];
            Color color = c->GetAttribute(info.name_).GetColor();
            memcpy(col, color.Data(), sizeof(col));
            if (ImGui::ColorEdit4(hideLabel, (float*)&col))
            {
                c->SetAttribute(info.name_, Color(col));
            }
        }
        break;
        case VAR_VECTOR2:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float v[2];
            Vector2 value = c->GetAttribute(info.name_).GetVector2();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat2(hideLabel, (float*)&v))
            {
                c->SetAttribute(info.name_, Vector2(v));
            }
        }
        break;
        case VAR_VECTOR3:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float v[3];
            Vector3 value = c->GetAttribute(info.name_).GetVector3();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat3(hideLabel, (float*)&v))
            {
                c->SetAttribute(info.name_, Vector3(v));
            }
        }
        break;
        case VAR_VECTOR4:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float v[4];
            Vector4 value = c->GetAttribute(info.name_).GetVector4();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat4(hideLabel, (float*)&v))
            {
                c->SetAttribute(info.name_, Vector4(v));
            }
        }
        break;
        case VAR_RECT:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float v[4];
            Rect value = c->GetAttribute(info.name_).GetRect();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat4(hideLabel, (float*)&v))
            {
                c->SetAttribute(info.name_, Rect(v));
            }
        }
        break;
        case VAR_QUATERNION:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            float q[4];
            Quaternion value = c->GetAttribute(info.name_).GetQuaternion();
            memcpy(q, value.Data(), sizeof(q));
            if (ImGui::InputFloat4(hideLabel, (float*)&q))
            {
                c->SetAttribute(info.name_, Quaternion(q));
            }
        }
        break;
        case VAR_BUFFER:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
            if (info.name_ == "Vertices")
            {
                const PODVector<unsigned char> value = c->GetAttribute(info.name_).GetBuffer();
                PODVector<Vector2> vertices;

                // load
                MemoryBuffer buffer(value);
                while (!buffer.IsEof())
                    vertices.Push(buffer.ReadVector2());

                ImGui::Text("Vertices: size <%i>", vertices.Size());
                ImGui::SameLine();
                if(ImGui::Button("Add"))
                {
                    if (vertices.Size() == 0)
                    {
                        vertices.Push(Vector2(-0.1f, 0.0f));
                        vertices.Push(Vector2(0.1f, 0.0f));
                        vertices.Push(Vector2(0.0f, 0.1f));
                    }
                    else
                    {
                        vertices.Push(Vector2::ZERO);
                    }
                    // save buffer
                    VectorBuffer ret;
                    for (unsigned i = 0; i < vertices.Size(); ++i)
                    {
//                        URHO3D_LOGERRORF("EditorWindow: add v2 <%f, %f>", vertices[i].x_, vertices[i].y_);
                        ret.WriteVector2(vertices[i]);
                    }

                    Variant val(ret.GetBuffer());
                    c->SetAttribute(info.name_, val);
                }

                bool modified = false;
                // for(Vector2 v2: vertices)
                for (unsigned i = 0; i < vertices.Size(); ++i)
                {
                    ///URHO3D_LOGERRORF("EditorWindow: 1 v2 <%f, %f>", v2.x_, v2.y_);
                    float v[2];// = { 0.0f, 0.0f };
                    memcpy(v, vertices[i].Data(), sizeof(v));

                    char buf[4];
                    sprintf(buf, "v%i", i);
                    if (ImGui::InputFloat2(buf, (float*)&v))
                    {
                        modified = true;
                        vertices[i] = Vector2(v);
                    }
                }

                // save
                if (modified)
                {
                    VectorBuffer ret;
                    for (unsigned i = 0; i < vertices.Size(); ++i)
                        ret.WriteVector2(vertices[i]);

                    c->SetAttribute(info.name_, ret.GetBuffer());
                }
            }
        }
        break;
        case VAR_RESOURCEREF:
        {
            ResourceRef v = c->GetAttribute(info.name_).GetResourceRef();
            ResourceCache* cache = GetSubsystem<ResourceCache>();

            // ImGui::Text("type <%s> name <%s> currentid <%i>", info.defaultValue_.GetTypeName().CString(), v.name_.CString(), currentModel_);

            if (v.type_ == StringHash("ParticleEffect"))
            {
                ImGui::SetNextItemWidth(total_w / 2.0f);
                ParticleEmitter* emitter = dynamic_cast<ParticleEmitter*>(c);
                EditParticleEmitter(emitter);
            }
            if (v.type_ == StringHash("ParticleEffect2D"))
            {
                ParticleEmitter2D* emitter = dynamic_cast<ParticleEmitter2D*>(c);

                if (ImGui::Button("Save Effect"))
                {
                    ParticleEffect2D* effect = emitter->GetEffect();

                    String fileName = GetSubsystem<FileSystem>()->GetProgramDir() + "Data/" + effect->GetName();
                    File file(context_, fileName, FILE_WRITE);
                    effect->Save(file);
                    URHO3D_LOGERRORF("Save effect name <%s>", effect->GetName().CString());
                }

                EditParticleEmitter2D(emitter);
            }
            else if (v.type_ == StringHash("Model"))
            {
                ImGui::SetNextItemWidth(total_w / 2.0f);
                currentModel_ = FindModel(v.name_);
                if (ImGui::Combo(hideLabel, &currentModel_, modelResourcesString_.CString()))
                {
                    ResourceFile resource = modelResources_.Values().At(currentModel_);
                    v.name_ = resource.path + resource.name;
                    c->SetAttribute(info.name_, v);
                }
            }
            else if (v.type_ == StringHash("Sprite2D"))
            {
                ImGui::SetNextItemWidth(total_w / 2.0f);
                const ResourceContainer& rc = FindContainer(v.type_);

//                static int lines = 1;
//                ImGui::SliderInt("Lines", &lines, 1, 15);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                ImGui::BeginChild("scrolling", ImVec2(0, ImGui::GetFrameHeightWithSpacing() + 50), true, ImGuiWindowFlags_HorizontalScrollbar);

                float btnWidth = 32.0f;
                float btnHeight = 32.0f;
                unsigned i = 0;
                Vector<ResourceFile> values = rc.resources_.Values();
                for (ResourceFile& resource: values)
                {
                    // const ResourceFile& resource = .At(i);
                    if (resource.ext == "scml" || resource.ext == "pex" || resource.ext == "txt" || resource.ext == "tmx" || resource.ext == "xml")
                        continue;

                    ImGui::PushID(i);
                    Texture2D* texture1 = cache->GetResource<Texture2D>(resource.path + resource.name);
//                    ImGui::Image((ImTextureID)texture1, ImVec2((float)texture1->GetWidth(), (float)texture1->GetHeight()));
                    int frame_padding = -1;     // -1 = uses default padding
                    float texWidth = (float)texture1->GetWidth();
                    float texHeight = (float)texture1->GetHeight();
                    if (ImGui::ImageButton((ImTextureID)texture1, ImVec2(btnWidth, btnHeight), ImVec2(0,0), ImVec2(1, 1))) //, frame_padding, ImVec4(0.0f,0.0f,0.0f,1.0f)))
                    {
                        v.name_ = resource.path + resource.name;
                        c->SetAttribute(info.name_, v);
                    }
                    ImGui::SameLine();
                    ImGui::PopID();

                    i++;
                }

                ImGui::EndChild();
                ImGui::PopStyleVar(2);
            }
            else if (v.type_ == StringHash("XMLFile"))
            {
                ImGui::SetNextItemWidth(total_w / 2.0f);
                currentModel_ = FindModel(v.name_);
                if (ImGui::Combo(hideLabel, &currentModel_, modelResourcesString_.CString()))
                {
                    ResourceFile resource = modelResources_.Values().At(currentModel_);
                    v.name_ = resource.path + resource.name;
                    c->SetAttribute(info.name_, v);
                }
            }
            else if (v.type_ == StringHash("Material"))
            {
                ImGui::SetNextItemWidth(total_w / 2.0f);
                 const ResourceContainer& rc = FindContainer(v.type_);
                 int index = rc.FindResourceIndex(v.name_);
                 if (ImGui::Combo(hideLabel, &index, rc.resourcesString_.CString()))
                 {
                     ResourceFile resource = rc.resources_.Values().At(index);
                     v.name_ = resource.path + resource.name;
                     c->SetAttribute(info.name_, v);
                 }
            }
        }
        break;
        case VAR_RESOURCEREFLIST:
        {
            ImGui::SetNextItemWidth(total_w / 2.0f);
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
        case VAR_VARIANTMAP:
        {
            float width = total_w / 2.0f;
            Variant vars = c->GetAttribute(info.name_);
            VariantMap* map = vars.GetVariantMapPtr();
            if (map)
            {
                static char buf[32] = "";
                ImGui::SetNextItemWidth(width / 3.0f);
                ImGui::InputText("##edit", buf, IM_ARRAYSIZE(buf));

                Vector<String> variantTypes;
                variantTypes.Push("int");
                variantTypes.Push("bool");
                variantTypes.Push("float");
                variantTypes.Push("String");
                variantTypes.Push("Vector3");
                variantTypes.Push("Color");

                String variantName;
                variantName.Join(variantTypes, "@");
                variantName.Replace('@', '\0');
                variantName.Append('\0');

                ImGui::SetNextItemWidth(width / 3.0f);
                ImGui::SameLine();
                static int variantSelected = 0;
                if (ImGui::Combo(hideLabel, &variantSelected, variantName.CString()))
                {
                }

                ImGui::SetNextItemWidth(width / 3.0f);
                ImGui::SameLine();
                if(ImGui::Button("Add"))
                {
                    // int, bool, float, String, Vector3, Color
                    Variant v;
                    if (variantSelected == 0)
                    {
                        v = 0;
                    }
                    else if (variantSelected == 1)
                    {
                        v = false;
                    }
                    else if (variantSelected == 2)
                    {
                        v = 0.0f;
                    }
                    else if (variantSelected == 3)
                    {
                        v = String("");
                    }
                    else if (variantSelected == 4)
                    {
                        v = Vector3::ZERO;
                    }
                    else if (variantSelected == 5)
                    {
                        v = Color::WHITE;
                    }

//                    map->Insert(Pair<StringHash, Variant>(StringHash(buf), v));
                    // FIXME
                    scene_->RegisterVar(buf);
                    (*map)[StringHash(buf)] = v;

                    c->SetAttribute(info.name_, vars);
                }

                ImGui::SetNextItemWidth(width / 3.0f);
                ImGui::SameLine();
                if(ImGui::Button("Del"))
                {
                    map->Erase(StringHash(buf));
                    c->SetAttribute(info.name_, vars);
                }

                Vector<StringHash> keys = map->Keys();
                for (unsigned i = 0; i < keys.Size(); i++)
                {
//                    ImGui::Text("variant <%s> type <%s>", keys.At(i).Reverse().CString(), (*map)[keys.At(i)].GetTypeName().CString());
                    ImGui::SetCursorPosX(width);
                    ImGui::SetNextItemWidth(width / 2.0f);

                    Variant v = (*map)[keys.At(i)];
                    if (VariantEdit(keys.At(i), v))
                    {
                        (*map)[keys.At(i)] = v;
                        c->SetAttribute(info.name_, vars);
                    }
                }
            }
        }
        break;
        default:
        {
            ImGui::Text("type <%s> name <%s>", info.defaultValue_.GetTypeName().CString(), info.name_.CString());
        }
        break;
        }
        ImGui::PopID();
    }
}

bool EditorWindow::VariantEdit(const StringHash& key, Variant& v)
{
    const String& name = scene_->GetVarName(key);
    const char* hideLabel = "##hidelabel";

    ImGui::PushID(name.CString());
    int total_w = (int)ImGui::GetContentRegionAvail().x;
    ImGui::Text("%s", name.CString());
    ImGui::SetNextItemWidth(total_w / 2.0f);
    ImGui::SameLine();

    // nombre
    bool ret = false;
    switch (v.GetType())
    {
        case VAR_BOOL:
        {
            bool val = v.GetBool();
            if (ImGui::Checkbox(hideLabel, &val))
            {
                v = val;
                ret = true;
            }
        }
        break;
        case VAR_INT:
        {
            unsigned int val = v.GetUInt();
            if (ImGui::InputInt(hideLabel, (int*)&val))
            {
                v = val;
                ret = true;
            }
        }
        break;
        case VAR_FLOAT:
        {
            float val = v.GetFloat();
            if (ImGui::InputFloat(hideLabel, &val))
            {
                v = val;
                ret = true;
            }
        }
        break;
        case VAR_VECTOR3:
        {
            float val[3];
            Vector3 value = v.GetVector3();
            memcpy(val, value.Data(), sizeof(v));
            if (ImGui::InputFloat3(hideLabel, (float*)&val))
            {
                v = Vector3(val);
                ret = true;
            }
        }
        break;
        case VAR_STRING:
        {
            String val = v.GetString();
            char buffer[512];
            strncpy(buffer, val.CString(), val.Length());
            buffer[val.Length()] = '\0';
            if (ImGui::InputText(hideLabel, buffer, 512))
            {
                v = buffer;
                ret = true;
            }
        }
        break;
    }
    ImGui::PopID();

    return ret;
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

void EditorWindow::EditParticleEmitter2D(ParticleEmitter2D* emitter)
{
    if (!emitter)
        return;

    ParticleEffect2D* effect = emitter->GetEffect();

//    ImGui::NewLine();
    int total_w = (int)ImGui::GetContentRegionAvail().x;

    Vector2 sourcePositionVariance = effect->GetSourcePositionVariance();
    if (EditAttribute("Source Position Variance", sourcePositionVariance, total_w))
    {
        effect->SetSourcePositionVariance(sourcePositionVariance);
    }

    float speed = effect->GetSpeed();
    if (EditAttribute("Speed", speed, total_w))
    {
        effect->SetSpeed(speed);
    }

    float speedVariance = effect->GetSpeedVariance();
    if (EditAttribute("SpeedVariance", speedVariance, total_w))
    {
        effect->SetSpeedVariance(speedVariance);
    }

    float particleLifeSpan = effect->GetParticleLifeSpan();
    if (EditAttribute("Particle Life Span", particleLifeSpan, total_w))
    {
        effect->SetParticleLifeSpan(particleLifeSpan);
    }

    float particleLifeSpanVariance = effect->GetParticleLifespanVariance();
    if (EditAttribute("Particle Life Span Variance", particleLifeSpanVariance, total_w))
    {
        effect->SetParticleLifespanVariance(particleLifeSpanVariance);
    }

    float angle = effect->GetAngle();
    if (EditAttribute("Angle", angle, total_w))
    {
        effect->SetAngle(angle);
    }

    float angleVariance = effect->GetAngleVariance();
    if (EditAttribute("Angle Variance", angleVariance, total_w))
    {
        effect->SetAngleVariance(angleVariance);
    }

    Vector2 gravity = effect->GetGravity();
    if (EditAttribute("Gravity", gravity, total_w))
    {
        effect->SetGravity(gravity);
    }

    float radialAcceleration = effect->GetRadialAcceleration();
    if (EditAttribute("Radial Accel", radialAcceleration, total_w))
    {
        effect->SetRadialAcceleration(radialAcceleration);
    }

    float radialAccelerationVariance = effect->GetRadialAccelVariance();
    if (EditAttribute("Radial Accel Variance", radialAccelerationVariance, total_w))
    {
        effect->SetRadialAccelVariance(radialAccelerationVariance);
    }

    float tangentialAcceleration = effect->GetTangentialAcceleration();
    if (EditAttribute("Tangential Accel", tangentialAcceleration, total_w))
    {
        effect->SetTangentialAcceleration(tangentialAcceleration);
    }

    float tangentialAccelerationVariance = effect->GetTangentialAccelVariance();
    if (EditAttribute("tangential Accel Variance", tangentialAccelerationVariance, total_w))
    {
        effect->SetTangentialAccelVariance(tangentialAccelerationVariance);
    }

    Color startColor = effect->GetStartColor();
    if (EditAttribute("Start Color", startColor, total_w))
    {
        effect->SetStartColor(startColor);
    }

    Color startColorVariance = effect->GetStartColorVariance();
    if (EditAttribute("Start Color Variance", startColorVariance, total_w))
    {
        effect->SetStartColorVariance(startColorVariance);
    }

    Color finishColor = effect->GetFinishColor();
    if (EditAttribute("Finish Color", finishColor, total_w))
    {
        effect->SetFinishColor(finishColor);
    }

    Color finishColorVariance = effect->GetFinishColorVariance();
    if (EditAttribute("Finish Color Variance", finishColorVariance, total_w))
    {
        effect->SetFinishColorVariance(finishColorVariance);
    }

    int maxParticles = effect->GetMaxParticles();
    if (EditAttribute("Max Particles", maxParticles, total_w))
    {
        effect->SetMaxParticles(maxParticles);
    }

    float startParticleSize = effect->GetStartParticleSize();
    if (EditAttribute("Start Particle Size", startParticleSize, total_w))
    {
        effect->SetStartParticleSize(startParticleSize);
    }

    float startParticleSizeVariance = effect->GetStartParticleSizeVariance();
    if (EditAttribute("Start Particle Size Variance", startParticleSizeVariance, total_w))
    {
        effect->SetStartParticleSizeVariance(startParticleSizeVariance);
    }

    float finishParticleSize = effect->GetFinishParticleSize();
    if (EditAttribute("Finish Particle Size", finishParticleSize, total_w))
    {
        effect->SetFinishParticleSize(finishParticleSize);
    }

    float finishParticleSizeVariance = effect->GetFinishParticleSizeVariance();
    if (EditAttribute("Finish Particle Size Variance", finishParticleSizeVariance, total_w))
    {
        effect->SetFinishParticleSizeVariance(finishParticleSizeVariance);
    }

    float duration = effect->GetDuration();
    if (EditAttribute("Duration", duration, total_w))
    {
        effect->SetDuration(duration);
    }

    // emitter type

    float maxRadius = effect->GetMaxRadius();
    if (EditAttribute("Max Radius", maxRadius, total_w))
    {
        effect->SetMaxRadius(maxRadius);
    }

    float maxRadiusVariance = effect->GetMaxRadiusVariance();
    if (EditAttribute("Max Radius Variance", maxRadiusVariance, total_w))
    {
        effect->SetMaxRadiusVariance(maxRadiusVariance);
    }

    float minRadius = effect->GetMinRadius();
    if (EditAttribute("Min Radius", minRadius, total_w))
    {
        effect->SetMinRadius(minRadius);
    }

    float minRadiusVariance = effect->GetMinRadiusVariance();
    if (EditAttribute("Min Radius Variance", minRadiusVariance, total_w))
    {
        effect->SetMinRadiusVariance(minRadiusVariance);
    }

    float rotatePerSecond = effect->GetRotatePerSecond();
    if (EditAttribute("Rotate per Second", rotatePerSecond, total_w))
    {
        effect->SetRotatePerSecond(rotatePerSecond);
    }

    float rotatePerSecondVariance = effect->GetRotatePerSecondVariance();
    if (EditAttribute("Rotate per Second Variance", rotatePerSecondVariance, total_w))
    {
        effect->SetRotatePerSecondVariance(rotatePerSecondVariance);
    }

    // blendFuncSource/blendFuncDestination

    float rotationStart = effect->GetRotationStart();
    if (EditAttribute("Rotation Start", rotationStart, total_w))
    {
        effect->SetRotationStart(rotationStart);
    }

    float rotationStartVariance = effect->GetRotationStartVariance();
    if (EditAttribute("Rotation Start Variance", rotationStartVariance, total_w))
    {
        effect->SetRotationStartVariance(rotationStartVariance);
    }

    float rotationEnd = effect->GetRotationEnd();
    if (EditAttribute("Rotation End", rotationEnd, total_w))
    {
        effect->SetRotationEnd(rotationEnd);
    }

    float rotationEndVariance = effect->GetRotationEndVariance();
    if (EditAttribute("Rotation End Variance", rotationEndVariance, total_w))
    {
        effect->SetRotationEndVariance(rotationEndVariance);
    }
}

bool EditorWindow::EditAttribute(const String& name, int& value, int totalWidth)
{
    ImGui::PushID(name.CString());
    ImGui::Text("%s", name.CString());
    ImGui::SameLine(totalWidth / 2.0f);
    ImGui::SetNextItemWidth(totalWidth / 2.0f);
    bool edited = ImGui::InputInt(hideLabel_, &value);
    ImGui::PopID();

    return edited;
}

bool EditorWindow::EditAttribute(const String& name, float& value, int totalWidth)
{
    ImGui::PushID(name.CString());
    ImGui::Text("%s", name.CString());
    ImGui::SameLine(totalWidth / 2.0f);
    ImGui::SetNextItemWidth(totalWidth / 2.0f);
    bool edited = ImGui::InputFloat(hideLabel_, &value);
    ImGui::PopID();

    return edited;
}

bool EditorWindow::EditAttribute(const String& name, Vector2& value, int totalWidth)
{
    float arrayValue[2];
    memcpy(arrayValue, value.Data(), sizeof(arrayValue));

    ImGui::PushID(name.CString());
    ImGui::Text("%s", name.CString());
    ImGui::SameLine(totalWidth / 2.0f);
    ImGui::SetNextItemWidth(totalWidth / 2.0f);
    bool edited = ImGui::InputFloat2(hideLabel_, arrayValue);
    value = Vector2(arrayValue);
    ImGui::PopID();

    return edited;
}

bool EditorWindow::EditAttribute(const String& name, Vector3& value, int totalWidth)
{
    float arrayValue[3];
    memcpy(arrayValue, value.Data(), sizeof(arrayValue));

    ImGui::PushID(name.CString());
    ImGui::Text("%s", name.CString());
    ImGui::SameLine(totalWidth / 2.0f);
    ImGui::SetNextItemWidth(totalWidth / 2.0f);
    bool edited = ImGui::InputFloat3(hideLabel_, arrayValue);
    value = Vector3(arrayValue);
    ImGui::PopID();

    return edited;
}

bool EditorWindow::EditAttribute(const String& name, Color& value, int totalWidth)
{
    float arrayValue[4];
    memcpy(arrayValue, value.Data(), sizeof(arrayValue));

    ImGui::PushID(name.CString());
    ImGui::Text("%s", name.CString());
    ImGui::SameLine(totalWidth / 2.0f);
    ImGui::SetNextItemWidth(totalWidth / 2.0f);
    bool edited = ImGui::ColorEdit4(hideLabel_, (float*)&arrayValue);
    value = Color(arrayValue);
    ImGui::PopID();

    return edited;
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

int EditorWindow::FindSprite(const String& name)
{
    int index = 0;
    for (auto it = spriteResources_.Begin(); it != spriteResources_.End(); it++, index++)
    {
        ResourceFile resource = (*it).second_;
        String s = resource.name;
        if (name == s)
            return index;
    }
    return -1;
}

ResourceContainer& EditorWindow::FindContainer(const StringHash& type)
{
    for(ResourceContainer& rc: resourcesContainer_)
    {
        if (rc.type_ == type)
            return rc;
    }
}

}
