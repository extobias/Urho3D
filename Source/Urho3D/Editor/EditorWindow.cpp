#include "../Editor/EditorWindow.h"

#include "../Core/CoreEvents.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Camera.h"
#include "../Graphics/ParticleEffect.h"
#include "../Graphics/ParticleEmitter.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderPath.h"
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
#include "../Editor/EditorGuizmo.h"
#include "../Editor/EditorSelection.h"
#include "../Editor/EditorModelDebug.h"
#include "../Editor/EditorImport.h"
#include "../UI/TBElement.h"
#include "../UI/UI.h"
#include "../Urho2D/Drawable2D.h"
#include "../Urho2D/ParticleEffect2D.h"
#include "../Urho2D/ParticleEmitter2D.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "ImGuiFileBrowser.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorWindow::EditorWindow(Context* context) :
    ImGuiElement(context),
    cameraNode_(new Node(context)),
    selection_(new EditorSelection(context)),
//    debugText_(""),
    guizmo_(nullptr),
    yaw_(0.0f),
    pitch_(0.0f),
    theta_(0.0f),
    phi_(-90.0f),
    camDistance_(10.0f),
    sceneLoading_(false),
    fullscreen_(false)
{
    for (int i = 0; i < 4; i++)
    {
        plotVarsOffset_[i] = 0;
    }

    cameraNode_->SetName("EditorCamera");
    cameraNode_->SetTemporary(true);
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -1.0f));
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);
    camera->SetFarClip(10000.0f);
    // camera->SetNearClip(0.0f);

    LoadResources();

    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(EditorWindow, HandlePostRenderUpdate));

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(EditorWindow, HandleUpdate));
    
    SubscribeToEvent(E_ASYNCLOADFINISHED, URHO3D_HANDLER(EditorWindow, HandleSceneLoaded));
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorWindow>(UI_CATEGORY);
}

void EditorWindow::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    ImGuiElement::HandlePostUpdate(eventType, eventData);
}

void EditorWindow::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!visible_)
        return;

    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    timeStep_ = timeStep;

    UpdateAnimations(timeStep);

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void EditorWindow::HandleSceneLoaded(StringHash eventType, VariantMap& eventData)
{
    sceneLoading_ = false;

    if (scene_)
    {
        guizmo_->SetScene(scene_);
        selection_->SetScene(scene_);

        guizmo_->SetCameraNode(cameraNode_);
    }
}

void EditorWindow::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    // UIElement* focusElement = GetSubsystem<UI>()->GetFocusElement();
    // if (focusElement && focusElement->IsVisible())
    //     return;

    if (!cameraNode_)
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float moveSpeed = 10.0f;
    // Mouse sensitivity as degrees per pixel
    float mouseSensitivity = 0.01f;
    // wheel speed
    float wheelSpeed = 0.15f;

    if (input->GetKeyDown(KEY_SHIFT))
    {
        moveSpeed *= 10.0f;
        mouseSensitivity = 0.5f;
    }

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
        if (navMode_ == NAV_FPS)
        {
            IntVector2 mouseMove = input->GetMouseMove();

            yaw_ += (float)mouseSensitivity * mouseMove.x_;
            pitch_ += (float)mouseSensitivity * mouseMove.y_;
            pitch_ = Clamp(pitch_, -90.0f, 90.0f);

            if (input->GetKeyDown(KEY_LEFT))
                yaw_ += (float)mouseSensitivity;
            if (input->GetKeyDown(KEY_RIGHT))
                yaw_ -= (float)mouseSensitivity;


            // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
            cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
        }
        else if (navMode_ == NAV_LOOK_AT)
        {
            IntVector2 mouseMove = input->GetMouseMove();
            theta_ += (float)mouseSensitivity * mouseMove.x_;
            phi_ += (float)mouseSensitivity * mouseMove.y_;

            float x = camDistance_ * sin(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);
            float y = camDistance_ * cos(phi_ * M_DEGTORAD);
            float z = camDistance_ * cos(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);

            cameraNode_->SetPosition(Vector3(x, y, z));
            cameraNode_->LookAt(Vector3::ZERO);
        }
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

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (navMode_ == NAV_FPS)
    {
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
    }
    else if (navMode_ == NAV_LOOK_AT)
    {
        if (input->GetKeyDown(KEY_A))
            theta_ += 0.1f * moveSpeed;
        if (input->GetKeyDown(KEY_D))
            theta_ -= 0.1f * moveSpeed;
        if (input->GetKeyDown(KEY_W))
            phi_ += 0.1f * moveSpeed;
        if (input->GetKeyDown(KEY_S))
            phi_ -= 0.1f * moveSpeed;

        float x = camDistance_ * sin(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);
        float y = camDistance_ * cos(phi_ * M_DEGTORAD);
        float z = camDistance_ * cos(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);

        cameraNode_->SetPosition(Vector3(x, y, z));
        cameraNode_->LookAt(Vector3::ZERO);
    }

    // Mouse input
    Camera* camera = cameraNode_->GetComponent<Camera>();
    // wheel zoom
    int delta = input->GetMouseMoveWheel();
    if (navMode_ == NAV_FPS)
    {
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
    else if (navMode_ == NAV_LOOK_AT)
    {
        if (delta != 0)
        {
            camDistance_ = delta > 0 ? camDistance_ + wheelSpeed : camDistance_ - wheelSpeed;

            float x = camDistance_ * sin(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);
            float y = camDistance_ * cos(phi_ * M_DEGTORAD);
            float z = camDistance_ * cos(theta_ * M_DEGTORAD) * sin(phi_ * M_DEGTORAD);

            cameraNode_->SetPosition(Vector3(x, y, z));
            cameraNode_->LookAt(Vector3::ZERO);
        }
    }
}

void EditorWindow::UpdateAnimations(float timeStep)
{
    
}

void EditorWindow::ConfigResources()
{
    Vector<String> materialFilters = {"xml", "material", "json"};
    Vector<String> textureFilters = {"dds", "png", "jpg", "bmp", "tga", "ktx", "pvr", "hdr"};

    resourcesContainer_.Push(ResourceContainer("Model", "Models", "mdl"));
    resourcesContainer_.Push(ResourceContainer("Animation", "Models", "ani"));
    resourcesContainer_.Push(ResourceContainer("Material", "Materials", materialFilters));
    resourcesContainer_.Push(ResourceContainer("Sprite2D", "Urho2D"));
    resourcesContainer_.Push(ResourceContainer("Object", "Objects", "xml"));
    resourcesContainer_.Push(ResourceContainer("Scene", "Scenes", "xml"));
    resourcesContainer_.Push(ResourceContainer("ParticleEffect2D", "Urho2D", "pex"));
    resourcesContainer_.Push(ResourceContainer("AnimationSet2D", "Urho2D", "scml"));
    
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

void EditorWindow::ClearResources()
{
    resourcesContainer_.Clear();
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

            ClearResources();
            LoadResources();
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

    ResourceContainer rc;
    FindContainer("Scene", rc);
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

            ClearResources();
            LoadResources();
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

    ResourceContainer rc;
    FindContainer("Object", rc);
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

        //    URHO3D_LOGERRORF("  prefix <%s> name <%s> ext <%s>", prefix.CString(), name.CString(), ext.CString());

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

            // find in wich container goes
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
}

void EditorWindow::CreateGuizmo()
{
    guizmo_ = new EditorGuizmo(context_);
    guizmo_->SetName("guizmo");
    guizmo_->SetFocusMode(FM_NOTFOCUSABLE);
    guizmo_->SetPosition(0, 0);
    guizmo_->SetSelection(selection_);

    if (parent_)
        parent_->AddChild(guizmo_);
    // AddChild(guizmo_);

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
    plotVarsRange_[index][0] = (float)min;
    plotVarsRange_[index][1] = float(max);
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

void EditorWindow::SetCameraEnabled(bool enable)
{
    cameraNode_->SetEnabledRecursive(enable);
}

void EditorWindow::AttachCamera()
{
    // editor_->SetCameraNode(gCameraNode);
    Graphics* graphics = GetSubsystem<Graphics>();

    cameraNode_->SetParent(scene_);
    Camera* camera = cameraNode_->GetComponent<Camera>();
    if (!camera)
        return;

    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.0f);

    Renderer* renderer = GetSubsystem<Renderer>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    // effectRenderPath->Load(cache->GetResource<XMLFile>("CoreData/RenderPaths/PBRDeferred.xml"));
    // effectRenderPath->Load(cache->GetResource<XMLFile>("CoreData/RenderPaths/ForwardDepth.xml"));
    effectRenderPath->Load(cache->GetResource<XMLFile>("CoreData/RenderPaths/Forward.xml"));
    viewport->SetRenderPath(effectRenderPath);

    unsigned index = renderer->GetNumViewports();
    
    renderer->SetViewport(1, viewport);
}

void EditorWindow::Render(float timeStep)
{
    if (!scene_ || sceneLoading_)
        return;

    static bool closable = true;
    ImGui::Begin("Editor", &closable);

    // Guizmo stuff
    static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

    // selection mode
    static const char* modeTypes[] = { "Object", "Mesh Vertex", "Polygon Vertex" };

    ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::CollapsingHeader("Options", NULL, headerFlags))
    {
        // const char* hideLabel = "##hidelabel";
        ImGui::PushID("Mode");
        int total_w = (int)ImGui::GetContentRegionAvail().x;
        ImGui::Text("Mode");
        ImGui::SameLine(total_w / 2.0f);
        ImGui::SetNextItemWidth(total_w / 2.0f);

        ImGui::Combo(hideLabel_, (int*)&mode_, modeTypes, IM_ARRAYSIZE(modeTypes));
        ImGui::PopID();

        // navegation mode
        static const char* navModeTypes[] = { "FPS", "Look At" };
        // const char* hideLabel = "##hidelabel";
        ImGui::PushID("Navigation");
        total_w = (int)ImGui::GetContentRegionAvail().x;
        ImGui::Text("Navigation");
        ImGui::SameLine(total_w / 2.0f);
        ImGui::SetNextItemWidth(total_w / 2.0f);

        ImGui::Combo(hideLabel_, (int*)&navMode_, navModeTypes, IM_ARRAYSIZE(navModeTypes));
        ImGui::PopID();

        // hide ui
        ImGui::PushID("HideUI");
        total_w = (int)ImGui::GetContentRegionAvail().x;
        ImGui::Text("Hide UI");
        ImGui::SameLine(total_w / 2.0f);
        ImGui::SetNextItemWidth(total_w / 2.0f);
     
        static bool hideUI = false;
        if (ImGui::Checkbox(hideLabel_, &hideUI))
        {
            UIElement* modalElement = GetSubsystem<UI>()->GetRootModalElement();
            // PODVector<UIElement*> children;
            // modalElement->GetChildren(children, true);

            const Vector<SharedPtr<UIElement> >& children = modalElement->GetChildren();
            for (unsigned int i = 0; i < children.Size(); i++)
            {
                // UIElement* child = children.At(i);
                TBUIElement* child = dynamic_cast<TBUIElement*>(children.At(i).Get());
                if (child)
                    child->SetVisible(hideUI);
            }

            // modalElement->SetEnabledRecursive(hideUI);
        }
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

    if (ImGui::CollapsingHeader("Viewport", NULL, headerFlags))
    {
        Renderer* renderer = GetSubsystem<Renderer>();
        unsigned numViewports = renderer->GetNumViewports();
        ImGui::Text("Viewports <%d> views <%d>", numViewports, renderer->GetNumViews());
        for(unsigned i = 0; i < numViewports; i++)
        {
            Viewport* viewport = renderer->GetViewport(i);
            if (viewport)
            {
                RenderPath* renderPath = viewport->GetRenderPath();

                ImGui::Text("\tviewports <%d> renderpath <%d>", i, renderPath->GetNumRenderTargets());
            }
        }
    }

    if (ImGui::CollapsingHeader("Scene", NULL, headerFlags))
    {
        if (ImGui::Button("Play"))
            scene_->SetUpdateEnabled(true);

        ImGui::SameLine();

        if (ImGui::Button("Pause"))
            scene_->SetUpdateEnabled(false);


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
    }

    if (ImGui::CollapsingHeader("Plot", NULL, headerFlags))
    {
        // FIXME make these dinamyc
        for(int i = 0; i < 4; i++)
        {
            char buf[32];
            int curIndex = (plotVarsOffset_[i] - 1) % (int)(sizeof(plotVars_[i])/sizeof(*plotVars_[i]));
            sprintf(buf, "var <%i> <%.4f>", i, plotVars_[i][curIndex]);
    //        URHO3D_LOGERRORF("editor-render timestep <%f> value <%f>", timeStep, plotVars_[i][curIndex]);
            ImGui::PlotLines(buf, plotVars_[i], IM_ARRAYSIZE(plotVars_[i]), plotVarsOffset_[i], NULL, plotVarsRange_[i][0], plotVarsRange_[i][1], ImVec2(0, 60));
        }
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImVec2 windowPos = ImGui::GetWindowPos();
    SetPosition(windowPos.x, windowPos.y);

    if (fullscreen_)
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        // SetPosition(IntVector2::ZERO);
        SetSize(graphics->GetSize());
    }
    else
    {
        ImVec2 windowSize = ImGui::GetWindowSize();
        SetSize(windowSize.x, windowSize.y);
    }

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

    AttributeEdit(node);

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

    ImGui::SameLine();

    static imgui_addons::ImGuiFileBrowser file_dialog;

    if (ImGui::Button("Import"))
    {
        ImGui::OpenPopup("Import FBX");
        fullscreen_ = true;
    }

    ImGuiWindow* window = imguiContext_->CurrentWindow;
    const ImGuiID id = window->GetID("Import FBX");
    fullscreen_= ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);

    if (file_dialog.showFileDialog("Import FBX", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(800, 450), ".fbx"))
    {
        ImportFBX(file_dialog.selected_fn.c_str(), file_dialog.selected_path.c_str());

        ResourceCache* cache = GetSubsystem<ResourceCache>();
        cache->ReleaseAllResources(true);

        ClearResources();
        LoadResources();

        fullscreen_ = false;
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

            StaticModel* staticModel = dynamic_cast<StaticModel*>(c);
            if (staticModel)
            {
                Model* model = staticModel->GetModel();
                if (model)
                {
                    // ImGui::Text("model geom <%i>", model->GetNumGeometries());
                    for (unsigned ii = 0; ii < model->GetNumGeometries(); ii++)
                    {
                        unsigned lod = model->GetNumGeometryLodLevels(ii);
                        ImGui::Text("model geom <%i> lod <%i>", ii, lod);
                        for (unsigned jj = 0; jj < lod; jj++)
                        {
                            Geometry* geo = model->GetGeometry(ii, jj);
                        }
                    }
                }
            }
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

        int width = (int)ImGui::GetContentRegionAvail().x;
        ImGui::Text("%s", info.name_.CString());
        // ImGui::SameLine(width / 2.0f);

        Variant attVariant = c->GetAttribute(info.name_);

        if(VariantEdit(info, info.name_, attVariant))
            c->SetAttribute(info.name_, attVariant);
    }
}
// FIXME sometimes info.name isnt name
bool EditorWindow::VariantEdit(const AttributeInfo& info, const String& name, Variant& value, unsigned index)
{
    const char* hideLabel = "##hidelabel";

    char id[512];
    memset(id, 0, 512);
    sprintf(id, "%s_%i", name.CString(), index);

    float total_w = ImGui::GetContentRegionAvail().x;
    float halfWidth = total_w / 2.0f;
    
    ImGui::SetNextItemWidth(halfWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(halfWidth);

    bool ret = false;
    switch (value.GetType())
    {
    case VAR_BOOL:
    {
        ImGui::PushID(name.CString());
        bool v = value.GetBool();
        if (ImGui::Checkbox(hideLabel, &v))
        {
            value = v;
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_INT:
    {
        ImGui::PushID(name.CString());
        unsigned int v = value.GetUInt();
        if (info.enumNames_)
        {
            String enumName = ComboNames(info.enumNames_);
            if (ImGui::Combo(hideLabel, (int*)&v, enumName.CString()))
            {
                value = v;
                ret = true;
            }
        }
        else
        {
            if (ImGui::InputInt(hideLabel, (int*)&v))
            {
                value = v;
                ret = true;
            }
        }
        ImGui::PopID();
    }
    break;
    case VAR_FLOAT:
    {
        ImGui::PushID(name.CString());
        float v = value.GetFloat();
        if (ImGui::InputFloat(hideLabel, &v))
        {
            value = v;
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_STRING:
    {
        ImGui::PushID(name.CString());
        String v = value.GetString();
        char buffer[512];
        strncpy(buffer, v.CString(), v.Length());
        buffer[v.Length()] = '\0';
        if (ImGui::InputText(hideLabel, buffer, 512))
        {
            value = buffer;
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_COLOR:
    {
        ImGui::PushID(name.CString());
        float col[4];
        Color color = value.GetColor();
        memcpy(col, color.Data(), sizeof(col));
        if (ImGui::ColorEdit4(hideLabel, (float*)&col))
        {
            value = Color(col);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_VECTOR2:
    {
        ImGui::PushID(name.CString());
        float v[2];
        Vector2 v2= value.GetVector2();
        memcpy(v, v2.Data(), sizeof(v));
        if (ImGui::InputFloat2(hideLabel, (float*)&v))
        {
            value = Vector2(v);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_VECTOR3:
    {
        ImGui::PushID(name.CString());
        float v[3];
        Vector3 v3 = value.GetVector3();
        memcpy(v, v3.Data(), sizeof(v));
        if (ImGui::InputFloat3(hideLabel, (float*)&v))
        {
            value = Vector3(v);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_VECTOR4:
    {
        ImGui::PushID(name.CString());
        float v[4];
        Vector4 v4 = value.GetVector4();
        memcpy(v, v4.Data(), sizeof(v));
        if (ImGui::InputFloat4(hideLabel, (float*)&v))
        {
            value = Vector4(v);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_RECT:
    {
        ImGui::PushID(name.CString());
        float v[4];
        Rect rect = value.GetRect();
        memcpy(v, rect.Data(), sizeof(v));
        if (ImGui::InputFloat4(hideLabel, (float*)&v))
        {
            value = Rect(v);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_QUATERNION:
    {
        ImGui::PushID(name.CString());
        float q[4];
        Quaternion quat = value.GetQuaternion();
        memcpy(q, quat.Data(), sizeof(q));
        if (ImGui::InputFloat4(hideLabel, (float*)&q))
        {
            value = Quaternion(q);
            ret = true;
        }
        ImGui::PopID();
    }
    break;
    case VAR_STRINGVECTOR:
    {
        ImGui::PushID(name.CString());
        StringVector tags = value.GetStringVector();

        static char buf[32] = "";
        ImGui::SetNextItemWidth(halfWidth / 2.0f);
        ImGui::InputText("##edit", buf, IM_ARRAYSIZE(buf));

        ImGui::SetNextItemWidth(halfWidth / 4.0f);
        ImGui::SameLine();
        if(ImGui::Button("Add"))
        {
            tags.Push(buf);
            value = tags;
            ret = true;
        }

        ImGui::SetNextItemWidth(halfWidth / 4.0f);
        ImGui::SameLine();
        if(ImGui::Button("Del"))
        {
            tags.Remove(String(buf));
            value = tags;
            ret = true;
        }

        for (unsigned i = 0; i < tags.Size(); i++)
        {
            ImGui::SetCursorPosX(halfWidth);
            ImGui::SetNextItemWidth(halfWidth);

            ImGui::Text("<%s>", tags.At(i).CString());
        }
        ImGui::PopID();
    }
    break;
    case VAR_BUFFER:
    {
        ImGui::PushID(name.CString());
        if (info.name_ == "Vertices")
        {
            const PODVector<unsigned char> bufferVec = value.GetBuffer();
            PODVector<Vector2> vertices;

            // load
            MemoryBuffer buffer(bufferVec);
            while (!buffer.IsEof())
                vertices.Push(buffer.ReadVector2());

            ImGui::Text("Vertices: size <%i>", vertices.Size());
            ImGui::SameLine();

            if (vertices.Size() < 8)
            {
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
                    // FIXME save buffer
                    VectorBuffer newVertices;
                    for (unsigned i = 0; i < vertices.Size(); ++i)
                        newVertices.WriteVector2(vertices[i]);

                    value = newVertices.GetBuffer();
                    ret = true;
                }
            }
            else
            {
                ImGui::NewLine();
            }

            bool modified = false;
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
                VectorBuffer newVertices;
                for (unsigned i = 0; i < vertices.Size(); ++i)
                    newVertices.WriteVector2(vertices[i]);

                value = newVertices;
                ret = true;
            }
        }
        ImGui::PopID();
    }
    break;
    case VAR_RESOURCEREF:
    {
        ImGui::PushID(name.CString());
        ResourceRef resRef = value.GetResourceRef();
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        if (resRef.type_ == StringHash("ParticleEffect"))
        {
            ImGui::SetNextItemWidth(halfWidth);
            // ParticleEmitter* emitter = dynamic_cast<ParticleEmitter*>(c);
            // EditParticleEmitter(emitter);
        }
        if (resRef.type_ == StringHash("ParticleEffect2D") || resRef.type_ == StringHash("Model") || resRef.type_ == StringHash("Material") || resRef.type_ == StringHash("XMLFile") || resRef.type_ == StringHash("AnimationSet2D"))
        {
            ImGui::SetNextItemWidth(halfWidth);
            ResourceContainer rc;
            FindContainer(resRef.type_, rc);
            int index = rc.FindResourceIndex(resRef.name_);
            if (ImGui::Combo(hideLabel, &index, rc.resourcesString_.CString()))
            {
                    ResourceFile resource = rc.resources_.Values().At(index);
                    resRef.name_ = resource.path + resource.name;

                    value = resRef;
                    ret = true;
            }

            if (resRef.type_ == StringHash("ParticleEffect2D"))
            {
                // ParticleEmitter2D* emitter = dynamic_cast<ParticleEmitter2D*>(c);
                // EditParticleEmitter2D(emitter);
            }
        }
        else if (resRef.type_ == StringHash("Sprite2D"))
        {
            ImGui::SetNextItemWidth(halfWidth);
            ResourceContainer rc;
            FindContainer(resRef.type_, rc);

//                static int lines = 1;
//                ImGui::SliderInt("Lines", &lines, 1, 15);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
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
                // SharedPtr<Texture2D> texture1(cache->GetResource<Texture2D>(resource.path + resource.name));
                Texture2D* texture1 = cache->GetResource<Texture2D>(resource.path + resource.name);

                int frame_padding = -1;     // -1 = uses default padding
                // float texWidth = (float)texture1->GetWidth();
                // float texHeight = (float)texture1->GetHeight();
                ImVec4 bg_col(0.0f, 0.0f, 0.0f, 1.0f);
                ImVec4 tint_col(0.0f, 1.0f, 0.0f, 1.0f);

                if (resource.name == resRef.name_)
                    bg_col.x = 1.0f;

                // ImGui::Text("name <%s>", resource.name.CString());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
                if (ImGui::ImageButton((ImTextureID)texture1, ImVec2(btnWidth, btnHeight), ImVec2(0,0), ImVec2(1, 1), frame_padding, bg_col))
                {
                    resRef.name_ = resource.path + resource.name;
                    ret = true;
                    // c->SetAttribute(info.name_, v);
                }
                ImGui::PopStyleColor(1);

                // delete texture1;

                ImGui::SameLine();
                ImGui::PopID();

                i++;
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
        }
        ImGui::PopID();
    }
    break;
    case VAR_RESOURCEREFLIST:
    {
        ResourceRefList v = value.GetResourceRefList();
        for (unsigned int i = 0; i < v.names_.Size(); i++)
        {
            if (v.type_ == StringHash("Material"))
            {
                ImGui::PushID((name + v.names_.At(i)).CString());
                ImGui::SetCursorPosX(halfWidth);
                ImGui::SetNextItemWidth(halfWidth);

                ResourceContainer rc;
                FindContainer(v.type_, rc);

                int index = rc.FindResourceIndex(v.names_.At(i));
                if (ImGui::Combo(hideLabel_, &index, rc.resourcesString_.CString()))
                {
                    ResourceFile resource = rc.resources_.Values().At(index);
                    v.names_[i] = resource.path + resource.name;

                    value = v;
                    ret = true;
                }
                ImGui::PopID();
            }
        }
        if (v.names_.Empty())
        {
            ImGui::NewLine();
        }
    }
    break;
    case VAR_VARIANTMAP:
    {
        ImGui::PushID(name.CString());
        VariantMap* map = value.GetVariantMapPtr();
        if (map)
        {
            static char buf[32] = "";
            ImGui::SetNextItemWidth(halfWidth / 3.0f);
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

            ImGui::SetNextItemWidth(halfWidth / 3.0f);
            ImGui::SameLine();
            static int variantSelected = 0;
            if (ImGui::Combo(hideLabel, &variantSelected, variantName.CString()))
            {
            }

            ImGui::SetNextItemWidth(halfWidth / 3.0f);
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

                // FIXME
                scene_->RegisterVar(buf);
                (*map)[StringHash(buf)] = v;

                ret = true;
            }

            ImGui::SetNextItemWidth(halfWidth / 3.0f);
            ImGui::SameLine();
            if(ImGui::Button("Del"))
            {
                map->Erase(StringHash(buf));
                ret = true;
            }

            Vector<StringHash> keys = map->Keys();
            for (unsigned i = 0; i < keys.Size(); i++)
            {
                ImGui::NewLine();

                Variant v = (*map)[keys.At(i)];
                String vname = scene_->GetVarName(keys.At(i));
                if (VariantEdit(info, vname, v))
                {
                    (*map)[keys.At(i)] = v;
                    ret = true;
                }
            }
        }
        ImGui::PopID();
    }
    break;
    case VAR_VARIANTVECTOR:
    {
        ImGui::PushID(name.CString());

        const VariantVector& v = value.GetVariantVector();
        VariantVector* vv = value.GetVariantVectorPtr();
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        
        if (!vv)
        {
            ImGui::PopID();
            return false;
        }

        StringVector names = info.metadata_["VectorStructElements"]->GetStringVector();
        unsigned size = 0;
        float width = total_w / 2.0f;
        if (name == "Animation States" && vv->Size())
        {
            static char buf[32] = "";

            ImGui::SetNextItemWidth(halfWidth);
            Variant sizev = (*vv).At(0);
            if (VariantEdit(info, names[0], sizev))
            {
                (*vv)[0] = sizev;
                ret = true;
            }

            size = sizev.GetInt();
        }

        (*vv).Reserve(size * 6 + 1);
        for (unsigned i = 1; i < (*vv).Size(); i += 6)
        {
            for (unsigned j = 0; j < 6; j++)
            {
                ImGui::SetCursorPosX(width);
                ImGui::SetNextItemWidth(width / 2.0f);

                Variant value = (*vv).At(i + j);
                if (VariantEdit(info, names[j + 1], value, i + j))
                {
                    (*vv)[i + j] = value;
                    ret = true;
                }
            }

            ImGui::SetCursorPosX(width);
            ImGui::SetNextItemWidth(width / 2.0f);
            if(ImGui::Button("Play"))
            {
                Variant value = (*vv).At(i);
                ResourceRef res = value.GetResourceRef();
                URHO3D_LOGERRORF("varianvector anim name <%s> type <%s>", res.name_.CString(), value.GetTypeName().CString());

                // Animation* anim = cache->GetResource<Animation>(res.name_);
                // AnimatedModel* model = static_cast<AnimatedModel*>(c);
                // if (model)
                // {
                    // AnimationState* state = model->AddAnimationState(anim);
                    // state_ = model->AddAnimationState(anim);
                    // if (state)
                    // {
                        // state->SetWeight(1.0f);
                        // state->SetLooped(true);
                        // state->AddTime(timeStep_);
                    // }
                // }
            }
        }
        ImGui::PopID();
    }
    break;
    default:
    {
        // ImGui::Text("type <%s> name <%s>", info.defaultValue_.GetTypeName().CString(), info.name_.CString());
    }
    break;
    }

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
                    Component* component = node->CreateComponent(factoryHashes.At(i));
                    component->ApplyAttributes();
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

    if (!effect)
        return;

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
    if (EditAttribute("Tangential Accel Variance", tangentialAccelerationVariance, total_w))
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
    ImGui::PushID("Emitter Type");
    ImGui::Text("%s", "Emitter Type");
    ImGui::SameLine(total_w / 2.0f);
    ImGui::SetNextItemWidth(total_w / 2.0f);

    const char* emitterTypes[] = { "Gravity", "Radial" };
    int emitterType = effect->GetEmitterType();
    if (ImGui::Combo(hideLabel_, &emitterType, emitterTypes, IM_ARRAYSIZE(emitterTypes)))
    {
        effect->SetEmitterType((EmitterType2D)emitterType);
    }
    ImGui::PopID();
    

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

bool EditorWindow::ImportFBX(const String& name, const String& path)
{
    EditorImport import(context_);
    return import.ImportModel(path);
}

void EditorWindow::DebugModelSubPart()
{
}

bool EditorWindow::FindContainer(const StringHash& type, ResourceContainer& container)
{
    for(ResourceContainer& rc: resourcesContainer_)
    {
        if (rc.type_ == type)
        {
            container = rc;
            return true;
        }
    }

    return false;
}

String EditorWindow::ComboNames(const char** names)
{
    Vector<String> enumNames;
    while (*names)
    {
        enumNames.Push(*names);
        names++;
    }

    String enumName;
    enumName.Join(enumNames, "@");
    enumName.Replace('@', '\0');
    enumName.Append('\0');

    return enumName;
}

}
