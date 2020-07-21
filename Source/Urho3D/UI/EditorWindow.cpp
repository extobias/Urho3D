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
#include "../UI/EditorGuizmo.h"
#include "../UI/UI.h"
#include "../Urho2D/StaticSprite2D.h"

#include "../UI/EditorModelDebug.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

EditorSelection::EditorSelection(Context* context)
    : Object (context)
{
}

EditorSelection::~EditorSelection() = default;

void EditorSelection::Add(unsigned id)
{
    Input* input = GetSubsystem<Input>();
    if (!input->GetKeyDown(KEY_SHIFT))
        selectedNodes_.Clear();

    selectedNodes_.Push(id);

    UpdateTransform();
}

void EditorSelection::Clear()
{
    selectedNodes_.Clear();
}

String EditorSelection::ToString()
{
    char buf[512];
    memset(buf, 0, 512);
    unsigned offset = 0;
    for(unsigned i: selectedNodes_)
    {
        sprintf(buf + offset, "%i, ", i);
        offset = strlen(buf);
    }

    return buf;
}

void EditorSelection::SetTransform(const Matrix3x4 &matrix)
{
    transform_ = matrix;
}

void EditorSelection::SetDelta(const Matrix3x4 &matrix)
{
    for(unsigned id: selectedNodes_)
    {
        Node* node = scene_->GetNode(id);
        node->Translate(matrix.Translation());
        node->Rotate(matrix.Rotation());
        node->Scale(matrix.Scale());
    }
}

void EditorSelection::Render()
{
    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
    for(unsigned id: selectedNodes_)
    {
        Node* node = scene_->GetNode(id);
        StaticSprite2D* draw = node->GetComponent<StaticSprite2D>();
        if (draw)
        {
            debugRenderer->AddBoundingBox(draw->GetBoundingBox(), node->GetTransform(), Color::YELLOW);
        }
    }

//        if (poligonPoints.Size() > 1)
//        {
//            float f = 1.0f / poligonPoints.Size();
//            Vector3 a = poligonPoints.At(0);
//            for (unsigned i = 1; i < poligonPoints.Size(); i++)
//            {
//                Vector3 p = poligonPoints.At(i);
//                float hue = i * f;
//                Color color;
//                color.FromHSV(hue, 1.0f, 1.0f, 1.0f);
//                debugRenderer->AddLine(a , p, color);
//                a = p;
//            }
//        }
}

void EditorSelection::UpdateTransform()
{
    if (selectedNodes_.Size() == 1)
    {
        Node* node = scene_->GetNode(selectedNodes_.At(0));
        transform_ = node->GetTransform();
        return;
    }

    Vector<Vector3> poligonPoints;
    PoligonPoints(poligonPoints);
    Vector3 center = CalculateCentroid(poligonPoints);
    transform_.SetTranslation(center);
}

bool EditorSelection::PointAboveLine(Vector3 point, Vector3 p1, Vector3 p2)
{
    // first, horizontally sort the points in the line
    Vector3 first;
    Vector3 second;

    if (p1.x_ > p2.x_)
    {
        first = p2;
        second = p1;
    }
    else
    {
        first = p1;
        second = p2;
    }

    Vector3 v1 = second - first;
    Vector3 v2 = second - point;
    Vector3 cp = v1.CrossProduct(v2);

    // above or on the line
    return (cp.z_ >= 0.0f);
}

void EditorSelection::PoligonPoints(Vector<Vector3>& points)
{
    if (selectedNodes_.Size() == 0)
        return;

    if (selectedNodes_.Size() == 1)
    {
        Node* nodeSelected = scene_->GetNode(selectedNodes_.At(0));
        points.Push(nodeSelected->GetPosition());

        return;
    }

    Vector3 p, q;
    p.x_ = M_INFINITY;
    q.x_ = -M_INFINITY;
    unsigned pi = 0;
    unsigned qi = 0;
    Vector<Vector3> a, b;
    for(unsigned i = 0; i < selectedNodes_.Size(); i++)
    {
//            URHO3D_LOGERRORF("EditorGuizmo: selected <%i>", selectedNodes_.At(i));
        Node* nodeSelected = scene_->GetNode(selectedNodes_.At(i));
        Vector3 pos = nodeSelected->GetPosition();
        if (pos.x_ < p.x_)
        {
            p = pos;
            pi = i;
        }
        if (pos.x_ > q.x_)
        {
            q = pos;
            qi = i;
        }
    }

    for(unsigned i = 0; i < selectedNodes_.Size(); i++)
    {
        Node* nodeSelected = scene_->GetNode(selectedNodes_.At(i));
        Vector3 point = nodeSelected->GetPosition();
        if (i == pi || i == qi)
            continue;

        if (PointAboveLine(point, p, q))
        {
            b.Push(point);
        }
        else
        {
            a.Push(point);
        }
    }

    Sort(a.Begin(), a.End(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.x_ < rhs.x_; });

    Sort(b.Begin(), b.End(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.x_ > rhs.x_; });

    points.Push(p);
    if (a.Size())
        points.Push(a);

    points.Push(q);
    if (b.Size())
        points.Push(b);
}

Vector3 EditorSelection::CalculateCentroid(const Vector<Vector3>& points)
{
    if (points.Size() < 2)
        return Vector3::ZERO;

    if (points.Size() == 2)
    {
        Vector3 v0 = points.At(0);
        Vector3 v1 = points.At(1);

        return (v0 + v1) / 2.0f;
    }

    Vector3 centroid;
    float signedArea = 0.0;
    Vector3 v0, v1;
    float a = 0.0;  // Partial signed area

    // For all vertices except last
    for (unsigned i=0; i < points.Size() - 1; ++i)
    {
        v0 = points.At(i);
        v1 = points.At(i + 1);
        Vector3 v2 = v0.CrossProduct(v1);
        a = v2.z_;
        signedArea += a;
        centroid.x_ += (v0.x_ + v1.x_) * a;
        centroid.y_ += (v0.y_ + v1.y_) * a;
    }

    // Do last vertex separately to avoid performing an expensive
    // modulus operation in each iteration.
    v0 = points.At(points.Size() - 1);
    v1 = points.At(0);
    Vector3 v2 = v0.CrossProduct(v1);

    a = v2.z_;
    signedArea += a;
    centroid.x_ += (v0.x_ + v1.x_) * a;
    centroid.y_ += (v0.y_ + v1.y_) * a;

    signedArea *= 0.5f;
    centroid.x_ /= (6.0f * signedArea);
    centroid.y_ /= (6.0f * signedArea);

    return centroid;
}



/// EditorWindow
EditorWindow::EditorWindow(Context* context) :
    ImGuiElement(context),
    selection_(new EditorSelection(context)),
//    debugText_(""),
    cameraNode_(nullptr),
    guizmo_(nullptr),
    currentModel_(0),
    currentSprite_(0),
    yaw_(0.0f),
    pitch_(0.0f)
{
//    for (int i = 0; i < 4; i++)
//    {
//        plotVarsOffset_[i] = 0;
//    }



    LoadResources();

//    SubscribeToEvent(E_GUIZMO_NODE_SELECTED, URHO3D_HANDLER(EditorWindow, HandleNodeSelected));

    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(EditorWindow, HandleUpdate));
}

EditorWindow::~EditorWindow() = default;

void EditorWindow::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorWindow>(UI_CATEGORY);
}

void EditorWindow::SetCameraNode(Node* node)
{
    cameraNode_ = node;

    if (guizmo_)
        guizmo_->SetCameraNode(node);
}

void EditorWindow::HandleNodeSelected(StringHash eventType, VariantMap& eventData)
{
//    selectedNode_ = eventData[P_GUIZMO_NODE_SELECTED].GetUInt();
//    selectedSubElementIndex_ = eventData[P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX].GetUInt();
//    hitPosition_ = eventData[P_GUIZMO_NODE_SELECTED_POSITION].GetVector3();
}

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

void EditorWindow::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    UIElement* focusElement = GetSubsystem<UI>()->GetFocusElement();
    if (focusElement)
        return;

    if (!cameraNode_)
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float MOVE_SPEED = 1.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
        IntVector2 mouseMove = input->GetMouseMove();

        yaw_ += (float)MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += (float)MOUSE_SENSITIVITY * mouseMove.y_;
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
        MOVE_SPEED *= 50.0f;
    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_E))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_Q))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void EditorWindow::LoadResources()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    FileSystem dir(context_);

    // load all resources dirs
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

    // Categorize them
    StringVector keys = resources_.Keys();
    for (int i = 0; i < keys.Size(); i++)
    {
        Vector<ResourceFile> list = resources_[keys.At(i)];
        // URHO3D_LOGERRORF("mapkey <%s> size <%i>", keys.At(i).CString(), list.Size());
        for (unsigned l = 0; l < list.Size(); l++)
        {
            ResourceFile file = list.At(l);
            String modelsFilter("mdl");
            URHO3D_LOGERRORF("    file <%s> dirPath <%s> path <%s> ext <%s>", file.name.CString(), file.prefix.CString(), file.path.CString(), file.ext.CString());
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

void EditorWindow::SetPlotVar(int index, float value)
{
//    plotVars_[index][plotVarsOffset_[index]] = value;
//    plotVarsOffset_[index] = (plotVarsOffset_[index] + 1) % (int)(sizeof(plotVars_[index])/sizeof(*plotVars_[index]));
}

void EditorWindow::SetScene(Scene* scene)
{
    scene_ = scene;

    selection_->SetScene(scene);

    if (guizmo_)
        guizmo_->SetScene(scene);
}

void EditorWindow::Render(float timeStep)
{
    if (!scene_)
        return;

    static bool closable = true;
    ImGui::Begin("Scene Inspector", &closable);

    // Guizmo stuff
    static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE currentGizmoMode(ImGuizmo::WORLD);

    static const char* modeTypes[] = { "Object", "Vertex" };

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

    if(ImGui::Button("Save scene"))
    {
        String fileNameWrite = "SaveTest";
        File fileWrite(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + fileNameWrite + ".xml", FILE_WRITE);
        SharedPtr<XMLFile> xml(new XMLFile(context_));
        scene_->SaveXML(fileWrite);
    }

    // ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,80));
    // ImGui::PlotLines("Lines", );
    const PODVector<unsigned>& s = selection_->GetSelectedNodes();

    ImGui::Text("Node selected <%s> guizmoBtn <%i>", selection_->ToString().CString(), guizmoBtn);
    // ImGui::SameLine();
    if(ImGui::Button("\+Local"))
    {
        if (s.Size() == 1)
        {
            Node* node = scene_->GetNode(s.At(0));
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
    if(ImGui::Button("\+Replicated"))
    {
        scene_->CreateChild();
    }

    ImGui::SameLine();
    if(ImGui::Button("\-Remove"))
    {
//        for(unsigned i = 0; i < selectedNodes_.Size(); i++)
//        {
//            Node* node = scene_->GetChild(selectedNodes_.At(i));
//            if (node)
//                scene_->RemoveChild(node);
//        }
    }

    DrawNodeTree();

    ImGui::Separator();

    DrawNodeSelected();

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();

//    ImGui::Text("%s", debugText_.CString());

    ImGui::Separator();

    // FIXME make these dinamyc
//    for(int i = 0; i < 4; i++)
//    {
//        char buf[32];
//        int curIndex = (plotVarsOffset_[i] - 1) % (int)(sizeof(plotVars_[i])/sizeof(*plotVars_[i]));
//        sprintf(buf, "var <%i> <%.4f>", i, plotVars_[i][curIndex]);
////        URHO3D_LOGERRORF("editor-render timestep <%f> value <%f>", timeStep, plotVars_[i][curIndex]);
//        ImGui::PlotLines(buf, plotVars_[i], IM_ARRAYSIZE(plotVars_[i]), plotVarsOffset_[i], NULL, -2.0f, 2.0f, ImVec2(0, 60));
//    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    SetPosition(windowPos.x, windowPos.y);

    ImVec2 windowSize = ImGui::GetWindowSize();
    SetSize(windowSize.x, windowSize.y);

    ImGui::End();
}

void EditorWindow::DrawChild(Node* node, int& i)
{
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    const PODVector<unsigned>& s = selection_->GetSelectedNodes();
    // selected node
    // if (selectedNode_ == node->GetID())
    if (s.Contains(node->GetID()))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    // open if child is selected
    auto children = node->GetChildren();
    for (Node* child : children)
    {
        if (s.Size() == 1 && s.Contains(child->GetID()))
        {
            nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
            break;
        }
    }

    bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, nodeFlags, "%s - %i - %i", node->GetName().CString(), node->GetID(), node->GetChildren().Size());
    if (ImGui::IsItemClicked())
    {
        selection_->Add(node->GetID());
//        selectedNode_ = node->GetID();
////        AddSelectedNode(node->GetID());

//        if (guizmo_)
//        {
//            guizmo_->SetSelectedNode(selectedNode_);
//            URHO3D_LOGERRORF("item selected <%i>", node->GetID());
//        }
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

    const PODVector<unsigned>& s = selection_->GetSelectedNodes();
    if (s.Size() == 1 && s.Contains(scene_->GetID()))
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
            selection_->Add(scene_->GetID());
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
    const PODVector<unsigned> selected = selection_->GetSelectedNodes();
    if (selected.Size() != 1)
        return;

    unsigned selectedNode = selected.At(0);
    if (selectedNode)
    {
        Node* node = scene_->GetNode(selectedNode);
        if (!node)
        {
            URHO3D_LOGERRORF("EditorWindow: node <%u> not found!", selectedNode);
            return;
        }

        // Node* cameraNode = scene_->GetChild("Camera");
        Node* cameraNode = cameraNode_;
        if(!cameraNode)
            return;

        Camera* camera = cameraNode->GetComponent<Camera>();
        if(!camera)
            return;

        AttributeEdit(node);

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

        bool headerOpen = true;
        auto childComponents = node->GetComponents();
        for (Component* c : childComponents)
        {
            // ImGui::Text("%s", c->GetTypeName().CString());
            ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
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
        ImGui::SetNextItemWidth(total_w / 2.0f);

        switch (info.type_)
        {
        case VAR_BOOL:
        {
            bool v = c->GetAttribute(info.name_).GetBool();
            if (ImGui::Checkbox(hideLabel, &v))
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

                if (ImGui::Combo(hideLabel, (int*)&v, enumName.CString()))
                {
                    // URHO3D_LOGERRORF("imgui return val <%u>", v);
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
            float v = c->GetAttribute(info.name_).GetFloat();
            if (ImGui::InputFloat(hideLabel, &v))
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
            if (ImGui::InputText(hideLabel, buffer, 512))
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
            if (ImGui::ColorEdit4(hideLabel, (float*)&col))
            {
                c->SetAttribute(info.name_, Color(col));
            }
        }
        break;
        case VAR_VECTOR2:
        {
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
            float v[4];
            Vector4 value = c->GetAttribute(info.name_).GetVector4();
            memcpy(v, value.Data(), sizeof(v));
            if (ImGui::InputFloat4(hideLabel, (float*)&v))
            {
                c->SetAttribute(info.name_, Vector4(v));
            }
        }
        break;
        case VAR_QUATERNION:
        {
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
                    vertices.Push(Vector2::ZERO);
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
            ImGui::Text("type <%s> name <%s> currentid <%i>", info.defaultValue_.GetTypeName().CString(), v.name_.CString(), currentModel_);
            if (v.type_ == StringHash("ParticleEffect"))
            {
                ParticleEmitter* emitter = dynamic_cast<ParticleEmitter*>(c);
                EditParticleEmitter(emitter);
            }
            else if (v.type_ == StringHash("Model"))
            {
                currentModel_ = FindModel(v.name_);
                if (ImGui::Combo("Model", &currentModel_, modelResourcesString_.CString()))
                {
                    ResourceFile resource = modelResources_.Values().At(currentModel_);
                    v.name_ = resource.path + resource.name;
                    c->SetAttribute(info.name_, v);
                }
            }
            else if (v.type_ == StringHash("Sprite2D"))
            {
//                currentSprite_ = FindSprite(v.name_);
//                if (ImGui::Combo("Sprite", &currentSprite_, spriteResourcesString_.CString()))
//                {
//                    ResourceFile resource = spriteResources_.Values().At(currentSprite_);
//                    v.name_ = resource.path + resource.name;
//                    c->SetAttribute(info.name_, v);
//                }
                auto* cache = GetSubsystem<ResourceCache>();

                static int lines = 1;
                ImGui::SliderInt("Lines", &lines, 1, 15);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                ImGui::BeginChild("scrolling", ImVec2(0, ImGui::GetFrameHeightWithSpacing() + 50), true, ImGuiWindowFlags_HorizontalScrollbar);
                for (int line = 0; line < lines; line++)
                {
//                    int num_buttons = 10 + ((line & 1) ? line * 9 : line * 3);
//                    for (int n = 0; n < num_buttons; n++)
//                    {
//                        if (n > 0) ImGui::SameLine();
//                        ImGui::PushID(n + line * 1000);
//                        char num_buf[16];
//                        sprintf(num_buf, "%d", n);
//                        const char* label = (!(n%15)) ? "FizzBuzz" : (!(n%3)) ? "Fizz" : (!(n%5)) ? "Buzz" : num_buf;
//                        float hue = n*0.05f;
//                        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
//                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
//                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
//                        ImGui::Button(label, ImVec2(40.0f + sinf((float)(line + n)) * 20.0f, 0.0f));
//                        ImGui::PopStyleColor(3);
//                        ImGui::PopID();
//                    }
                    float btnWidth = 32.0f;
                    float btnHeight = 32.0f;
                    for (int i = 0; i < spriteResources_.Size(); i++)
                    {
                        ResourceFile resource = spriteResources_.Values().At(i);

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
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleVar(2);
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
        ImGui::PopID();
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


}
