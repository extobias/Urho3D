#include "EditorModelDebug.h"

#include "../Core/Context.h"
#include "../Graphics/Batch.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Model.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
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

void EditorModelDebug::UpdateBatches(const FrameInfo& frame)
{
    const Matrix3x4* worldTransform = node_ ? &node_->GetWorldTransform() : nullptr;
    transform_ = *worldTransform;
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
       primitiveType_ = TRIANGLE_STRIP;

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

           // indexBuffer_ = new IndexBuffer(context_);
           // indexBuffer_->SetShadowed(true);
           // indexCount > 0xFFFF
           indexBuffer_->SetSize(indexCount, true);

           unsigned* indexDest = (unsigned*)indexBuffer_->Lock(0, indexCount);

           // vertexBuffer_ = new VertexBuffer(context_);
           // vertexBuffer_->SetShadowed(true);
           vertexBuffer_->SetSize(totalVertices * size, vertexElements_, true);
           unsigned vertexSize2 = vertexBuffer_->GetVertexSize();
           (void)vertexSize2;
           auto* dest = (unsigned char*)vertexBuffer_->Lock(0, totalVertices * size, true);

           if (dest && indexDest)
           {
               float scale = 15.0f;
               unsigned color = Color::GREEN.ToUInt();
               unsigned indexOffset = 0;

               Matrix3x4 transform = node_->GetWorldTransform();

               for(unsigned i = 0; i < totalVertices; i++)
               {
                   // Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                   Vector3 position = Vector3::ZERO;
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
                   /// indexDest[indexOffset + indexLength] = 0xFFFF;

                   indexOffset += indexLength;
               }

               worldTransforms_.Resize(totalVerticesReal);
               batches_[0].worldTransform_ = &worldTransforms_[0];
               batches_[0].numWorldTransforms_ = totalVerticesReal;
               for(unsigned i = 0; i < totalVerticesReal; i++)
               {
                   Vector3 position = *((const Vector3*)(&srcData[i * vertexSize + positionOffset]));
                   Matrix3x4 worldTransform (transform);
                   worldTransform.SetTranslation(position);
                   worldTransforms_[i] = worldTransform;
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
