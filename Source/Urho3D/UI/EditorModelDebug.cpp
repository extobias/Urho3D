#include "EditorModelDebug.h"

#include "../Core/Context.h"
#include "../Graphics/Batch.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/OctreeQuery.h"
#include "../Graphics/VertexBuffer.h"

#include "../IO/Log.h"
#include "../Resource/ResourceEvents.h"

namespace Urho3D
{

extern const char* SUBSYSTEM_CATEGORY;

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

EditorModelDebug::EditorModelDebug(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY),
    geometry_(new Geometry(context)),
    vertexBuffer_(new VertexBuffer(context_)),
    indexBuffer_(new IndexBuffer(context_))
{
    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);

    batches_.Resize(1);
    batches_[0].geometry_ = geometry_;
    batches_[0].geometryType_ = GEOM_INSTANCED;
}

EditorModelDebug::~EditorModelDebug() = default;

void EditorModelDebug::RegisterObject(Context* context)
{
    context->RegisterFactory<EditorModelDebug>(SUBSYSTEM_CATEGORY);
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
        float distance = query.ray_.HitDistance(boundingBox_.Transformed(worldTransforms_[i]));
        Vector3 normal = -query.ray_.direction_;
        unsigned subObjectElementIndex = M_MAX_UNSIGNED;

        // Then proceed to OBB and triangle-level tests if necessary
        if (level >= RAY_OBB && distance < query.maxDistance_)
        {
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
                        float geometryDistance = geometry->GetHitDistance(localRay, subObjectElementIndex, &geometryNormal);
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

            URHO3D_LOGERRORF("hit node <%s> subObject <%i> subObjectElementIndex <%i>",
                             node_->GetName().CString(), i, subObjectElementIndex);
            results.Push(result);
        }
    }
}

void EditorModelDebug::UpdateBatches(const FrameInfo& frame)
{
    const Matrix3x4* worldTransform = node_ ? &node_->GetWorldTransform() : nullptr;
    for(unsigned i = 0; i < numWorldTransforms_; i++)
    {
        Matrix3x4 transform(Matrix3x4::IDENTITY);
        transform.SetTranslation(*worldTransform * vertexOffset_[i]);
        worldTransforms_[i] = transform;
    }
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

    if (model_)
        UnsubscribeFromEvent(model_, E_RELOADFINISHED);

    model_ = model;

    if (model)
    {
       SubscribeToEvent(model, E_RELOADFINISHED, URHO3D_HANDLER(EditorModelDebug, HandleModelReloadFinished));

       SetBoundingBox(model->GetBoundingBox());

       /// set up
       primitiveType_ = TRIANGLE_LIST;

       const Matrix3x4* worldTransform = node_ ? &node_->GetWorldTransform() : nullptr;
       // transform_ = *worldTransform;
       transform_ = Matrix3x4::IDENTITY;

       if(model->GetVertexBuffers().Size())
       {
           VertexBuffer* vb = model->GetVertexBuffers().At(0);

           unsigned vertexSize = vb->GetVertexSize();
           unsigned totalVerticesReal = vb->GetVertexCount();
           unsigned totalVertices = 1;

           const SharedArrayPtr<unsigned char>& vertexData = vb->GetShadowDataShared();
           const auto* srcData = (const unsigned char*)&vertexData[0];

           unsigned positionOffset = VertexBuffer::GetElementOffset(vb->GetElements(), TYPE_VECTOR3, SEM_POSITION);

           vertexElements_.Clear();
           vertexElements_.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION, 0, false));
           vertexElements_.Push(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR, 0, false));
           VertexBuffer::UpdateOffsets(vertexElements_);

           unsigned size = 8;

           unsigned indexLength = sizeof(cubeIndex) / sizeof(cubeIndex[0]);
           unsigned indexCount = totalVertices * indexLength;

           indexBuffer_->SetShadowed(true);
           // indexCount > 0xFFFF
           indexBuffer_->SetSize(indexCount, true);

           unsigned* indexDest = (unsigned*)indexBuffer_->Lock(0, indexCount);

           vertexBuffer_->SetShadowed(true);
           vertexBuffer_->SetSize(totalVertices * size, vertexElements_, true);
           unsigned vertexSize2 = vertexBuffer_->GetVertexSize();
           (void)vertexSize2;
           auto* dest = (unsigned char*)vertexBuffer_->Lock(0, totalVertices * size, true);

           if (dest && indexDest)
           {
               float scale = 0.02f;
               unsigned color = Color::GREEN.ToUInt();
               unsigned indexOffset = 0;

               Matrix3x4 transform = node_->GetWorldTransform();

               for(unsigned i = 0; i < totalVertices; i++)
               {
                   // Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                   Vector3 position = Vector3::ZERO;
                   // position.y_ = 3.0f;
                   for(unsigned j = 0; j < size; j++)
                   {
                       *((Vector3*)dest) = position + Vector3(cubeVertices + j * 3) * scale;
                       dest += sizeof(Vector3);

                       *((unsigned*)dest) = color;
                       dest += sizeof(unsigned);
                   }

                   for (unsigned j = 0; j < indexLength; j ++ )
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
               for(unsigned i = 0; i < totalVerticesReal; i++)
               {
                   Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                   vertexOffset_.Push(position);
               }

               geometry_->SetVertexBuffer(0, vertexBuffer_);
               geometry_->SetIndexBuffer(indexBuffer_);
               geometry_->SetDrawRange(primitiveType_, 0, indexOffset, 0, totalVertices * size);

               vertexBuffer_->Unlock();
               indexBuffer_->Unlock();
           }
       }
    }
}

void EditorModelDebug::SetMaterial(Material* material)
{
    batches_[0].material_ = material;
}

void EditorModelDebug::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
    OnMarkedDirty(node_);
}

void EditorModelDebug::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void EditorModelDebug::HandleModelReloadFinished(StringHash eventType, VariantMap& eventData)
{
    Model* currentModel = model_;
    model_.Reset(); // Set null to allow to be re-set
    SetModel(currentModel);
}

}
