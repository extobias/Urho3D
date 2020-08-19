#include "../UI/EditorSelection.h"

#include "../Core/CoreEvents.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/DebugRenderer.h"
#include "../Input/Input.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../IO/MemoryBuffer.h"
#include "../Math/Polyhedron.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/SceneEvents.h"
#include "../UI/UI.h"
#include "../Urho2D/StaticSprite2D.h"

namespace Urho3D
{

EditorSelection::EditorSelection(Context* context)
    : Object (context)
{
}

EditorSelection::~EditorSelection() = default;

void EditorSelection::Add(Node *node)
{
    if (selectedNodes_.Contains(node))
        return;

    Input* input = GetSubsystem<Input>();
    if (!input->GetKeyDown(KEY_SHIFT))
        selectedNodes_.Clear();

    selectedNodes_.Push(node);

    UpdateTransform();
}

void EditorSelection::Clear()
{
    selectedNodes_.Clear();
}

String EditorSelection::ToString()
{
    char buf[5120];
    memset(buf, 0, 5120);
    unsigned offset = 0;
    for(Node* node: selectedNodes_)
    {
        sprintf(buf + offset, "%i, ", node->GetID());
        offset = strlen(buf);
    }

    return buf;
}

void EditorSelection::SetTransform(const Matrix3x4 &matrix)
{
    transform_ = matrix;
}

void EditorSelection::SetDelta(const Matrix4 &matrix)
{
    for(Node* node: selectedNodes_)
    {
        node->Translate(matrix.Translation(), TS_WORLD);
        // node->Rotate(matrix.Rotation());
        node->RotateAround(transform_.Translation(), -matrix.Rotation(), TS_WORLD);
        node->Scale(matrix.Scale());
    }
}

void EditorSelection::Render()
{
    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
    for(Node* node: selectedNodes_)
    {
        if (node)
        {
            StaticSprite2D* draw = node->GetComponent<StaticSprite2D>();
            if (draw)
            {
                debugRenderer->AddBoundingBox(draw->GetBoundingBox(), node->GetWorldTransform(), Color::YELLOW);
            }
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
        Node* node = selectedNodes_.At(0);
        transform_ = node->GetWorldTransform();
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
        Node* nodeSelected = selectedNodes_.At(0);
        points.Push(nodeSelected->GetWorldPosition());

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
        Node* nodeSelected = selectedNodes_.At(i);
        Vector3 pos = nodeSelected->GetWorldPosition();
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
        Node* nodeSelected = selectedNodes_.At(i);
        Vector3 point = nodeSelected->GetWorldPosition();
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

}
