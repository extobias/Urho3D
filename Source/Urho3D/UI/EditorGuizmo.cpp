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
#include "../IO/MemoryBuffer.h"
#include "../Resource/ResourceCache.h"
#include "../UI/EditorWindow.h"
#include "../UI/EditorSelection.h"
#include "../UI/EditorModelDebug.h"
#include "../UI/UI.h"
#include "../Urho2D/StaticSprite2D.h"
#include "../Urho2D/CollisionPolygon2D.h"

#include "imgui.h"
#include "ImGuizmo.h"
#include "ImCurveEdit.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

static float cubeVertices[] = {
  // front
  -1.0, -1.0,  1.0,
   1.0, -1.0,  1.0,
   1.0,  1.0,  1.0,
  -1.0,  1.0,  1.0,
  // back
  -1.0, -1.0, -1.0,
   1.0, -1.0, -1.0,
   1.0,  1.0, -1.0,
  -1.0,  1.0, -1.0
};

static short cubeIndex[] = {
    // front
    0, 1, 2,
    2, 3, 0,
    // right
    1, 5, 6,
    6, 2, 1,
    // back
    7, 6, 5,
    5, 4, 7,
    // left
    4, 0, 3,
    3, 7, 4,
    // bottom
    4, 5, 1,
    1, 0, 4,
    // top
    3, 2, 6,
    6, 7, 3
};

/// EditorBrush
class URHO3D_API EditorBrush : public Component
{
    URHO3D_OBJECT(EditorBrush, Component);

public:
    explicit EditorBrush(Context* context);

    ~EditorBrush();

    void OnNodeSet(Node* node) override;

    void Update(StaticModel* model, const IntVector2& position, float radius);

    void Update(StaticModel* model, const Vector3& position);

    void SetVisible(bool visible);

    IntRect GetScreenRect();

    void HandleWheelMouse(StringHash eventType, VariantMap& eventData);

private:

    float GetModelHeight(StaticModel* model, const Vector3& position, float x, float z);

    SharedPtr<CustomGeometry> customGeometry_;

    SharedPtr<BorderImage> selectionImage_;

    float size_{ 0.1f };

    bool addedToOctree_{ false };
};

EditorBrush::EditorBrush(Context *context)
    : Component(context)
{
    selectionImage_ = new BorderImage(context_);
    selectionImage_->SetSize(size_, size_);
    selectionImage_->SetBorder(IntRect(3, 3, 3, 3));
    selectionImage_->SetImageBorder(IntRect(10, 10, 10, 10));
    selectionImage_->SetPosition(0.0f, 0.0f);
    selectionImage_->SetOpacity(0.3f);
    selectionImage_->SetColor(Color::GREEN);

    UI* ui = GetSubsystem<UI>();
    ui->GetRoot()->AddChild(selectionImage_);

    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(EditorBrush, HandleWheelMouse));
}

EditorBrush::~EditorBrush() = default;

void EditorBrush::OnNodeSet(Node* node)
{
    if (!node)
        return;

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    customGeometry_ = node->CreateComponent<CustomGeometry>();
    customGeometry_->SetNumGeometries(1);
    customGeometry_->SetMaterial(cache->GetResource<Material>("Materials/DeferredDecal.xml"));
    customGeometry_->SetOccludee(true);
    customGeometry_->SetEnabled(true);
}

void EditorBrush::Update(StaticModel* model, const IntVector2& position, float radius)
{
    if (!node_)
        return;

    // selectionImage_->SetPosition(position.x_ - size_ / 2, position.y_ - size_ / 2);
    Scene* scene = GetScene();
    Node* camNode = scene->GetChild("Camera");
    Camera* camera = camNode->GetComponent<Camera>();
    float z = 0.0f;

    Graphics* g = GetSubsystem<Graphics>();
    float x = (float)position.x_ / g->GetWidth();
    float y = (float)position.y_ / g->GetHeight();
    Vector3 pos = camera->ScreenToWorldPoint(Vector3(x, y, z));

    EditorWindow* editor = static_cast<EditorWindow*>(GetSubsystem<UI>()->GetRootModalElement()->GetChild(String("editor"), false));
    if (editor)
    {
        char str[256];
        sprintf(str, "<%.2f, %.2f, %.2f>", pos.x_, pos.y_, pos.z_);
//        editor->debugText_.Clear();
//        editor->debugText_.AppendWithFormat("spos <%f, %f> \n wpos %s", x, y, str);
    }
    else
    {
        URHO3D_LOGERRORF("EditorBrush::Update: editor not found!");
    }

    node_->SetPosition(pos);

    // Generate the circle
    customGeometry_->BeginGeometry(0, LINE_STRIP);
    for (unsigned i = 0; i < 364; i += 4)
    {
      float angle = (float)i * M_PI / 180.0f;
      float x = radius * Cos(angle / 0.0174532925f);
      float z = radius * Sin(angle / 0.0174532925f);
      float y = 0.01f;
//      float y = terrainComponent.GetHeight(Vector3(position.x + x, 0, position.z + z));
      customGeometry_->DefineVertex(Vector3(x, y, z));
      customGeometry_->DefineColor(Color(1, 0, 0));
    }
    customGeometry_->Commit();

    Octree* octree = node_->GetScene()->GetComponent<Octree>();
    if (octree && !addedToOctree_)
    {
      octree->AddManualDrawable(customGeometry_);
      addedToOctree_ = true;
    }
}

void EditorBrush::Update(StaticModel* model, const Vector3& position)
{
    if (!node_)
        return;

    node_->SetPosition(position);
    EditorWindow* editor = static_cast<EditorWindow*>(GetSubsystem<UI>()->GetRootModalElement()->GetChild(String("editor"), false));
    if (editor)
    {
        char str[256];
        sprintf(str, "<%.2f, %.2f, %.2f>", position.x_, position.y_, position.z_);
//        editor->debugText_.Clear();
//        editor->debugText_.AppendWithFormat("wpos %s", str);
    }
    else
    {
        URHO3D_LOGERRORF("EditorBrush::Update: editor not found!");
    }

    // use cube as geom
    unsigned indexLength = sizeof(cubeIndex) / sizeof(cubeIndex[0]);
    customGeometry_->BeginGeometry(0, TRIANGLE_LIST);
    for (unsigned j = 0; j < indexLength; j ++)
    {
        customGeometry_->DefineVertex(Vector3(cubeVertices + cubeIndex[j] * 3) * size_);
        customGeometry_->DefineColor(Color(1, 0, 0));
    }
    customGeometry_->Commit();

//    unsigned size = 8;
//    for(unsigned j = 0; j < size; j++)
//    {
//        customGeometry_->DefineVertex(Vector3(cubeVertices + j * 3) * vertexScale_);
//        customGeometry_->DefineColor(Color(1, 0, 0));
//    }

    Octree* octree = node_->GetScene()->GetComponent<Octree>();
    if (octree && !addedToOctree_)
    {
      octree->AddManualDrawable(customGeometry_);
      addedToOctree_ = true;
    }
}

float EditorBrush::GetModelHeight(StaticModel* model, const Vector3& position, float x, float z)
{
    return 0.01f;
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
    size_ += delta / 10.0f;
    selectionImage_->SetSize(size_, size_);
}

struct ComponentBufferEdit : public ImCurveEdit::Delegate
{
    ComponentBufferEdit(Node* node)
    {
        node_ = node;

        PODVector<CollisionPolygon2D*> dest;
        node_->GetComponents(dest);

        vertices_.Resize(dest.Size());
        unsigned count = 0;
        for(auto c: dest)
        {
            const PODVector<unsigned char> value = c->GetAttribute("Vertices").GetBuffer();
            // load
            MemoryBuffer buffer(value);
            while (!buffer.IsEof())
            {
                vertices_[count].Push(buffer.ReadVector2());
            }

            count++;
        }

        max_ = ImVec2(1.f, 1.f);
        min_ = ImVec2(0.f, 0.f);
    }

    size_t GetCurveCount()
    {
        return vertices_.Size();
    }

    bool IsVisible(size_t curveIndex)
    {
        return true;
    }

    size_t GetPointCount(size_t curveIndex)
    {
        return vertices_[curveIndex].Size();
    }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
        return cols[curveIndex];
    }

    ImVec2* GetPoints(size_t curveIndex)
    {
        return (ImVec2*)vertices_[curveIndex].Buffer();
    }

    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const
    {
        return ImCurveEdit::CurveLinear;
    }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        // URHO3D_LOGERRORF("EditPoint point index <%i> value <%f, %f>", value.x, value.y);
        vertices_[curveIndex][pointIndex] = Vector2(value.x, value.y);

        VectorBuffer ret;
        for (unsigned i = 0; i < vertices_[curveIndex].Size(); ++i)
            ret.WriteVector2(vertices_[curveIndex][i]);

        PODVector<CollisionPolygon2D*> dest;
        node_->GetComponents(dest);

        dest.At(curveIndex)->SetAttribute("Vertices", ret.GetBuffer());

        return pointIndex;
    }

    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
//        if (vertices_.Size() >= 8)
//            return;
//        vertices_.Push(Vector2(value.x, value.y));
    }

    virtual ImVec2& GetMax() { return max_; }
    virtual ImVec2& GetMin() { return min_; }
    virtual unsigned int GetBackgroundColor() { return 0x20202020; }

  //   bool mbVisible[3];
     ImVec2 min_;
     ImVec2 max_;

  //private:

  //   void SortValues(size_t curveIndex)
  //   {
  //      auto b = std::begin(mPts[curveIndex]);
  //      auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
  //      std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

  //   }

private:

    Vector<PODVector<Vector2> > vertices_;
    Node* node_;
};

/// VerticeEdit
//struct RampEdit : public ImCurveEdit::Delegate
//{
//   RampEdit()
//   {
//      mPts[0][0] = ImVec2(0.1f, 0.1f);
//      mPts[0][1] = ImVec2(0.2f, 0.1f);
//      mPts[0][2] = ImVec2(0.3f, 0.1f);
//      mPts[0][3] = ImVec2(0.4f, 0.1f);
//      mPts[0][4] = ImVec2(0.5f, 0.1f);
//      mPointCount[0] = 5;

//      mPts[1][0] = ImVec2(-5.f, 0.2f);
//      mPts[1][1] = ImVec2(3.f, 0.7f);
//      mPts[1][2] = ImVec2(8.f, 0.2f);
//      mPts[1][3] = ImVec2(8.f, 0.8f);
//      mPointCount[1] = 4;

//      mPts[2][0] = ImVec2(4.f, 0);
//      mPts[2][1] = ImVec2(6.f, 0.1f);
//      mPts[2][2] = ImVec2(9.f, 0.82f);
//      mPts[2][3] = ImVec2(15.f, 0.24f);
//      mPts[2][4] = ImVec2(2.f, 0.34f);
//      mPts[2][5] = ImVec2(2.5f, 0.12f);
//      mPointCount[2] = 6;
//      mbVisible[0] = mbVisible[1] = mbVisible[2] = true;
//      mMax = ImVec2(1.f, 1.f);
//      mMin = ImVec2(0.f, 0.f);
//   }
//   size_t GetCurveCount()
//   {
//      return 1;
//   }

//   bool IsVisible(size_t curveIndex)
//   {
//      return mbVisible[curveIndex];
//   }
//   size_t GetPointCount(size_t curveIndex)
//   {
//      return mPointCount[curveIndex];
//   }

//   uint32_t GetCurveColor(size_t curveIndex)
//   {
//      uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
//      return cols[curveIndex];
//   }
//   ImVec2* GetPoints(size_t curveIndex)
//   {
//      return mPts[curveIndex];
//   }
//   virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const
//   {
//      return ImCurveEdit::CurveLinear;
//   }
//   virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
//   {
//      mPts[curveIndex][pointIndex] = ImVec2(value.x, value.y);
//      // SortValues(curveIndex);
//      for (size_t i = 0; i < GetPointCount(curveIndex); i++)
//      {
//         if (mPts[curveIndex][i].x == value.x)
//            return (int)i;
//      }
//      return pointIndex;
//   }
//   virtual void AddPoint(size_t curveIndex, ImVec2 value)
//   {
//      if (mPointCount[curveIndex] >= 8)
//         return;
//      mPts[curveIndex][mPointCount[curveIndex]++] = value;
//      SortValues(curveIndex);
//   }
//   virtual ImVec2& GetMax() { return mMax; }
//   virtual ImVec2& GetMin() { return mMin; }
//   virtual unsigned int GetBackgroundColor() { return 0x20202020; }

//   ImVec2 mPts[3][8];
//   size_t mPointCount[3];
//   bool mbVisible[3];
//   ImVec2 mMin;
//   ImVec2 mMax;

//private:

//   void SortValues(size_t curveIndex)
//   {
//      auto b = std::begin(mPts[curveIndex]);
//      auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
//      std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

//   }
//};

//static RampEdit points_;

/// EditorGuizmo
EditorGuizmo::EditorGuizmo(Context* context) :
    ImGuiElement(context),
    buttons_(0),
    selection_(nullptr),
    brush_(nullptr),
    clicked_(false),
    editPointHover_(false)
{
    SetDragDropMode(DD_DISABLED);

//    SubscribeToEvent(E_EDITOR_NODE_SELECTED, URHO3D_HANDLER(EditorGuizmo, HandleNodeSelected));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(EditorGuizmo, HandleMouseMove));
}

EditorGuizmo::~EditorGuizmo() = default;

void EditorGuizmo::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorGuizmo>(UI_CATEGORY);
    context->RegisterFactory<EditorBrush>();
}

void EditorGuizmo::Render(float timeStep)
{
    if (!scene_)
        return;

    Graphics* g = GetSubsystem<Graphics>();

    Octree* octree = scene_->GetComponent<Octree>();
    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
    // octree->DrawDebugGeometry(debugRenderer);

    // ImGuizmo::SetDrawlist();

    ImGuizmo::BeginFrame();
    // resize fullscreen for UIElement events
    ImGuizmo::SetRect(0, 0, g->GetWidth(), g->GetHeight());
    SetPosition(0, 0);
    SetSize(g->GetWidth(), g->GetHeight());

    if (!scene_ || !cameraNode_)
        return;

    Camera* camera = cameraNode_->GetComponent<Camera>();
    if (!camera)
        return;

    ImGuizmo::SetOrthographic(camera->IsOrthographic());

    Matrix4 projection = camera->GetProjection().Transpose();
    Matrix4 view = camera->GetView().ToMatrix4().Transpose();
    Matrix4 transform = selection_->GetTransform().ToMatrix4().Transpose();
    Matrix4 delta;

    // grid
    Quaternion gridRotation(90.0f, Vector3::RIGHT);
    Matrix4 gridMatrix;
    // gridMatrix.SetRotation(gridRotation.RotationMatrix());
//    gridMatrix.SetTranslation(Vector3(0.0f, 0.0, -1.0f));

    float gridSize = 10.0f;
    ImGuizmo::DrawGrid(&view.m00_, &projection.m00_, &gridMatrix.m00_, gridSize);

    //  || !selection_->GetSelectedNodes().Size()
    if (!selection_)
        return;

    selection_->Render();

    if (currentEditMode_ == SELECT_OBJECT)
    {
        if (!selection_->IsEmpty())
        {
            ImGuizmo::Manipulate(&view.m00_, &projection.m00_, (ImGuizmo::OPERATION)currentOperation_, (ImGuizmo::MODE)currentMode_, &transform.m00_, &delta.m00_);

            // URHO3D_LOGERRORF("delta translation <%s>", delta.Transpose().Translation().ToString().CString());
            selection_->SetTransform(Matrix3x4(transform.Transpose()));
            selection_->SetDelta(delta.Transpose(), currentOperation_);
        }
    }
    else if (currentEditMode_ == SELECT_MESH_VERTEX)
    {
//            UI* ui = GetSubsystem<UI>();
//            IntVector2 cursorPos = ui->GetCursorPosition();
//            // Vector3 worldPos = GetWorldPosition();
//            StaticModel* staticModel = node->GetComponent<StaticModel>();
//            if(staticModel)
//            {
//                BoundingBox modelBox = staticModel->GetBoundingBox();
//                debugRenderer->AddBoundingBox(modelBox, Color::YELLOW, false);
//                if (brush_)
//                    brush_->Update(staticModel, hitPoint_);
//            }
    }
    else if (currentEditMode_ == SELECT_POLYGON_VERTEX)
    {
        if (selection_->GetSelectedNodes().Size() == 1)
        {
            Node* node = selection_->GetSelectedNodes().At(0);
            if (node)
            {
                PODVector<CollisionPolygon2D*> dest;
                node->GetComponents(dest);
                if (dest.Size())
                {
                    RenderVerticesPoint(node);
                    {
                        // apply changes
                    }
                }
            }
        }
    }



//    debugRenderer->AddLine(ray_.origin_, ray_.origin_ + ray_.direction_ * 1000.0f, Color::BLUE, false);
    // Octree* octree = scene_->GetComponent<Octree>();
    // octree->DrawDebugGeometry(false);
//    Sphere sphereHit;
//    sphereHit.radius_ = 0.01f;
//    sphereHit.center_ = hitPoint_;
//    debugRenderer->AddSphere(sphereHit, Color::RED, false);
//    for(unsigned i = 0; i < hitPositions_.Size(); i++)
//    {
//        sphereHit.center_ = hitPositions_[i];
//        debugRenderer->AddSphere(sphereHit, Color::RED, false);
//    }
}

void EditorGuizmo::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
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
                selection_->Add(result.node_);
            }
            else
            {
                if (!editPointHover_)
                   selection_->Clear();

                clickStart_ = screenPosition;
                clicked_ = true;
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

void EditorGuizmo::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, UIElement* beginElement)
{
    ImGuiElement::OnClickEnd(position, screenPosition, button, buttons, qualifiers, cursor, beginElement);

    clicked_ = false;
}

void EditorGuizmo::OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    ImGuiElement::OnHover(position, screenPosition, buttons, qualifiers, cursor);

//    if (clicked_)
//    {
//        Renderer* renderer = GetSubsystem<Renderer>();
//        Viewport* viewport = renderer->GetViewport(0);

//        Vector3 start = viewport->ScreenToWorldPoint(clickStart_.x_, clickStart_.y_, 100.0f);
//        Vector3 end = viewport->ScreenToWorldPoint(screenPosition.x_, screenPosition.y_, -10.0f);

//        URHO3D_LOGERRORF("EditorGuizmo: start <%s> end <%s>", start.ToString().CString(), end.ToString().CString());

//        auto* graphics = GetSubsystem<Graphics>();
//        Vector3 dpi = graphics->GetDisplayDPI();
//        float scalePhysics = dpi.z_ / 160.0f;
//        float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

//        BoundingBox bb(start, end);
//        DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
//        debugRenderer->AddBoundingBox(bb, Color::BLUE);

//        SelectObjects(bb);
//    }


//    if(ImGuizmo::IsOver())
//        SetFocusMode(FM_FOCUSABLE);
//    else
//        SetFocusMode(FM_NOTFOCUSABLE);

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

void EditorGuizmo::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    if (!scene_ || !selection_)
        return;

    using namespace MouseMove;

    MouseButtonFlags mouseButtons = MouseButtonFlags(eventData[P_BUTTONS].GetUInt());
    QualifierFlags qualifiers = QualifierFlags(eventData[P_QUALIFIERS].GetUInt());
    IntVector2 DeltaP = IntVector2(eventData[P_DX].GetInt(), eventData[P_DY].GetInt());
    IntVector2 screenPosition = IntVector2(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
//    IntVector2 position = ScreenToElement(screenPosition);
    IntVector2 position = screenPosition;

    if(ImGuizmo::IsOver())
        SetFocusMode(FM_FOCUSABLE);
    else
        SetFocusMode(FM_NOTFOCUSABLE);

    const Vector<SharedPtr<Node> >& s = selection_->GetSelectedNodes();
    if (s.Size() == 1)
    {
        Node* node = s.At(0);
        if (node)
        {
            if(currentEditMode_ == SELECT_MESH_VERTEX)
            {
                CalculateHitPoint(position);

                // this is only one action
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
            else if (currentEditMode_ == SELECT_POLYGON_VERTEX)
            {

            }
        }
    }
}

void EditorGuizmo::SetScene(Scene* scene)
{
    // ImGuiElement::SetScene(scene);
    scene_ = scene;

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

void EditorGuizmo::SelectObjects(const BoundingBox& boundingBox)
{
    Octree* octree = scene_->GetComponent<Octree>();
    PODVector<Drawable*> result;
//    Frustum frustum;
//    frustum.Define(boundingBox);
    Camera* camera = cameraNode_->GetComponent<Camera>();
//    Frustum fr =  camera->GetViewSpaceFrustum();
    Frustum fr = camera->GetSplitFrustum(1.0f, 1000.0f);

//    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
//    debugRenderer->AddFrustum(fr, Color::RED);

    FrustumOctreeQuery query(result, fr);

    octree->GetDrawables(query);

    URHO3D_LOGERRORF("EditorGuizmo::SelectObjects: result size <%i>", result.Size());
    if (result.Size())
    {
        for (unsigned i = 0; i < result.Size(); i++)
        {
            Node* node = result.At(i)->GetNode();
            // URHO3D_LOGERRORF("EditorGuizmo::SelectObjects: node name <%s>", node->GetName().CString());
            selection_->Add(node);
        }
    }
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

//    Node* node = scene_->GetNode(selectedNode_);
//    if (node)
//    {
//        EditorModelDebug* debugModel = node->GetComponent<EditorModelDebug>();
//        if (debugModel)
//        {
//            debugModel->SelectVertex(fr);
//        }
//    }
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

//    Node* node = scene_->GetNode(selectedNode_);
    Node* node = nullptr;
//    URHO3D_LOGERRORF("EditorGuizmo: selectvertex result <%i> selNode <%s>", result.Size(), node->GetName().CString());
    if (result.Size())
    {
        hitPositions_.Clear();
        for (int i = 0; i < result.Size(); i++)
        {
            RayQueryResult r = result[i];
            Node* hitNode = r.drawable_->GetNode();
            if (hitNode)
            {
                String name = hitNode->GetName();
                if (name == "Zone")
                    continue;
            }

            if(r.subObjectElementIndex_ != M_MAX_UNSIGNED)
            {
                if (node && node->GetID() == r.node_->GetID())
                {
                    hitPositions_.Push(r.position_);
                    faces.Push(IntVector2(r.subObject_, r.subObjectElementIndex_));
                }
            }
//            URHO3D_LOGERRORF("EditorGuizmo: selectvertex result <%i> hitNode <%s>", result.Size(), hitNode->GetName().CString());
//            if (node && node->GetID() == r.node_->GetID())
//            {
////                URHO3D_LOGERRORF("rayquery result: node <%s> subObject <%i> subObjectElementIndex <%i>",
////                                 r.node_->GetName().CString(), r.subObject_, r.subObjectElementIndex_);
//                if(r.subObjectElementIndex_ == M_MAX_UNSIGNED)
//                {
//                    hitPositions_.Push(r.position_);
//                }
//                else
//                {
//                    if (node && node->GetID() == r.node_->GetID())
//                    {
//                        faces.Push(IntVector2(r.subObject_, r.subObjectElementIndex_));
//                    }
//                }
//            }
        }
    }

    return faces;
}

void EditorGuizmo::CreateBrush()
{
    Node* brushNode = scene_->CreateTemporaryChild("BrushNode", LOCAL);
    brush_ = brushNode->CreateComponent<EditorBrush>();
}

void EditorGuizmo::CalculateHitPoint(const IntVector2& position)
{
    Octree* octree = scene_->GetComponent<Octree>();
    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* v0 = renderer->GetViewport(0);
    IntVector2 mousePos = position;
    ray_ = v0->GetScreenRay(mousePos.x_, mousePos.y_);

    PODVector<RayQueryResult> result;
    RayOctreeQuery query(result, ray_, RAY_TRIANGLE);
    octree->Raycast(query);

//    Node* node = scene_->GetNode(selectedNode_);
//    if (result.Size())
//    {
//        for (int i = 0; i < result.Size(); i++)
//        {
//            RayQueryResult r = result[i];
//            Node* hitNode = r.drawable_->GetNode();
//            if (hitNode)
//            {
//                String name = hitNode->GetName();
//                if (name == "Zone")
//                    continue;
//            }

//            if(r.subObjectElementIndex_ != M_MAX_UNSIGNED)
//            {
//                if (node && node->GetID() == r.node_->GetID())
//                {
//                    hitPoint_ = r.position_;
//                }
//            }
//        }
//    }
}

void EditorGuizmo::RenderVerticesPoint(Node *node)
{
    Graphics* g = GetSubsystem<Graphics>();

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("VerticeEdit", nullptr, flags);

    Camera* camera = cameraNode_->GetComponent<Camera>();
    if (!camera)
        return;

    Matrix4 projection = camera->GetProjection().Transpose();
    Matrix4 view = camera->GetView().ToMatrix4().Transpose();
    Matrix4 transform = node->GetTransform().ToMatrix4().Transpose();

    ComponentBufferEdit buffer(node);
    ImVector<ImCurveEdit::EditPoint> selectedPoints;

    editPointHover_ = ImCurveEdit::EditPolygon(&view.m00_, &projection.m00_, &transform.m00_, buffer, ImVec2(g->GetWidth(), g->GetHeight()), NULL, &selectedPoints);

    // URHO3D_LOGERRORF("EditorGuizmo: point edited <%i> selected <%i>", pointEdited_, selectedPoints.size());

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

}
