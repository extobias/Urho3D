#include "EditorModelDebug.h"

#include "../Container/Ptr.h"
#include "../Core/Context.h"
#include "../Graphics/Batch.h"
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/OctreeQuery.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

enum ClipMask : unsigned
{
    CLIPMASK_X_POS = 0x1,
    CLIPMASK_X_NEG = 0x2,
    CLIPMASK_Y_POS = 0x4,
    CLIPMASK_Y_NEG = 0x8,
    CLIPMASK_Z_POS = 0x10,
    CLIPMASK_Z_NEG = 0x20,
};
URHO3D_FLAGSET(ClipMask, ClipMaskFlags);

extern const char* GEOMETRY_CATEGORY;

static float cubeVertices[] = {
    // front
    -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0,
    // back
    -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0};

static short cubeIndex[] = {
    // front
    0, 1, 2, 2, 3, 0,
    // right
    1, 5, 6, 6, 2, 1,
    // back
    7, 6, 5, 5, 4, 7,
    // left
    4, 0, 3, 3, 7, 4,
    // bottom
    4, 5, 1, 1, 0, 4,
    // top
    3, 2, 6, 6, 7, 3};

static const VertexCollisionMask DEFAULT_VERTEX_COLLISION_MASK_TYPE = TRACK_ROAD;

static const char* VertexCollisionMaskNames[] = {"None",          "Track Road",     "Track Border",
                                                 "Offtrack Weak", "Offtrack Heavy", nullptr};

EditorModelDebug::EditorModelDebug(Context* context)
    : Drawable(context, DRAWABLE_GEOMETRY)
    , geometry_(new Geometry(context))
    , geometryFaces_(new Geometry(context))
    , vertexBuffer_(new VertexBuffer(context))
    , indexBuffer_(new IndexBuffer(context))
    , vertexBufferFaces_(new VertexBuffer(context))
    , indexBufferFaces_(new IndexBuffer(context))
    , vertexMaskType_(TRACK_ROAD)
    , vertexScale_(0.01f)
    , currentFace_(0)
    , currentIndex_(IntVector3::ZERO)
    , texture_(nullptr)
    , img_(nullptr)
    , imgData_(nullptr)
    , brushColor_(1.0f, 0.0f, 0.0f, 1.0f)
    , brushSize_(100)
    , lastUVPoint_(-1, -1)
{
    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);

    geometryFaces_->SetVertexBuffer(0, vertexBufferFaces_);
    geometryFaces_->SetIndexBuffer(indexBufferFaces_);

    batches_.Resize(2);
    // original model vertices
    // batches_[0].geometry_ = geometry_;
    // batches_[0].geometryType_ = GEOM_INSTANCED;

    // debug triangle info and world's transform for each vertices
    // batches_[1].geometry_ = geometryFaces_;
    // batches_[1].geometryType_ = GEOM_INSTANCED;
}

EditorModelDebug::~EditorModelDebug()
{
    free(imgData_);

    if (batches_.Size() && batches_[0].instancingData_)
        delete batches_[0].instancingData_;

    if (batches_.Size() > 1 && batches_[1].instancingData_)
        delete batches_[1].instancingData_;
}

void EditorModelDebug::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorModelDebug>(GEOMETRY_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Vertex Mask Type", vertexMaskType_, VertexCollisionMaskNames,
                          DEFAULT_VERTEX_COLLISION_MASK_TYPE, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, ResourceRef,
                                    ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
}

void EditorModelDebug::ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results)
{
    // If no bones or no bone-level testing, use the Drawable test
    RayQueryLevel level = query.level_;
    if (level < RAY_AABB)
    {
        Drawable::ProcessRayQuery(query, results);
        return;
    }

    // Check ray hit distance to AABB before proceeding with more accurate tests
    // GetWorldBoundingBox() updates the world transforms
    if (query.ray_.HitDistance(GetWorldBoundingBox()) >= query.maxDistance_)
        return;

    for (unsigned i = 0; i < numWorldTransforms_; ++i)
    {
        // Initial test using AABB
        const Matrix3x4& transform = worldTransforms_[i];
        BoundingBox transformedBoundingBox = boundingBox_.Transformed(transform);
        float distance = query.ray_.HitDistance(transformedBoundingBox);
        Vector3 normal = -query.ray_.direction_;
        unsigned subObjectElementIndex = M_MAX_UNSIGNED;

        // Then proceed to OBB and triangle-level tests if necessary
        if (level >= RAY_OBB && distance < query.maxDistance_)
        {
            float distance2 = query.ray_.HitDistance(boundingBox_.Transformed(transform));

            Matrix3x4 inverse = worldTransforms_[i].Inverse();
            Ray localRay = query.ray_.Transformed(inverse);
            distance = localRay.HitDistance(boundingBox_);

            if (level == RAY_TRIANGLE && distance < query.maxDistance_)
            {
                distance = M_INFINITY;

                for (unsigned j = 0; j < batches_.Size(); ++j)
                {
                    Geometry* geometry = batches_[j].geometry_;
                    if (geometry)
                    {
                        Vector3 geometryNormal;
                        float geometryDistance =
                            geometry->GetHitDistance(localRay, subObjectElementIndex, &geometryNormal);
                        if (geometryDistance < query.maxDistance_ && geometryDistance < distance)
                        {
                            distance = geometryDistance;
                            normal = (worldTransforms_[i] * Vector4(geometryNormal, 0.0f)).Normalized();
                        }
                    }
                }
            }
        }

        if (distance < query.maxDistance_)
        {
            RayQueryResult result;
            result.position_ = query.ray_.origin_ + distance * query.ray_.direction_;
            result.normal_ = normal;
            result.distance_ = distance;
            result.drawable_ = this;
            result.node_ = node_;
            result.subObject_ = i;
            result.subObjectElementIndex_ = subObjectElementIndex;
            results.Push(result);
        }
    }
}

void EditorModelDebug::UpdateBatches(const FrameInfo& frame)
{
    unsigned offset = 0;
    unsigned char* instanceData = static_cast<unsigned char*>(batches_[0].instancingData_);
    Color color(Color::GREEN);

    const Matrix3x4* worldTransform = node_ ? &node_->GetWorldTransform() : nullptr;
    for (unsigned i = 0; i < numWorldTransforms_; i++)
    {
        // transform
        Matrix3x4 transform(Matrix3x4::IDENTITY);
        transform.SetTranslation(*worldTransform * vertexOffset_[i]);
        worldTransforms_[i] = transform;

        // color
        memcpy(instanceData + offset, color.ToVector4().Data(), sizeof(Vector4));
        offset += sizeof(Vector4);
    }

    color = Color::RED;
    for (unsigned i = 0; i < selectedIndex_.Size(); i++)
    {
        int s = selectedIndex_[i];
        memcpy(instanceData + s * sizeof(Vector4), color.ToVector4().Data(), sizeof(Vector4));
    }

    // CalcMatrixFaces();

    UpdateFacesColor();

    //    offset = 0;
    //    unsigned char* instanceDataFaces = static_cast<unsigned char*>(batches_[1].instancingData_);
    //    for(unsigned i = 0; i < numWorldTransformsFaces_; i++)
    //    {
    //        Matrix3x4 transform(Matrix3x4::IDENTITY);
    //        transform.SetTranslation(Vector3(0.0f, 0.5f, 0.0f));

    //        // Matrix4 tt =  worldTransform->ToMatrix4().Transpose() *
    //        worldTransformsFaces_[i].ToMatrix4().Transpose();
    //        // transform = Matrix3x4(tt);
    //        // worldTransformsFaces_[i] = worldTransformsFaces_[i] * *worldTransform;

    //        // worldTransformsFaces_[i].SetTranslation(Vector3(0.0f, 1.0f, 0.0f));
    //    }
}

void EditorModelDebug::UpdateGeometry(const FrameInfo& frame)
{
    URHO3D_LOGERRORF("editordebug applyattr: <%u>", vertexMaskType_);
}

void EditorModelDebug::ApplyAttributes() { URHO3D_LOGERRORF("editordebug applyattr: <%u>", vertexMaskType_); }

void EditorModelDebug::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);

    if (attr.name_ == "Vertex Mask Type")
    {
        //        switch (src.GetInt()) {
        //            case 0: vertexMaskType_ = NONE; break;
        //            case 1: vertexMaskType_ = TRACK_ROAD; break;
        //            case 2: vertexMaskType_ = TRACK_BORDER; break;
        //            case 3: vertexMaskType_ = OFFTRACK_WEAK; break;
        //            case 4: vertexMaskType_ = OFFTRACK_HEAVY; break;
        //        }
        //        if(src.GetInt())
        //            vertexMaskType_ = (VertexCollisionMask) (1 << (src.GetInt() - 1));
        //        else
        //            vertexMaskType_ = NONE;
    }

    // char b[16];
    // sprintf(b, "%#010x", vertexMaskType_);
    // URHO3D_LOGERRORF("editordebug attrset: <%s>", b);
    // URHO3D_LOGERRORF("editordebug attrset: name <%s> value <%i>", attr.name_.CString(), src.GetInt());

    selectedFaces_.Clear();
    selectedIndex_.Clear();
    selectedFacesIndex_.Clear();
}

void EditorModelDebug::SetModelAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetModel(cache->GetResource<Model>(value.name_));
}

ResourceRef EditorModelDebug::GetModelAttr() const { return GetResourceRef(model_, Model::GetTypeStatic()); }

unsigned EditorModelDebug::GetFacesCount() const
{
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();
    IndexBuffer* indexBuffer = ib.At(0);

    unsigned count = indexBuffer->GetIndexCount();

    return count / 3;
}

void EditorModelDebug::SetModel(Model* model)
{
    if (model == model_)
        return;

    if (!node_)
    {
        URHO3D_LOGERROR("Can not set model while model component is not attached to a scene node");
        return;
    }

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    if (model_)
        UnsubscribeFromEvent(model_, E_RELOADFINISHED);

    model_ = model;

    StaticModel* staticModel = node_->GetComponent<StaticModel>();
    if (staticModel)
    {
        Material* mat = staticModel->GetMaterial();
        if (mat)
        {
            texture_ = (Texture2D*)mat->GetTexture(TU_DIFFUSE);
            if (texture_)
            {
                // SharedPtr<Image> testImg = texture_->GetImage();
                // testImg->SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                // Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");

                img_ = cache->GetResource<Image>(texture_->GetName());
                imgAux_ = cache->GetResource<Image>(texture_->GetName());

                unsigned imgSize = img_->GetWidth() * img_->GetHeight() * sizeof(unsigned);
                imgData_ = (unsigned*)malloc(imgSize);
                memset(imgData_, 0, imgSize);

                memcpy(imgData_, img_->GetData(), imgSize);

                // URHO3D_LOGERRORF("EditorModelDebug::SetModel: model <%s> pointer_img <%p> size <%u>",
                // model_->GetName().CString(), imgData_, imgSize);
            }
        }
    }

    if (model)
    {
        SubscribeToEvent(model, E_RELOADFINISHED, URHO3D_HANDLER(EditorModelDebug, HandleModelReloadFinished));

        CreateVertexInstances();

        CreateFaceInstances();

        SetMaterial(cache->GetResource<Material>("Materials/plane-collision.xml"));
    }
}

void EditorModelDebug::SetMaterial(Material* material)
{
    batches_[0].material_ = material;
    batches_[1].material_ = material;
}

void EditorModelDebug::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
    OnMarkedDirty(node_);
}

void EditorModelDebug::SelectVertex(const Frustum& frustum)
{
    // selectedIndex_.Clear();
    for (unsigned i = 0; i < numWorldTransforms_; ++i)
    {
        // Initial test using AABB
        const Matrix3x4& transform = worldTransforms_[i];
        BoundingBox transformedBoundingBox = boundingBox_.Transformed(transform);

        if (frustum.IsInside(transformedBoundingBox) > INTERSECTS)
        {
            if (!selectedIndex_.Contains(i))
            {
                selectedIndex_.Push(i);
            }
        }
    }
}

void EditorModelDebug::SelectFaces(const PODVector<IntVector2>& faces) { selectedFaces_ = faces; }

void EditorModelDebug::AddSelectedFaces(const PODVector<IntVector2>& faces, const PODVector<Vector2>& texUV, bool end)
{
    selectedFaces_.Clear();

    for (unsigned i = 0; i < faces.Size(); i++)
    {
        if (!selectedFaces_.Contains(faces[i]))
        {
            selectedFaces_.Push(faces[i]);
        }
    }

    UpdateFacesIndexes();

    // ApplyVertexCollisionMask();

    ApplyVertexUV(texUV);

    if (end)
    {
        unsigned imgSize = img_->GetWidth() * img_->GetHeight() * sizeof(unsigned);

        memset(imgData_, 0, imgSize);

        memcpy(imgData_, imgAux_->GetData(), imgSize);
    }

    // DebugList(selectedFaces_);
}

void EditorModelDebug::AddPointUV(const IntVector2& screenPosition, const Vector2& texUV, Image* image, bool end)
{
    int texWidth = texture_->GetLevelWidth(0);
    int texHeight = texture_->GetLevelHeight(0);

    int x = static_cast<int>(texUV.x_ * texWidth);
    int y = static_cast<int>(texUV.y_ * texHeight);
    // first
    if (lastUVPoint_.x_ == -1 && lastUVPoint_.y_ == -1)
    {
        lastUVPoint_ = IntVector2(x, y);
        lastScreenPoint_ = screenPosition;
        return;
    }
    else
    {
        // draw over texture
        // int scale = 2;
        // unsigned color = Color::YELLOW.ToUInt();
        // DrawLine2(lastScreenPoint_ / scale, screenPosition / scale, image, color);

        
        // Vector2 dif = Vector2(screenPosition) - Vector2(lastScreenPoint_);
        Vector2 dif = Vector2(x, y) - Vector2(lastUVPoint_);
        Vector2 lastScreenPoint;

        // URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: last <%s> curr <%s> ver <%i>", lastUVPoint_.ToString().CString(), Vector2(x, y).ToString().CString(), vertices_.Size());
        
        DrawLine(lastUVPoint_, IntVector2(x, y));

        int iter = 5;
        float step = 1.0f / iter;
        for (unsigned i = 0; i < (unsigned)iter; i++)
        {
            float p = step * i;
            Vector2 pos = dif * p + Vector2(lastUVPoint_);
            
            // URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: step <%f> pos <%f, %f>", p, pos.x_, pos.y_);

            // DrawLine(IntVector2(lastScreenPoint.x_, lastScreenPoint_.y_), IntVector2(pos.x_, pos.y_));

            // si el punto esta en algun triangulo marcar
            // ese triangulo para dibujar
            // unsigned index = FindTriangleIndex(pos);
            // if (index > -1)
            // {
            //     Vector2 v0 = uvs_.At(k + u);
            //     // Vector2 v1 = uvs_.At(k + 1);
            //     // Vector2 v2 = uvs_.At(k + 2);

            //     int xx = texWidth * v0.x_;
            //     int yy = texHeight * v0.y_;
                
            //     URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: uv <%f, %f> pos <%i, %i>", v0.x_, v0.y_, xx, yy);
            //     DrawPoint(xx, yy, img_, Color::CYAN.ToUInt());
            // }

            // DrawPoint((pos.x_ / scale), (pos.y_ / scale), image, color);

            lastScreenPoint = pos;
        }

        lastUVPoint_ = end ? IntVector2(-1, -1) : IntVector2(x, y);

        // texture_->SetData(0, 0, 0, texWidth, texHeight, img_->GetData());

        // for (unsigned i = 0; i < 9; i++)
        // {
        //     int x = (i % 3) - 1;
        //     int y = (i / 3) - 1;
        //     image->SetPixel((screenPosition.x_ / scale) + x, (screenPosition.y_ / scale) + y, Color::GREEN);
        // }

        lastScreenPoint_ = end ? IntVector2(-1, -1) : screenPosition;
    }
}

unsigned EditorModelDebug::FindTriangleIndex(const Vector2& pos)
{
    bool inside = false;
    for (unsigned k = 0; k < vertices_.Size(); k += 3)
    {
        Vector3 v0 = vertices_.At(k);
        Vector3 v1 = vertices_.At(k + 1);
        Vector3 v2 = vertices_.At(k + 2);

        inside = IsInsideTriangle(v0, v1, v2, pos);
        // URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: pos <%i> inside <%i>", k, inside);
        if (inside)
        {
            return k;
            // for (unsigned u = 0; u < 3; u++)
            // {
            //     Vector2 v0 = uvs_.At(k + u);
            //     // Vector2 v1 = uvs_.At(k + 1);
            //     // Vector2 v2 = uvs_.At(k + 2);

            //     int xx = texWidth * v0.x_;
            //     int yy = texHeight * v0.y_;
                
            //     URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: uv <%f, %f> pos <%i, %i>", v0.x_, v0.y_, xx, yy);
            //     DrawPoint(xx, yy, img_, Color::CYAN.ToUInt());
            // }
            // URHO3D_LOGERRORF("EditorModelDebug::AddPointUV: diff <%f> pos <%f, %f> inside <%i>", dif.Length(), pos.x_, pos.y_, inside);
        }
    }
    return -1;
}

bool EditorModelDebug::IsInsideTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector2& pos)
{
    float area = 0.5f * (-v1.y_ * v2.x_ + v0.y_ * (-v1.x_ + v2.x_) + v0.x_ * (v1.y_ - v2.y_) + v1.x_ * v2.y_);
    float s = 1 / (2 * area) * (v0.y_ * v2.x_ - v0.x_ * v2.y_ + (v2.y_ - v0.y_) * pos.x_ + (v0.x_ - v2.x_) * pos.y_);
    float t = 1 / (2 * area) * (v0.x_ * v1.y_ - v0.y_ * v1.x_ + (v0.y_ - v1.y_) * pos.x_ + (v1.x_ - v0.x_) * pos.y_);

    return s > 0.0f && t > 0.0f && (1.0f - s - t) > 0.0f;
}

void EditorModelDebug::DrawLine(const IntVector2& p1, const IntVector2& p2)
{
    if (!model_ || !imgData_)
        return;

    unsigned level = 0;
    int texWidth = texture_->GetLevelWidth(0);
    int texHeight = texture_->GetLevelHeight(0);

    unsigned color = brushColor_.ToUInt();
    int x1 = p1.x_;
    int y1 = p1.y_;
    int x2 = p2.x_;
    int y2 = p2.y_;
    // Iterators, counters required by algorithm
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    // Calculate line deltas
    dx = x2 - x1;
    dy = y2 - y1;
    // Create a positive copy of deltas (makes iterating easier)
    dx1 = Urho3D::Abs(dx);
    dy1 = Urho3D::Abs(dy);
    // Calculate error intervals for both axis
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;
    // The line is X-axis dominant
    if (dy1 <= dx1)
    {
        // Line is drawn left to right
        if (dx >= 0)
        {
            x = x1;
            y = y1;
            xe = x2;
        }
        else
        { // Line is drawn right to left (swap ends)
            x = x2;
            y = y2;
            xe = x1;
        }
        // Color c(img_->GetPixel(x, y));
        // if (c.a_ == 1.0f)
        img_->SetPixelInt(x, y, color); // Draw first pixel
        // Rasterize the line
        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            // Deal with octants...
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    y = y + 1;
                }
                else
                {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            // Draw pixel from line span at
            // currently rasterized position
            // Color c(img_->GetPixel(x, y));
            // if (c.a_ == 1.0f)
            img_->SetPixelInt(x, y, color);
        }
    }
    else
    { // The line is Y-axis dominant
        // Line is drawn bottom to top
        if (dy >= 0)
        {
            x = x1;
            y = y1;
            ye = y2;
        }
        else
        { // Line is drawn top to bottom
            x = x2;
            y = y2;
            ye = y1;
        }
        // Color c(img_->GetPixel(x, y));
        // if (c.a_ == 1.0f)
        img_->SetPixelInt(x, y, color);
        // Rasterize the line
        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            // Deal with octants...
            if (py <= 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    x = x + 1;
                }
                else
                {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            // Draw pixel from line span at
            // currently rasterized position
            // Color c(img_->GetPixel(x, y));
            // if (c.a_ == 1.0f)
            img_->SetPixelInt(x, y, color);
        }
    }

    texture_->SetData(level, 0, 0, texWidth, texHeight, img_->GetData());
}

void EditorModelDebug::DrawLine2(const IntVector2& p1, const IntVector2& p2, Image* img, unsigned color)
{
    if (!model_ || !imgData_)
        return;

    // URHO3D_LOGERRORF("EditorModelDebug::DrawLine2: p1 <%i, %i> p2 <%i, %i>", p1.x_, p1.y_, p2.x_, p2.y_);

    unsigned level = 0;
    // int texWidth = width;
    // int texHeight = height;
    int texWidth = img->GetWidth();
    int texHeight = img->GetHeight();

    // unsigned color = Color::RED.ToUInt();
    int x1 = p1.x_;
    int y1 = p1.y_;
    int x2 = p2.x_;
    int y2 = p2.y_;
    // Iterators, counters required by algorithm
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    // Calculate line deltas
    dx = x2 - x1;
    dy = y2 - y1;
    // Create a positive copy of deltas (makes iterating easier)
    dx1 = Urho3D::Abs(dx);
    dy1 = Urho3D::Abs(dy);
    // Calculate error intervals for both axis
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;
    // The line is X-axis dominant
    if (dy1 <= dx1)
    {
        // Line is drawn left to right
        if (dx >= 0)
        {
            x = x1;
            y = y1;
            xe = x2;
        }
        else
        { // Line is drawn right to left (swap ends)
            x = x2;
            y = y2;
            xe = x1;
        }
        // Color c(img_->GetPixel(x, y));
        // if (c.a_ == 1.0f)
        img->SetPixelInt(x, y, color); // Draw first pixel
        // Rasterize the line
        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            // Deal with octants...
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    y = y + 1;
                }
                else
                {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            // Draw pixel from line span at
            // currently rasterized position
            // Color c(img_->GetPixel(x, y));
            // if (c.a_ == 1.0f)
            img->SetPixelInt(x, y, color);
        }
    }
    else
    { // The line is Y-axis dominant
        // Line is drawn bottom to top
        if (dy >= 0)
        {
            x = x1;
            y = y1;
            ye = y2;
        }
        else
        { // Line is drawn top to bottom
            x = x2;
            y = y2;
            ye = y1;
        }
        // Color c(img_->GetPixel(x, y));
        // if (c.a_ == 1.0f)
        img->SetPixelInt(x, y, color);
        // Rasterize the line
        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            // Deal with octants...
            if (py <= 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    x = x + 1;
                }
                else
                {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            // Draw pixel from line span at
            // currently rasterized position
            // Color c(img_->GetPixel(x, y));
            // if (c.a_ == 1.0f)
            img->SetPixelInt(x, y, color);
        }
    }

    // texture_->SetData(level, 0, 0, texWidth, texHeight, img_->GetData());
}

void EditorModelDebug::DrawFaces(const PODVector<IntVector2>& faces)
{
    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    unsigned vertexSize = vertexBuffer->GetVertexSize();

    const Vector<Vector<SharedPtr<Geometry>>>& geoms = model_->GetGeometries();
    //    URHO3D_LOGERRORF("buffer vertex <%u> index <%u> geom <%u>", vb.Size(), ib.Size(), geoms.Size());
    //    for(unsigned i = 0; i < geoms.Size(); i++)
    //    {
    //        const Vector<SharedPtr<Geometry>>& gs = geoms[i];
    //        SharedPtr<Geometry> g = geoms[i][0];

    //        // VertexBuffer* vb = g->GetVertexBuffer(0);
    //        // URHO3D_LOGERRORF("geom <%u> vb <%u> ib <%u>", i, vb->GetVertexCount(),
    //        g->GetIndexBuffer()->GetIndexCount()); URHO3D_LOGERRORF("geom <%u> vb start <%u> vb count <%u> ib start
    //        <%u> ib count <%u> ", i,
    //                         g->GetVertexStart(), g->GetVertexCount(), g->GetIndexStart(), g->GetIndexCount());
    //    }

    IndexBuffer* indexBuffer = ib.At(0);
    // unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();

    // FIXME here isn't required to lock, its only for reading
    //    unsigned char* dstData = static_cast<unsigned char*>(vertexBuffer->Lock(0, vertexBuffer->GetVertexCount(),
    //    true));
    unsigned char* dstData = vertexBuffer->GetShadowDataShared();
    if (!dstData)
        return;

    Color color = GetFaceColor();
    DebugRenderer* debugRenderer = node_->GetScene()->GetComponent<DebugRenderer>();
    for (unsigned i = 0; i < faces.Size(); i++)
    {
        const IntVector2& face = faces.At(i);
        if (face.y_ != M_MAX_UNSIGNED)
        {
            SharedPtr<Geometry> g = geoms[face.x_][0];

            unsigned short* index = ((unsigned short*)&indexData[0]) + g->GetIndexStart() + face.y_;

            currentFace_ = face.y_ / 3;
            currentIndex_.x_ = index[0];
            currentIndex_.y_ = index[1];
            currentIndex_.z_ = index[2];

            const Vector3& v0 = *((const Vector3*)(&dstData[index[0] * vertexSize]));
            const Vector3& v1 = *((const Vector3*)(&dstData[index[1] * vertexSize]));
            const Vector3& v2 = *((const Vector3*)(&dstData[index[2] * vertexSize]));

            const Matrix3x4& transform = node_->GetWorldTransform();

            debugRenderer->AddTriangle(transform * v0, transform * v1, transform * v2, color);

            //            float d0 = (transform * v0).DistanceToPoint(hitPosition_);
            //            float d1 = (transform * v1).DistanceToPoint(hitPosition_);
            //            float d2 = (transform * v2).DistanceToPoint(hitPosition_);

            Sphere sphere;
            sphere.radius_ = 0.02f;
            Color sphereColor;
            sphereColor.FromHSL(39.0f, 100.0f, 50.0f);

            // if(d0 < d1 && d0 < d2)
            {
                sphere.center_ = (transform * v0);
                debugRenderer->AddSphere(sphere, sphereColor);
            }
            // else if(d1 < d0 && d1 < d2)
            {
                sphere.center_ = (transform * v1);
                debugRenderer->AddSphere(sphere, sphereColor);
            }
            // else
            {
                sphere.center_ = (transform * v2);
                debugRenderer->AddSphere(sphere, sphereColor);
            }
        }
    }
    // vertexBuffer->Unlock();

    // debugRenderer->AddTriangleMesh(&vertexData[0], vertexSize, &indexData[0], indexSize, geom->GetIndexStart(),
    // geom->GetIndexCount(), transform, color);
}

void EditorModelDebug::UpdateFacesIndexes()
{
    if (!model_)
        return;

    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    // unsigned vertexSize = vertexBuffer->GetVertexSize();

    IndexBuffer* indexBuffer = ib.At(0);
    // unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();

    const Vector<Vector<SharedPtr<Geometry>>>& geoms = model_->GetGeometries();

    //    unsigned char* dstData = static_cast<unsigned char*>(vertexBuffer->Lock(0, vertexBuffer->GetVertexCount(),
    //    true)); if(!dstData)
    //        return;

    for (unsigned i = 0; i < selectedFaces_.Size(); i++)
    {
        IntVector2 face = selectedFaces_.At(i);
        if (face.y_ != M_MAX_UNSIGNED)
        {
            SharedPtr<Geometry> g = geoms[face.x_][0];
            unsigned short* index = ((unsigned short*)&indexData[0]) + g->GetIndexStart() + face.y_;

            for (unsigned j = 0; j < 3; j++)
            {
                if (!selectedIndex_.Contains(index[j]))
                {
                    selectedIndex_.Push(index[j]);
                }
            }

            IntVector3 faceIndex(index[0], index[1], index[2]);
            selectedFacesIndex_.Push(faceIndex);
        }
    }
    // vertexBuffer->Unlock();
}

void EditorModelDebug::ApplyVertexCollisionMask()
{
    if (!model_)
        return;

    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    const PODVector<VertexElement>& ve = vertexBuffer->GetElements();

    const VertexElement* vertexObjectIndex = VertexBuffer::GetElement(ve, TYPE_INT, SEM_OBJECTINDEX);
    const VertexElement* vertexUV = VertexBuffer::GetElement(ve, TYPE_VECTOR2, SEM_TEXCOORD);
    if (!vertexObjectIndex || !vertexUV)
    {
        URHO3D_LOGERRORF("vertex element not found!");
        return;
    }
    unsigned char* dstData = static_cast<unsigned char*>(vertexBuffer->Lock(0, vertexBuffer->GetVertexCount(), true));
    if (dstData)
    {
        unsigned vertexSize = vertexBuffer->GetVertexSize();
        unsigned size = ELEMENT_TYPESIZES[vertexObjectIndex->type_];
        // for(unsigned i = 0; i < vertexBuffer->GetVertexCount(); i++)
        for (unsigned i = 0; i < selectedIndex_.Size(); i++)
        {
            int s = selectedIndex_[i];
            //            unsigned int val = *((unsigned int*)&dstData[s * vertexSize + vertexElement->offset_]);
            //            val = val | GetCollisionMask(vertexMaskType_);
            unsigned val = GetCollisionMask(vertexMaskType_);

            memcpy(&dstData[(unsigned)s * vertexSize + vertexObjectIndex->offset_], &val, size);
        }

        vertexBuffer->Unlock();
    }
}

void EditorModelDebug::ApplyVertexUV(const PODVector<Vector2>& texUV)
{
    if (!model_ || texUV.Size() == 0 || !imgData_)
        return;

    int texWidth = texture_->GetLevelWidth(0);
    int texHeight = texture_->GetLevelHeight(0);

    unsigned* data = (unsigned*)malloc(brushSize_ * brushSize_ * sizeof(unsigned));
    memset(data, 0, brushSize_ * brushSize_ * sizeof(unsigned));

    Vector2 uv = texUV.At(0);
    unsigned xx = static_cast<int>(uv.x_ * texWidth);
    unsigned yy = static_cast<int>(uv.y_ * texHeight);

    // ARGB
    // unsigned c = (0xAA << 24) | (0x00 << 16) | (0xFF << 8) | 0x00;
    unsigned c = brushColor_.ToUInt();
    unsigned t = (0xfF << 24) | (0x00 << 16) | (0x00 << 8) | 0xFF;
    for (unsigned ii = 0; ii < brushSize_; ii++)
    {
        for (unsigned jj = 0; jj < brushSize_; jj++)
        {
            int x = jj - brushSize_ / 2;
            int y = brushSize_ / 2 - ii;

            int sumsq = x * x + y * y;
            int r = brushSize_ / 2;

            int offsetx = xx - (brushSize_ / 2) + jj;
            int offsety = yy - (brushSize_ / 2) + ii;

            if ((sumsq < r * r))
            {
                unsigned p = 0;
                if ((offsetx >= 0) && (offsety >= 0) && (offsety * texWidth + offsetx < texWidth * texHeight))
                    p = imgData_[offsety * texWidth + offsetx];

                Color c2(p);
                Color c1(brushColor_);

                c = c1.Blend(c2).ToUInt();

                data[ii * brushSize_ + jj] = (unsigned)c;

                // if ((offsetx >= 0) && (offsety >=0))
                //     imgData_[offsety * texWidth + offsetx] = (unsigned)c;
            }
            else
            {
                unsigned p = 0;
                // if ((offsetx >= 0) && (offsety >=0) && (offsetx * texWidth + offsety < texWidth * texHeight))
                //     p = imgData_[offsety * texWidth + offsetx];
                if ((offsetx >= 0) && (offsety >= 0) && (offsetx * texWidth + offsety < texWidth * texHeight))
                    p = img_->GetPixel(offsetx, offsety).ToUInt();

                data[ii * brushSize_ + jj] = (unsigned)p;
            }
        }
    }

    int halfBrushSize = brushSize_ / 2;
    int centerx = xx - halfBrushSize;
    int centery = yy - halfBrushSize;

    // check border size
    if (centerx < 0)
        centerx = 0;

    if (centery < 0)
        centery = 0;

    if (centerx + brushSize_ > texWidth)
        centerx = texWidth - brushSize_;

    if (centery + brushSize_ > texHeight)
        centery = texHeight - brushSize_;

    if (centerx < 0 || centerx + brushSize_ > texWidth || centery < 0 || centery + brushSize_ > texHeight ||
        brushSize_ <= 0 || brushSize_ <= 0)
    {
        URHO3D_LOGERRORF("EditorModelDebug::ApplyVertexUV: center <%i, %i>", centerx, centery);
    }

    texture_->SetData(0, centerx, centery, brushSize_, brushSize_, data);

    Image imgData(context_);
    imgData.SetSize(brushSize_, brushSize_, 4);
    imgData.SetData((const unsigned char*)data);

    bool subImage =
        imgAux_->SetSubimage(&imgData, IntRect(centerx, centery, centerx + brushSize_, centery + brushSize_));

    free(data);
}

void EditorModelDebug::AddVertexElement(unsigned vertexIndex, unsigned vertexElementIndex)
{
    // Resize VertexElement anterior
    // URHO3D_LOGERRORF("addindex <%i> vertexindex <%i>", vertexElementIndex, vertexIndex);
    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();

    VertexBuffer* vertexBuffer = vb.At(vertexIndex);
    PODVector<VertexElement> veOriginal = vertexBuffer->GetElements();
    PODVector<VertexElement> ves = veOriginal;

    VertexElementType type = TYPE_INT;
    VertexElementSemantic semantic = SEM_OBJECTINDEX;
    ves.Insert(vertexElementIndex + 1, VertexElement(type, semantic));

    // Save data before resizing
    SharedArrayPtr<unsigned char> vertexData;
    vertexData = vertexBuffer->GetShadowDataShared();

    unsigned vertexSizeOld = vertexBuffer->GetVertexSize();
    // URHO3D_LOGERRORF("before size <%i> count <%i> size <%i> ve <%i>", sizeof(vertexData.Get()),
    // vertexBuffer->GetVertexCount(), vertexBuffer->GetVertexSize(), veOriginal.Size());

    vertexBuffer->SetSize(vertexBuffer->GetVertexCount(), ves);
    ves = vertexBuffer->GetElements();

    unsigned vertexSize = vertexBuffer->GetVertexSize();
    // URHO3D_LOGERRORF("after size <%i> count <%i> size <%i>  ve <%i>", sizeof(vertexBuffer->GetShadowData()),
    // vertexBuffer->GetVertexCount(), vertexBuffer->GetVertexSize(), ves.Size());
    URHO3D_LOGERRORF("vertex element sizes: old <%i> new <%i>", veOriginal.Size(), ves.Size());

    unsigned char* srcData = reinterpret_cast<unsigned char*>(&vertexData[0]);
    unsigned char* dstData = static_cast<unsigned char*>(vertexBuffer->Lock(0, vertexBuffer->GetVertexCount(), true));
    if (dstData)
    {
        // const PODVector<VertexElement>& ve = vertexBuffer->GetElements();
        for (unsigned i = 0; i < vertexBuffer->GetVertexCount(); i++)
        {
            for (unsigned j = 0; j < veOriginal.Size(); j++)
            {
                const VertexElement& el = veOriginal.At(j);
                unsigned size = ELEMENT_TYPESIZES[el.type_];

                const VertexElement* elNew = VertexBuffer::GetElement(ves, el.type_, el.semantic_);
                if (elNew)
                {
                    // URHO3D_LOGERRORF("coping type <%i> semantic <%i> size <%i> offset <%u, %u>", el.type_,
                    // el.semantic_, size, el.offset_, elNew->offset_);
                    memcpy(&dstData[i * vertexSize + elNew->offset_], &srcData[i * vertexSizeOld + el.offset_], size);
                }
                else
                {
                    URHO3D_LOGERRORF("vertex element not found, type <%i> semantic <%i>", el.type_, el.semantic_);
                }
            }

            // init new vertex element
            const VertexElement* elNew = VertexBuffer::GetElement(ves, TYPE_INT, SEM_OBJECTINDEX);
            unsigned size = ELEMENT_TYPESIZES[TYPE_INT];
            if (elNew)
            {
                (void)size;
                // memcpy(&dstData[i * vertexSize + elNew->offset_], NONE, size);
                memset(&dstData[i * vertexSize + elNew->offset_], NONE, size);
            }
        }
        vertexBuffer->Unlock();
    }
}

void EditorModelDebug::SaveModel()
{
    ApplyVertexCollisionMask();

    // String fileName = GetSubsystem<FileSystem>()->GetProgramDir() + "MeshTest.mdl";
    URHO3D_LOGERRORF("file name <%s>", model_->GetName().CString());
    String fileName = GetSubsystem<FileSystem>()->GetProgramDir() + "Data/" + model_->GetName() + "-collision";

    File file(context_, fileName, FILE_WRITE);
    if (model_->Save(file))
        URHO3D_LOGERRORF("file saved name <%s>", fileName.CString());
    else
        URHO3D_LOGERRORF("error saving name <%s>", fileName.CString());
}

void EditorModelDebug::SelectAll()
{
    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();

    // VertexBuffer* vertexBuffer = vb.At(0);
    // unsigned vertexSize = vertexBuffer->GetVertexSize();

    IndexBuffer* indexBuffer = ib.At(0);
    unsigned indexCount = indexBuffer->GetIndexCount();
    // unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();
    if (!indexData)
        return;

    // const Vector<Vector<SharedPtr<Geometry> > >& geoms = model_->GetGeometries();

    selectedIndex_.Clear();
    selectedFacesIndex_.Clear();
    for (unsigned i = 0; i < indexCount; i++)
    {
        unsigned short* index = ((unsigned short*)&indexData[0]) + i;
        // for(unsigned j = 0; j < 3; j++)
        {
            // if(!selectedIndex_.Contains(index[0]))
            {
                selectedIndex_.Push(index[0]);
            }
        }
    }

    ApplyVertexCollisionMask();
}

void EditorModelDebug::OnWorldBoundingBoxUpdate()
{
    if (model_)
    {
        worldBoundingBox_ = model_->GetBoundingBox();
        const Matrix3x4& transform = node_->GetWorldTransform();
        worldBoundingBox_.Transform(transform);
    }
}

void EditorModelDebug::HandleModelReloadFinished(StringHash eventType, VariantMap& eventData)
{
    Model* currentModel = model_;
    model_.Reset(); // Set null to allow to be re-set
    SetModel(currentModel);
}

Color EditorModelDebug::GetFaceColor()
{
    VertexCollisionMask type = GetCollisionMask(vertexMaskType_);
    return GetFaceColor(type);
}

Color EditorModelDebug::GetFaceColor(VertexCollisionMaskFlags mask)
{
    Color color(Color::BLUE);
    //    if((mask & TRACK_ROAD) == TRACK_ROAD)
    //        color = Color::GREEN;
    //    else if((mask & TRACK_BORDER) == TRACK_BORDER)
    //        color = Color::YELLOW;
    //    else if((mask & OFFTRACK_WEAK) == OFFTRACK_WEAK)
    //        color = Color::RED;
    //    else if((mask & OFFTRACK_HEAVY) == OFFTRACK_HEAVY)
    //        color = Color::BLACK;
    if ((mask & TRACK_ROAD))
        color = Color::GREEN;
    else if ((mask & TRACK_BORDER))
        color = Color::YELLOW;
    else if ((mask & OFFTRACK_WEAK))
        color = Color::RED;
    else if ((mask & OFFTRACK_HEAVY))
        color = Color::BLACK;

    color.a_ = 0.7f;
    return color;
}

VertexCollisionMask EditorModelDebug::GetCollisionMask(unsigned int mask)
{
    VertexCollisionMask maskType = NONE;
    switch (mask)
    {
    case 0:
        maskType = NONE;
        break;
    case 1:
        maskType = TRACK_ROAD;
        break;
    case 2:
        maskType = TRACK_BORDER;
        break;
    case 3:
        maskType = OFFTRACK_WEAK;
        break;
    case 4:
        maskType = OFFTRACK_HEAVY;
        break;
    }
    return maskType;
}

void EditorModelDebug::CreateVertexInstances()
{
    /// set up
    primitiveType_ = TRIANGLE_LIST;
    transform_ = Matrix3x4::IDENTITY;

    if (model_->GetVertexBuffers().Size())
    {
        VertexBuffer* vb = model_->GetVertexBuffers().At(0);

        unsigned vertexSize = vb->GetVertexSize();
        unsigned totalVerticesReal = vb->GetVertexCount();

        URHO3D_LOGERRORF("editormodeldebug: vertices <%u>", totalVerticesReal);
        unsigned totalVertices = 1;

        const SharedArrayPtr<unsigned char>& vertexData = vb->GetShadowDataShared();
        const auto* srcData = (const unsigned char*)&vertexData[0];

        unsigned positionOffset = VertexBuffer::GetElementOffset(vb->GetElements(), TYPE_VECTOR3, SEM_POSITION);

        vertexElements_.Clear();
        vertexElements_.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION, 0, false));
        vertexElements_.Push(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, 0, false));
        VertexBuffer::UpdateOffsets(vertexElements_);

        unsigned indexLength = sizeof(cubeIndex) / sizeof(cubeIndex[0]);
        unsigned indexCount = totalVertices * indexLength;

        unsigned size = 8;

        indexBuffer_->SetShadowed(true);
        // indexCount > 0xFFFF
        indexBuffer_->SetSize(indexCount, true);

        unsigned* indexDest = (unsigned*)indexBuffer_->Lock(0, indexCount);

        vertexBuffer_->SetShadowed(true);
        vertexBuffer_->SetSize(totalVertices * size, vertexElements_, true);

        auto* dest = (unsigned char*)vertexBuffer_->Lock(0, totalVertices * size, true);

        if (dest && indexDest)
        {
            unsigned color = Color::GREEN.ToUInt();
            unsigned indexOffset = 0;

            for (unsigned i = 0; i < totalVertices; i++)
            {
                // Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                Vector3 position = Vector3::ZERO;
                for (unsigned j = 0; j < size; j++)
                {
                    *((Vector3*)dest) = position + Vector3(cubeVertices + j * 3) * vertexScale_;
                    dest += sizeof(Vector3);

                    *((unsigned*)dest) = color;
                    dest += sizeof(unsigned);
                }

                for (unsigned j = 0; j < indexLength; j++)
                {
                    unsigned index = indexOffset + cubeIndex[j];
                    indexDest[indexOffset + j] = index;
                }
                indexOffset += indexLength;
            }

            worldTransforms_.Resize(totalVerticesReal);
            numWorldTransforms_ = totalVerticesReal;
            batches_[0].worldTransform_ = &worldTransforms_[0];
            batches_[0].numWorldTransforms_ = totalVerticesReal;
            batches_[0].instancingData_ = new float[4 * totalVerticesReal];

            unsigned offset = 0;
            unsigned char* instanceData = static_cast<unsigned char*>(batches_[0].instancingData_);
            for (unsigned i = 0; i < totalVerticesReal; i++)
            {
                memcpy(instanceData + offset, Vector4(1.0f, 0.2f, 0.3f, 0.4f).Data(), sizeof(Vector4));
                offset += sizeof(Vector4);
            }

            for (unsigned i = 0; i < totalVerticesReal; i++)
            {
                Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                vertexOffset_.Push(position);
            }

            geometry_->SetVertexBuffer(0, vertexBuffer_);
            geometry_->SetIndexBuffer(indexBuffer_);
            geometry_->SetDrawRange(primitiveType_, 0, indexOffset, 0, totalVertices * size);

            vertexBuffer_->Unlock();
            indexBuffer_->Unlock();

            Vector3 min(cubeVertices + 4 * 3);
            Vector3 max(cubeVertices + 2 * 3);

            //               URHO3D_LOGERRORF("vector min <%f, %f, %f>", min.x_, min.y_, min.z_);
            //               URHO3D_LOGERRORF("vector max <%f, %f, %f>", max.x_, max.y_, max.z_);

            SetBoundingBox(BoundingBox(min * vertexScale_, max * vertexScale_));
        }
    }
}

void EditorModelDebug::CreateFaceInstances()
{
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();
    IndexBuffer* indexBuffer = ib.At(0);

    vertexElements_.Clear();
    vertexElements_.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION, 0, false));
    vertexElements_.Push(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, 0, false));
    VertexBuffer::UpdateOffsets(vertexElements_);

    unsigned totalVertices = 1;
    unsigned totalVerticesReal = 1;

    unsigned indexLength = 3;
    unsigned indexCount = totalVertices * indexLength;
    unsigned size = 3;

    indexBufferFaces_->SetShadowed(true);
    indexBufferFaces_->SetSize(indexCount, true);

    unsigned* indexDest = (unsigned*)indexBufferFaces_->Lock(0, indexCount);

    vertexBufferFaces_->SetShadowed(true);
    vertexBufferFaces_->SetSize(totalVertices * size, vertexElements_, true);

    unsigned char* dest = (unsigned char*)vertexBufferFaces_->Lock(0, totalVertices * size, true);

    if (dest && indexDest)
    {
        float scale = 1.00f;
        unsigned color = Color::YELLOW.ToUInt();
        unsigned indexOffset = 0;

        for (unsigned i = 0; i < totalVertices; i++)
        {
            Vector3 position = Vector3::ZERO;
            for (unsigned j = 0; j < size; j++)
            {
                *((Vector3*)dest) = position + Vector3(cubeVertices + j * 3) * scale;
                dest += sizeof(Vector3);

                *((unsigned*)dest) = color;
                dest += sizeof(unsigned);
            }

            for (unsigned j = 0; j < indexLength; j++)
            {
                unsigned index = indexOffset + cubeIndex[j];
                indexDest[indexOffset + j] = index;
            }
            indexOffset += indexLength;
        }

        geometryFaces_->SetVertexBuffer(0, vertexBufferFaces_);
        geometryFaces_->SetIndexBuffer(indexBufferFaces_);
        geometryFaces_->SetDrawRange(primitiveType_, 0, indexOffset, 0, totalVertices * size);

        vertexBufferFaces_->Unlock();
        indexBufferFaces_->Unlock();

        unsigned count = indexBuffer->GetIndexCount();

        CalcMatrixFaces();

        numWorldTransformsFaces_ = count / 3;
        batches_[1].worldTransform_ = &worldTransformsFaces_[0];
        batches_[1].numWorldTransforms_ = count / 3;
        batches_[1].instancingData_ = new float[4 * count / 3];

        UpdateFacesColor();
    }
}

void EditorModelDebug::UpdateFacesColor()
{
    if (!model_)
        return;

    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();
    IndexBuffer* indexBuffer = ib.At(0);

    unsigned count = indexBuffer->GetIndexCount();

    unsigned offset = 0;
    unsigned char* instanceData = static_cast<unsigned char*>(batches_[1].instancingData_);

    // count = 6;
    for (unsigned i = 0; i < count; i += 3)
    {
        VertexCollisionMask collisionMask = GetFaceCollisionMask(i);

        memcpy(instanceData + offset, GetFaceColor(collisionMask).Data(), sizeof(Vector4));
        // memcpy(instanceData + offset, Color::GREEN.Data(), sizeof(Vector4));
        offset += sizeof(Vector4);
    }
}

void EditorModelDebug::CalcMatrixFaces()
{
    if (!model_)
        return;

    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();
    IndexBuffer* indexBuffer = ib.At(0);

    float scale = 1.0f;
    // create matrix transform for each face
    Vector3 p1, p2, p3;
    p1 = Vector3(cubeVertices + 0 * 3) * scale;
    p2 = Vector3(cubeVertices + 1 * 3) * scale;
    p3 = Vector3(cubeVertices + 2 * 3) * scale;

    //    URHO3D_LOGERRORF("p1 <%f, %f, %f>", p1.x_, p1.y_, p1.z_);
    //    URHO3D_LOGERRORF("p2 <%f, %f, %f>", p2.x_, p2.y_, p2.z_);
    //    URHO3D_LOGERRORF("p3 <%f, %f, %f>", p3.x_, p3.y_, p3.z_);

    unsigned count = indexBuffer->GetIndexCount();
    // count = 3;
    worldTransformsFaces_.Resize(count / 3);
    worldTransformsFaces_.Clear();
    for (unsigned i = 0; i < count; i += 3)
    {
        Vector3 q1, q2, q3;
        // float vertexScale = node_->GetScale().x_;
        Matrix3x4 transform = node_->GetTransform();
        Vector3 nodeTrans = transform.Translation();
        nodeTrans += Vector3(0.0f, 0.01f, 0.0f);
        transform.SetTranslation(nodeTrans);

        Vector<Vector3> r = GetFacePoints(i);
        q1 = transform * r[0];
        q2 = transform * r[1];
        q3 = transform * r[2];

        //            URHO3D_LOGERRORF("q1 <%f, %f, %f>", q1.x_, q1.y_, q1.z_);
        //            URHO3D_LOGERRORF("q2 <%f, %f, %f>", q2.x_, q2.y_, q2.z_);
        //            URHO3D_LOGERRORF("q3 <%f, %f, %f>", q3.x_, q3.y_, q3.z_);

        Matrix4 matQ(q1.x_, q1.y_, q1.z_, 1.0f, q2.x_, q2.y_, q2.z_, 1.0f, q3.x_, q3.y_, q3.z_, 1.0f, 0.0f, 0.0f, 0.0f,
                     1.0f);

        Matrix4 matP(p1.x_, p1.y_, p1.z_, 1.0f, p2.x_, p2.y_, p2.z_, 1.0f, p3.x_, p3.y_, p3.z_, 1.0f, 0.0f, 0.0f, 0.0f,
                     1.0f);

        Matrix4 t = matQ.Transpose() * matP.Transpose().Inverse();
        //        Vector4 q11 = t * Vector4(p1, 1.0f);
        //        Vector4 q22 = t * Vector4(p2, 1.0f);
        //        Vector4 q33 = t * Vector4(p3, 1.0f);

        //            URHO3D_LOGERRORF("q11 <%f, %f, %f, %f>", q11.x_, q11.y_, q11.z_, q11.w_);
        //            URHO3D_LOGERRORF("q22 <%f, %f, %f, %f>", q22.x_, q22.y_, q22.z_, q22.w_);
        //            URHO3D_LOGERRORF("q33 <%f, %f, %f, %f>", q33.x_, q33.y_, q33.z_, q33.w_);

        worldTransformsFaces_.Push(Matrix3x4(t));
    }
}

Vector<Vector3> EditorModelDebug::GetFacePoints(unsigned face)
{
    Vector<Vector3> result;

    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    unsigned vertexSize = vertexBuffer->GetVertexSize();

    IndexBuffer* indexBuffer = ib.At(0);
    // unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();
    // unsigned indexStart = indexBuffer->GetS

    // FIXME here isn't required to lock, its only for reading
    unsigned char* vertexData = vertexBuffer->GetShadowData();
    if (!vertexData)
        return result;

    if (face != M_MAX_UNSIGNED)
    {
        unsigned short* index = ((unsigned short*)&indexData[0]) + face;

        result.Push(*((const Vector3*)(&vertexData[index[0] * vertexSize])));
        result.Push(*((const Vector3*)(&vertexData[index[1] * vertexSize])));
        result.Push(*((const Vector3*)(&vertexData[index[2] * vertexSize])));
    }

    return result;
}

VertexCollisionMask EditorModelDebug::GetFaceCollisionMask(unsigned face)
{
    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    unsigned vertexSize = vertexBuffer->GetVertexSize();

    IndexBuffer* indexBuffer = ib.At(0);
    // unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();
    // unsigned indexStart = indexBuffer->GetS
    const Vector<Vector<SharedPtr<Geometry>>>& geoms = model_->GetGeometries();
    // FIXME here isn't required to lock, its only for reading
    unsigned char* vertexData = vertexBuffer->GetShadowData();
    if (!vertexData)
        return NONE;

    const PODVector<VertexElement>& ve = vertexBuffer->GetElements();
    const VertexElement* vertexElement = VertexBuffer::GetElement(ve, TYPE_INT, SEM_OBJECTINDEX);
    if (!vertexElement)
    {
        // URHO3D_LOGERRORF("vertex element for collision not found!");
        return NONE;
    }

    if (face != M_MAX_UNSIGNED)
    {
        SharedPtr<Geometry> g = geoms[0][0];

        // FIXME verificar el tipo de los indices
        unsigned short* index = ((unsigned short*)&indexData[0]) + g->GetIndexStart() + face;
        unsigned short i = index[0];
        unsigned short j = index[1];
        unsigned short k = index[2];
        //        const unsigned& c0 = *((const unsigned*)(&vertexData[index[0] * vertexSize +
        //        vertexElement->offset_])); const unsigned& c1 = *((const unsigned*)(&vertexData[index[1] * vertexSize
        //        + vertexElement->offset_])); const unsigned& c2 = *((const unsigned*)(&vertexData[index[2] *
        //        vertexSize + vertexElement->offset_]));

        const VertexCollisionMaskFlags& c0 =
            *((const VertexCollisionMaskFlags*)(&vertexData[i * vertexSize + vertexElement->offset_]));
        const VertexCollisionMaskFlags& c1 =
            *((const VertexCollisionMaskFlags*)(&vertexData[j * vertexSize + vertexElement->offset_]));
        const VertexCollisionMaskFlags& c2 =
            *((const VertexCollisionMaskFlags*)(&vertexData[k * vertexSize + vertexElement->offset_]));

        // color = GetFaceColor(c0);
        if ((c0 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY)) &&
            (c1 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY)) &&
            (c2 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY)) && c0 == c1 && c0 == c2 && c1 == c2)
        {
            return (VertexCollisionMask)c0;
        }
    }

    return NONE;
}

void EditorModelDebug::DebugList(const PODVector<IntVector2>& list)
{
    //    char b[1024 * 8];
    //    memset(b, 0, sizeof(b));
    //    unsigned offset = 0;
    //    for (unsigned i = 0; i < list.Size(); i++)
    //    {
    //        offset += sprintf(b + offset, "[%u,%u] ", list[i].x_, list[i].y_);
    //    }

    //    URHO3D_LOGERRORF("debug list <%s> size <%u>", b, list.Size());
}

void EditorModelDebug::SaveImage(const String& name) { img_->SavePNG(name); }

bool EditorModelDebug::SetSize(int width, int height, bool threaded)
{
    if (width <= 0 || height <= 0)
        return false;

    width_ = width;
    height_ = height;

    CalculateViewport();
    return true;
}

void EditorModelDebug::CalculateViewport()
{
    // Add half pixel offset due to 3D frustum culling
    scaleX_ = 0.5f * width_;
    scaleY_ = -0.5f * height_;
    offsetX_ = 0.5f * width_ + 0.5f;
    offsetY_ = 0.5f * height_ + 0.5f;
    projOffsetScaleX_ = projection_.m00_ * scaleX_;
    projOffsetScaleY_ = projection_.m11_ * scaleY_;
}

void EditorModelDebug::CheckVertexVisibility(Camera* camera, Image* image)
{
    if (!model_)
        return;

    // Geometries
    const Vector<Vector<SharedPtr<Geometry>>>& geoms = model_->GetGeometries();
    SharedPtr<Geometry> geometry = geoms[0][0];

    // Vertex Buffer
    const Vector<SharedPtr<VertexBuffer>>& vb = model_->GetVertexBuffers();

    VertexBuffer* vertexBuffer = vb.At(0);
    const PODVector<VertexElement>& ve = vertexBuffer->GetElements();

    const VertexElement* vPos = VertexBuffer::GetElement(ve, TYPE_VECTOR3, SEM_POSITION);
    const VertexElement* vUV = VertexBuffer::GetElement(ve, TYPE_VECTOR2, SEM_TEXCOORD);
    if (!vPos || !vUV)
    {
        URHO3D_LOGERRORF("vertex element not found!");
        return;
    }

    // URHO3D_LOGERRORF("EditorModelDebug::CheckVertexVisibility: vertex count <%u> size <%u>",
    // vertexBuffer->GetVertexCount(), sizeof(unsigned short));

    // Index Buffer
    const Vector<SharedPtr<IndexBuffer>>& ib = model_->GetIndexBuffers();
    IndexBuffer* indexBuffer = ib.At(0);
    unsigned indexSize = indexBuffer->GetIndexSize();
    unsigned char* indexData = indexBuffer->GetShadowData();

    Matrix3x4 modelMatrix = node_->GetWorldTransform();

    Matrix3x4 view = camera->GetView();
    projection_ = camera->GetProjection();
    Matrix4 viewProj = projection_ * view;

    float nearClip = camera->GetNearClip();
    float farClip = camera->GetFarClip();
    // reverseCulling_ = camera->GetReverseCulling();

    Matrix4 modelViewProj = viewProj * modelMatrix;

    Graphics* graphics = GetSubsystem<Graphics>();
    SetSize(graphics->GetWidth(), graphics->GetHeight(), false);

    numTriangles_ = 0;

    Vector4 vertices[64 * 3];
    Vector2 uvs[3];

    vertices_.Clear();
    uvs_.Clear();

    unsigned char* vertexData =
        static_cast<unsigned char*>(vertexBuffer->Lock(0, vertexBuffer->GetVertexCount(), true));
    if (vertexData)
    {
        unsigned positionOffset = VertexBuffer::GetElementOffset(ve, TYPE_VECTOR3, SEM_POSITION);
        unsigned uvOffset = VertexBuffer::GetElementOffset(ve, TYPE_VECTOR2, SEM_TEXCOORD);
        unsigned vertexSize = vertexBuffer->GetVertexSize();

        // ver de donde sacar indexStart
        unsigned indexStart = geometry->GetIndexStart();
        unsigned indexCount = indexBuffer->GetIndexCount();

        // // 16-bit indices
        if (indexSize == sizeof(unsigned short))
        {
            const unsigned short* indices = ((const unsigned short*)indexData) + indexStart;
            const unsigned short* indicesEnd = indices + indexCount;

            while (indices < indicesEnd)
            {
                const Vector3& v0 = *((const Vector3*)(&vertexData[indices[0] * vertexSize]));
                const Vector3& v1 = *((const Vector3*)(&vertexData[indices[1] * vertexSize]));
                const Vector3& v2 = *((const Vector3*)(&vertexData[indices[2] * vertexSize]));

                const Vector2& uv0 = *((const Vector2*)(&vertexData[indices[0] * vertexSize + uvOffset]));
                const Vector2& uv1 = *((const Vector2*)(&vertexData[indices[1] * vertexSize + uvOffset]));
                const Vector2& uv2 = *((const Vector2*)(&vertexData[indices[2] * vertexSize + uvOffset]));

                uvs[0] = uv0;
                uvs[1] = uv1;
                uvs[2] = uv2;

                vertices[0] = ModelTransform(modelViewProj, v0);
                vertices[1] = ModelTransform(modelViewProj, v1);
                vertices[2] = ModelTransform(modelViewProj, v2);

                DrawTriangle(vertices, 0, image, uvs);

                indices += 3;
            }
        }

        vertexBuffer->Unlock();
    }

    URHO3D_LOGERRORF("EditorModelDebug::CheckVertexVisibility: num triangles <%u>", numTriangles_);
}

inline Vector4 EditorModelDebug::ModelTransform(const Matrix4& transform, const Vector3& vertex) const
{
    return Vector4(
        transform.m00_ * vertex.x_ + transform.m01_ * vertex.y_ + transform.m02_ * vertex.z_ + transform.m03_,
        transform.m10_ * vertex.x_ + transform.m11_ * vertex.y_ + transform.m12_ * vertex.z_ + transform.m13_,
        transform.m20_ * vertex.x_ + transform.m21_ * vertex.y_ + transform.m22_ * vertex.z_ + transform.m23_,
        transform.m30_ * vertex.x_ + transform.m31_ * vertex.y_ + transform.m32_ * vertex.z_ + transform.m33_);
}

inline Vector3 EditorModelDebug::ViewportTransform(const Vector4& vertex) const
{
    float invW = 1.0f / vertex.w_;
    return Vector3(invW * vertex.x_ * scaleX_ + offsetX_, invW * vertex.y_ * scaleY_ + offsetY_,
                   invW * vertex.z_ * OCCLUSION_Z_SCALE);
}

void EditorModelDebug::DrawTriangle(Vector4* vertices, unsigned threadIndex, Image* image, Vector2* uvs)
{
    ClipMaskFlags clipMask{};
    ClipMaskFlags andClipMask{};
    bool drawOk = false;
    Vector3 projected[3];

    // Build the clip plane mask for the triangle
    for (unsigned i = 0; i < 3; ++i)
    {
        ClipMaskFlags vertexClipMask{};

        if (vertices[i].x_ > vertices[i].w_)
            vertexClipMask |= CLIPMASK_X_POS;
        if (vertices[i].x_ < -vertices[i].w_)
            vertexClipMask |= CLIPMASK_X_NEG;
        if (vertices[i].y_ > vertices[i].w_)
            vertexClipMask |= CLIPMASK_Y_POS;
        if (vertices[i].y_ < -vertices[i].w_)
            vertexClipMask |= CLIPMASK_Y_NEG;
        if (vertices[i].z_ > vertices[i].w_)
            vertexClipMask |= CLIPMASK_Z_POS;
        if (vertices[i].z_ < 0.0f)
            vertexClipMask |= CLIPMASK_Z_NEG;

        clipMask |= vertexClipMask;

        if (!i)
            andClipMask = vertexClipMask;
        else
            andClipMask &= vertexClipMask;
    }

    // If triangle is fully behind any clip plane, can reject quickly
    if (andClipMask)
        return;

    // Check if triangle is fully inside
    if (!clipMask)
    {
        projected[0] = ViewportTransform(vertices[0]);
        projected[1] = ViewportTransform(vertices[1]);
        projected[2] = ViewportTransform(vertices[2]);

        bool clockwise = SignedArea(projected[0], projected[1], projected[2]) < 0.0f;
        if (cullMode_ == CULL_NONE || (cullMode_ == CULL_CCW && clockwise) || (cullMode_ == CULL_CW && !clockwise))
        {
            DrawTriangle2D(projected, clockwise, threadIndex, image, uvs);
            drawOk = true;
        }
    }
    else
    {
        bool triangles[64];

        // Initial triangle
        triangles[0] = true;
        unsigned numTriangles = 1;

        if (clipMask & CLIPMASK_X_POS)
            ClipVertices(Vector4(-1.0f, 0.0f, 0.0f, 1.0f), vertices, triangles, numTriangles);
        if (clipMask & CLIPMASK_X_NEG)
            ClipVertices(Vector4(1.0f, 0.0f, 0.0f, 1.0f), vertices, triangles, numTriangles);
        if (clipMask & CLIPMASK_Y_POS)
            ClipVertices(Vector4(0.0f, -1.0f, 0.0f, 1.0f), vertices, triangles, numTriangles);
        if (clipMask & CLIPMASK_Y_NEG)
            ClipVertices(Vector4(0.0f, 1.0f, 0.0f, 1.0f), vertices, triangles, numTriangles);
        if (clipMask & CLIPMASK_Z_POS)
            ClipVertices(Vector4(0.0f, 0.0f, -1.0f, 1.0f), vertices, triangles, numTriangles);
        if (clipMask & CLIPMASK_Z_NEG)
            ClipVertices(Vector4(0.0f, 0.0f, 1.0f, 0.0f), vertices, triangles, numTriangles);

        // Draw each accepted triangle
        for (unsigned i = 0; i < numTriangles; ++i)
        {
            if (triangles[i])
            {
                unsigned index = i * 3;
                projected[0] = ViewportTransform(vertices[index]);
                projected[1] = ViewportTransform(vertices[index + 1]);
                projected[2] = ViewportTransform(vertices[index + 2]);

                bool clockwise = SignedArea(projected[0], projected[1], projected[2]) < 0.0f;
                if (cullMode_ == CULL_NONE || (cullMode_ == CULL_CCW && clockwise) ||
                    (cullMode_ == CULL_CW && !clockwise))
                {
                    // DrawTriangle2D(projected, clockwise, threadIndex);
                    DrawTriangle2D(projected, clockwise, threadIndex, image, uvs);
                    drawOk = true;
                }
            }
        }
    }

    if (drawOk)
        ++numTriangles_;
}

inline float EditorModelDebug::SignedArea(const Vector3& v0, const Vector3& v1, const Vector3& v2) const
{
    float aX = v0.x_ - v1.x_;
    float aY = v0.y_ - v1.y_;
    float bX = v2.x_ - v1.x_;
    float bY = v2.y_ - v1.y_;
    return aX * bY - aY * bX;
}

void EditorModelDebug::ClipVertices(const Vector4& plane, Vector4* vertices, bool* triangles, unsigned& numTriangles)
{
    unsigned num = numTriangles;

    for (unsigned i = 0; i < num; ++i)
    {
        if (triangles[i])
        {
            unsigned index = i * 3;
            float d0 = plane.DotProduct(vertices[index]);
            float d1 = plane.DotProduct(vertices[index + 1]);
            float d2 = plane.DotProduct(vertices[index + 2]);

            // If all vertices behind the plane, reject triangle
            if (d0 < 0.0f && d1 < 0.0f && d2 < 0.0f)
            {
                triangles[i] = false;
                continue;
            }
            // If 2 vertices behind the plane, create a new triangle in-place
            else if (d0 < 0.0f && d1 < 0.0f)
            {
                vertices[index] = ClipEdge(vertices[index], vertices[index + 2], d0, d2);
                vertices[index + 1] = ClipEdge(vertices[index + 1], vertices[index + 2], d1, d2);
            }
            else if (d0 < 0.0f && d2 < 0.0f)
            {
                vertices[index] = ClipEdge(vertices[index], vertices[index + 1], d0, d1);
                vertices[index + 2] = ClipEdge(vertices[index + 2], vertices[index + 1], d2, d1);
            }
            else if (d1 < 0.0f && d2 < 0.0f)
            {
                vertices[index + 1] = ClipEdge(vertices[index + 1], vertices[index], d1, d0);
                vertices[index + 2] = ClipEdge(vertices[index + 2], vertices[index], d2, d0);
            }
            // 1 vertex behind the plane: create one new triangle, and modify one in-place
            else if (d0 < 0.0f)
            {
                unsigned newIdx = numTriangles * 3;
                triangles[numTriangles] = true;
                ++numTriangles;

                vertices[newIdx] = ClipEdge(vertices[index], vertices[index + 2], d0, d2);
                vertices[newIdx + 1] = vertices[index] = ClipEdge(vertices[index], vertices[index + 1], d0, d1);
                vertices[newIdx + 2] = vertices[index + 2];
            }
            else if (d1 < 0.0f)
            {
                unsigned newIdx = numTriangles * 3;
                triangles[numTriangles] = true;
                ++numTriangles;

                vertices[newIdx + 1] = ClipEdge(vertices[index + 1], vertices[index], d1, d0);
                vertices[newIdx + 2] = vertices[index + 1] = ClipEdge(vertices[index + 1], vertices[index + 2], d1, d2);
                vertices[newIdx] = vertices[index];
            }
            else if (d2 < 0.0f)
            {
                unsigned newIdx = numTriangles * 3;
                triangles[numTriangles] = true;
                ++numTriangles;

                vertices[newIdx + 2] = ClipEdge(vertices[index + 2], vertices[index + 1], d2, d1);
                vertices[newIdx] = vertices[index + 2] = ClipEdge(vertices[index + 2], vertices[index], d2, d0);
                vertices[newIdx + 1] = vertices[index + 1];
            }
        }
    }
}

inline Vector4 EditorModelDebug::ClipEdge(const Vector4& v0, const Vector4& v1, float d0, float d1) const
{
    float t = d0 / (d0 - d1);
    return v0 + t * (v1 - v0);
}

void EditorModelDebug::DrawTriangle2D(const Vector3* vertices, bool clockwise, unsigned threadIndex, Image* texImage, Vector2* uvs)
{
    Vector3 v0 = vertices[0];
    Vector3 v1 = vertices[1];
    Vector3 v2 = vertices[2];

    vertices_.Push(v0);
    vertices_.Push(v1);
    vertices_.Push(v2);

    uvs_.Push(uvs[0]);
    uvs_.Push(uvs[1]);
    uvs_.Push(uvs[2]);

    float scale = 1.0f / 2.0f;
    // URHO3D_LOGERRORF("EditorModelDebug::DrawTriangle2D: size <%i, %i> v0 <%s> v1 <%s>", width, height, (v0 *
    // scale).ToString().CString(), (v1 * scale).ToString().CString());
    unsigned color = Color::RED.ToUInt();
    DrawLine2(IntVector2(v0.x_ * scale, v0.y_ * scale), IntVector2(v1.x_ * scale, v1.y_ * scale), texImage, color);
    DrawLine2(IntVector2(v1.x_ * scale, v1.y_ * scale), IntVector2(v2.x_ * scale, v2.y_ * scale), texImage, color);
    DrawLine2(IntVector2(v2.x_ * scale, v2.y_ * scale), IntVector2(v0.x_ * scale, v0.y_ * scale), texImage, color);
}

void EditorModelDebug::DrawPoint(int x, int y, Image* image, unsigned color)
{
    // draw point
    for (unsigned j = 0; j < 9; j++)
    {
        int xx = (j % 3) - 1;
        int yy = (j / 3) - 1;
        image->SetPixelInt(x + xx, y + yy, color);
    }
}

} // namespace Urho3D
