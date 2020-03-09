#include "../UI/EditorGuizmo.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/View.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/StaticModel.h"
#include "../Input/Input.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../UI/EditorWindow.h"
#include "../UI/EditorModelDebug.h"
#include "../UI/UI.h"

#include "imgui.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

/// EditorBrush
class URHO3D_API EditorBrush : public Object
{
    URHO3D_OBJECT(EditorBrush, Object);

public:
    explicit EditorBrush(Context* context, Node* node);

    ~EditorBrush();

    void Update(StaticModel* model, const IntVector2& position, float radius);

    void SetVisible(bool visible);

    IntRect GetScreenRect();

    void HandleWheelMouse(StringHash eventType, VariantMap& eventData);

private:

    SharedPtr<Node> node_;

    SharedPtr<CustomGeometry> customGeometry_;

    SharedPtr<BorderImage> selectionImage_;

    int size_{ 5 };

    bool addedToOctree_{ false };
};

EditorBrush::EditorBrush(Context *context, Node *node)
    : Object(context),
      node_(node)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    selectionImage_ = new BorderImage(context_);
    selectionImage_->SetSize(size_, size_);
    selectionImage_->SetBorder(IntRect(3, 3, 3, 3));
    selectionImage_->SetImageBorder(IntRect(10, 10, 10, 10));
    selectionImage_->SetPosition(0, 0);
    selectionImage_->SetOpacity(0.3);
    selectionImage_->SetColor(Color::GREEN);

    UI* ui = GetSubsystem<UI>();
    ui->GetRoot()->AddChild(selectionImage_);
//    customGeometry_ = node_->CreateComponent<CustomGeometry>();
//    customGeometry_->SetNumGeometries(1);
//    customGeometry_->SetMaterial(cache->GetResource<Material>("Materials/VColUnlit.xml"));
//    customGeometry_->SetOccludee(false);
//    customGeometry_->SetEnabled(true);

    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(EditorBrush, HandleWheelMouse));
}

EditorBrush::~EditorBrush() = default;

void EditorBrush::Update(StaticModel* model, const IntVector2& position, float radius)
{
    // node_->SetPosition(Vector3(position.x_, 0.0f, position.z_));
    selectionImage_->SetPosition(position.x_ - size_ / 2, position.y_ - size_ / 2);
    // Generate the circle
//    customGeometry_->BeginGeometry(0, LINE_STRIP);
//    for (unsigned i = 0; i < 364; i += 4)
//    {
//      float angle = i * M_PI / 180;
//      float x = radius * Cos(angle / 0.0174532925);
//      float z = radius * Sin(angle / 0.0174532925);
//      float y = 1.0f;
//      // float y = terrainComponent.GetHeight(Vector3(position.x + x, 0, position.z + z));
//      customGeometry_->DefineVertex(Vector3(x, y + 0.25, z));
//      customGeometry_->DefineColor(Color(1, 1, 0));
//    }
//    customGeometry_->Commit();

//    Octree* octree = node_->GetScene()->GetComponent<Octree>();
//    if (octree && !addedToOctree_)
//    {
//      octree->AddManualDrawable(customGeometry_);
//      addedToOctree_ = true;
//    }
}

void EditorBrush::SetVisible(bool visible)
{
    selectionImage_->SetVisible(visible);
}

IntRect EditorBrush::GetScreenRect()
{
    // return selectionImage_->GetImageRect();
    IntVector2 position = selectionImage_->GetPosition();
    return IntRect(position, position + IntVector2(size_, size_));
}

void EditorBrush::HandleWheelMouse(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseWheel;
    int delta = eventData[P_WHEEL].GetInt();
    size_ += delta;
    selectionImage_->SetSize(size_, size_);
}


/// EditorGuizmo
EditorGuizmo::EditorGuizmo(Context* context) :
    ImGuiElement(context),
    buttons_(0)
{
    SetDragDropMode(DD_DISABLED);

    SubscribeToEvent(E_EDITOR_NODE_SELECTED, URHO3D_HANDLER(EditorGuizmo, HandleNodeSelected));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(EditorGuizmo, HandleMouseMove));
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

    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
//    debugRenderer->AddFrustum(selectFrustum_, Color::RED, false);

    Node* node = scene_->GetNode(selectedNode_);
    brush_->SetVisible(currentEditMode_ == SELECT_VERTEX && node);

    if (node)
    {
        if(currentEditMode_ == SELECT_OBJECT)
        {
            Matrix4 identity = Matrix4::IDENTITY;
            Matrix4 projection = camera->GetProjection().Transpose();
            Matrix4 view = camera->GetView().ToMatrix4().Transpose();
            Matrix4 nodeTransform = node->GetTransform().ToMatrix4().Transpose();

            ImGuizmo::Manipulate(&view.m00_, &projection.m00_, currentOperation_, currentMode_, &nodeTransform.m00_);

            node->SetTransform(Matrix3x4(nodeTransform.Transpose()));
        }
        else if(currentEditMode_ == SELECT_VERTEX)
        {
            UI* ui = GetSubsystem<UI>();
            IntVector2 cursorPos = ui->GetCursorPosition();

            // Vector3 worldPos = GetWorldPosition();

            StaticModel* staticModel = node->GetComponent<StaticModel>();
            if(staticModel)
            {
                BoundingBox modelBox = staticModel->GetBoundingBox();

                debugRenderer->AddBoundingBox(modelBox, Color::YELLOW, false);

                brush_->Update(staticModel, cursorPos, 1.0f);
            }
        }
    }

    debugRenderer->AddLine(ray_.origin_, ray_.origin_ + ray_.direction_ * 1000.0f, Color::BLUE, false);

    // Octree* octree = scene_->GetComponent<Octree>();
    // octree->DrawDebugGeometry(false);

//    Sphere sphereHit;
//    sphereHit.radius_ = 0.01f;
//    for(unsigned i = 0; i < hitPositions_.Size(); i++)
//    {
//        sphereHit.center_ = hitPositions_[i];
//        debugRenderer->AddSphere(sphereHit, Color::RED, false);
//    }
}

void EditorGuizmo::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor)
{
    ImGuiElement::OnClickBegin(position, screenPosition, button, buttons, qualifiers, cursor);

    // check new selected node
    if (!ImGuizmo::IsOver() && button & MOUSEB_LEFT)
    {
        if(currentEditMode_ == SELECT_OBJECT)
        {
            RayQueryResult result = SelectObject(position);
            if(result.node_)
            {
                selectedNode_ = result.node_->GetID();

                VariantMap eventData;
                eventData[P_GUIZMO_NODE_SELECTED] = selectedNode_;
                eventData[P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX] = result.subObjectElementIndex_;
                eventData[P_GUIZMO_NODE_SELECTED_POSITION] = result.position_;
                SendEvent(E_GUIZMO_NODE_SELECTED, eventData);
            }
        }
//        else if(currentEditMode_ == SELECT_VERTEX)
//        {
//            PODVector<IntVector2> faces = SelectVertex(position);
//            Node* node = scene_->GetNode(selectedNode_);
//            if (node)
//            {
//                EditorModelDebug* debugModel = node->GetComponent<EditorModelDebug>();
//                if (debugModel)
//                {
//                    debugModel->DrawFaces(faces);
//                }
//            }
//        }
    }
}

void EditorGuizmo::OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor)
{
//    ImGuiElement::OnHover(position, screenPosition, buttons, qualifiers, cursor);

//    if(ImGuizmo::IsOver())
//        SetFocusMode(FM_FOCUSABLE);
//    else
//        SetFocusMode(FM_NOTFOCUSABLE);

//    // URHO3D_LOGERRORF("EditorGuizmo::OnHover");

//    Node* node = scene_->GetNode(selectedNode_);
//    if(currentEditMode_ == SELECT_VERTEX && node)
//    {
//        PODVector<IntVector2> faces = SelectVertex(position);
//        EditorModelDebug* debugModel = node->GetComponent<EditorModelDebug>();
////        for(unsigned i = 0; i < faces.Size(); i++)
////        {
////            // URHO3D_LOGERRORF("selected vertex face <%i, %i>", faces.At(i).x_, faces.At(i).y_);
////            if (debugModel)
////            {
////                debugModel->DrawFaces(faces);
////            }
////        }

//        buttons_ = buttons;
//        if (buttons & MOUSEB_LEFT)
//        {
//            // URHO3D_LOGERRORF("selected vertex face");
//            if (debugModel)
//            {
//                // debugModel->AddSelectedFaces(faces);
//                debugModel->DrawFaces(faces);
//            }
//        }
//    }
}

void EditorGuizmo::HandleNodeSelected(StringHash eventType, VariantMap& eventData)
{
    selectedNode_ = eventData[P_EDITOR_NODE_SELECTED].GetUInt();

    // selectedSubElementIndex_ = eventData[P_GUIZMO_NODE_SELECTED_SUBELEMENTINDEX].GetUInt();
    // hitPosition_ = eventData[P_GUIZMO_NODE_SELECTED_POSITION].GetVector3();
}

void EditorGuizmo::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseMove;

    MouseButtonFlags mouseButtons = MouseButtonFlags(eventData[P_BUTTONS].GetUInt());
    QualifierFlags qualifiers = QualifierFlags(eventData[P_QUALIFIERS].GetUInt());
    IntVector2 DeltaP = IntVector2(eventData[P_DX].GetInt(), eventData[P_DY].GetInt());
    IntVector2 screenPosition = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
//    IntVector2 position = ScreenToElement(screenPosition);
    IntVector2 position = screenPosition;

    ImGuiElement::OnHover(position, screenPosition, mouseButtons, qualifiers, nullptr);

    if(ImGuizmo::IsOver())
        SetFocusMode(FM_FOCUSABLE);
    else
        SetFocusMode(FM_NOTFOCUSABLE);

    Node* node = scene_->GetNode(selectedNode_);
    if(currentEditMode_ == SELECT_VERTEX && node)
    {
        if (mouseButtons & MOUSEB_LEFT)
        {
            PODVector<IntVector2> faces = SelectVertex(position);
            EditorModelDebug* debugModel = node->GetComponent<EditorModelDebug>();

            if (debugModel)
            {
                debugModel->AddSelectedFaces(faces);
            }
            buttons_ = mouseButtons;
        }
    }
}

void EditorGuizmo::SetScene(Scene* scene)
{
    ImGuiElement::SetScene(scene);

    CreateBrush();
}

Ray EditorGuizmo::GetCameraRay()
{
    Node* cameraNode = cameraNode_;
    if (!cameraNode)
        return Ray();

    Camera* camera = cameraNode->GetComponent<Camera>();
//    float x = float(cursorPos.x_ - rect.Left()) / rect.Width();
//    float y = float(cursorPos.y_ - rect.Top()) / rect.Height();
    IntVector2 screenPos = GetScreenPosition();
    // URHO3D_LOGERRORF("editorgizmo.getcameraray: pos <%f, %f>", x, y);
    return camera->GetScreenRay(screenPos.x_, screenPos.y_);
}

Vector3 EditorGuizmo::GetWorldPosition()
{
    if (!cameraNode_)
        return Vector3::ZERO;

    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* viewport = renderer->GetViewport(0);
    View* view = viewport->GetView();
    if (!view)
        return Vector3::ZERO;

    IntRect rect = view->GetViewRect();

    UI* ui = GetSubsystem<UI>();
    IntVector2 cursorPos = ui->GetCursorPosition();

    float x = float(cursorPos.x_ - rect.Left()) / rect.Width();
    float y = float(cursorPos.y_ - rect.Top()) / rect.Height();

    // URHO3D_LOGERRORF("screen pos <%f, %f> cursor <%u, %u>", x, y, cursorPos.x_, cursorPos.y_);

    Camera* camera = cameraNode_->GetComponent<Camera>();
    return camera->ScreenToWorldPoint(Vector3(x, y, 0.0f));
    // return viewport->ScreenToWorldPoint(cursorPos.x_, cursorPos.y_, 0.0f);
}

RayQueryResult EditorGuizmo::SelectObject(const IntVector2& position)
{
    if (!scene_)
        return RayQueryResult();

    Octree* octree = scene_->GetComponent<Octree>();
    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* v0 = renderer->GetViewport(0);
    IntVector2 mousePos = position;
    ray_ = v0->GetScreenRay(mousePos.x_, mousePos.y_);

    PODVector<RayQueryResult> result;
    RayOctreeQuery query(result, ray_);
    octree->Raycast(query);

    if (result.Size())
    {
        hitPositions_.Clear();
        for (int i = 0; i < result.Size(); i++)
        {
            RayQueryResult r = result[i];
            Node* hitNode = r.drawable_->GetNode();

            if(r.subObjectElementIndex_ == M_MAX_UNSIGNED)
            {
                hitPositions_.Push(r.position_);
            }

            if (hitNode)
            {
                String name = hitNode->GetName();
                if (name == "Zone")
                    continue;

                return r;
            }
        }
    }

    return RayQueryResult();
}

void EditorGuizmo::SelectVertex(const IntRect& screenRect)
{
    if (!cameraNode_ || !scene_)
        return;

    Camera* camera = cameraNode_->GetComponent<Camera>();
    Octree* octree = scene_->GetComponent<Octree>();
    Graphics* graphics = GetSubsystem<Graphics>();

    IntVector2 rectPos = screenRect.Min();
    IntVector2 rectEnd = screenRect.Max();
//    URHO3D_LOGERRORF("fov <%f> aspect <%f> zoom <%f> near <%f> far <%f>", camera->GetFov(), camera->GetAspectRatio(),
//                     camera->GetZoom(), camera->GetNearClip(), camera->GetFarClip());
    // Frustum fr = camera->GetFrustum();
    camera->SetOrthographic(true);
    Frustum fr =  camera->GetSplitFrustum(1.0f, 1000.0f);
    camera->SetOrthographic(false);

    Ray lefttopRectRay = camera->GetScreenRay((float)rectPos.x_ / graphics->GetWidth(), (float)rectPos.y_ / graphics->GetHeight());
    float dNear = lefttopRectRay.HitDistance(fr.planes_[PLANE_NEAR]);
    float dFar = lefttopRectRay.HitDistance(fr.planes_[PLANE_FAR]);
    fr.vertices_[3] = lefttopRectRay.origin_ + dNear * lefttopRectRay.direction_;
    fr.vertices_[7] = lefttopRectRay.origin_ + dFar * lefttopRectRay.direction_;

    Ray leftbottomRectRay = camera->GetScreenRay((float)rectPos.x_ / graphics->GetWidth(), (float)rectEnd.y_ / graphics->GetHeight());
    dNear = leftbottomRectRay.HitDistance(fr.planes_[PLANE_NEAR]);
    dFar = leftbottomRectRay.HitDistance(fr.planes_[PLANE_FAR]);
    fr.vertices_[2] = leftbottomRectRay.origin_ + dNear * leftbottomRectRay.direction_;
    fr.vertices_[6] = leftbottomRectRay.origin_ + dFar * leftbottomRectRay.direction_;

    Ray righttopRectRay = camera->GetScreenRay((float)rectEnd.x_ / graphics->GetWidth(), (float)rectPos.y_ / graphics->GetHeight());
    dNear = righttopRectRay.HitDistance(fr.planes_[PLANE_NEAR]);
    dFar = righttopRectRay.HitDistance(fr.planes_[PLANE_FAR]);
    fr.vertices_[0] = righttopRectRay.origin_ + dNear * righttopRectRay.direction_;
    fr.vertices_[4] = righttopRectRay.origin_ + dFar * righttopRectRay.direction_;

    Ray rightbottomRectRay = camera->GetScreenRay((float)rectEnd.x_ / graphics->GetWidth(), (float)rectEnd.y_ / graphics->GetHeight());
    dNear = rightbottomRectRay.HitDistance(fr.planes_[PLANE_NEAR]);
    dFar = rightbottomRectRay.HitDistance(fr.planes_[PLANE_FAR]);
    fr.vertices_[1] = rightbottomRectRay.origin_ + dNear * rightbottomRectRay.direction_;
    fr.vertices_[5] = rightbottomRectRay.origin_ + dFar * rightbottomRectRay.direction_;

    fr.UpdatePlanes();

    Node* node = scene_->GetNode(selectedNode_);
    if (node)
    {
        EditorModelDebug* debugModel = node->GetComponent<EditorModelDebug>();
        if (debugModel)
        {
            debugModel->SelectVertex(fr);
        }
    }
}

PODVector<IntVector2> EditorGuizmo::SelectVertex(const IntVector2& position)
{
    PODVector<IntVector2> faces;
    if (!cameraNode_ || !scene_)
        return faces;

    Octree* octree = scene_->GetComponent<Octree>();
    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* v0 = renderer->GetViewport(0);
    IntVector2 mousePos = position;
    ray_ = v0->GetScreenRay(mousePos.x_, mousePos.y_);

    PODVector<RayQueryResult> result;
    RayOctreeQuery query(result, ray_, RAY_TRIANGLE);
    octree->Raycast(query);

    Node* node = scene_->GetNode(selectedNode_);
//    URHO3D_LOGERRORF("EditorGuizmo: selectvertex result <%i> selNode <%s>", result.Size(), node->GetName().CString());
    if (result.Size())
    {
        hitPositions_.Clear();
        for (int i = 0; i < result.Size(); i++)
        {
            RayQueryResult r = result[i];
            Node* hitNode = r.drawable_->GetNode();
//            URHO3D_LOGERRORF("EditorGuizmo: selectvertex result <%i> hitNode <%s>", result.Size(), hitNode->GetName().CString());
//            if (node && node->GetID() == r.node_->GetID())
//            {
//                URHO3D_LOGERRORF("rayquery result: node <%s> subObject <%i> subObjectElementIndex <%i>",
//                                 r.node_->GetName().CString(), r.subObject_, r.subObjectElementIndex_);
//            }

            if(r.subObjectElementIndex_ == M_MAX_UNSIGNED)
            {
                hitPositions_.Push(r.position_);
            }
            else
            {
                if (node && node->GetID() == r.node_->GetID())
                {
                    faces.Push(IntVector2(r.subObject_, r.subObjectElementIndex_));
                }
            }

            if (hitNode)
            {
                String name = hitNode->GetName();
                if (name == "Zone")
                    continue;
            }
        }
    }

    return faces;
}

void EditorGuizmo::CreateBrush()
{
    Node* brushNode = scene_->CreateTemporaryChild("BrushNode", LOCAL);
    brush_ = new EditorBrush(context_, brushNode);
}

}
