#pragma once

#include "../Graphics/CustomGeometry.h"
#include "../Graphics/Model.h"
#include "../IO/Log.h"
#include "../Graphics/VertexBuffer.h"

// #include <Bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h>
#include <Bullet/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h>

namespace Urho3D
{

class MaterialTriangleMeshInterface : public btTriangleIndexVertexMaterialArray
{
public:

	MaterialTriangleMeshInterface(Model* model, unsigned lodLevel);
	MaterialTriangleMeshInterface(CustomGeometry* custom);

    /// OK to use quantization flag.
    bool useQuantize_;

    unsigned GetNormalOffset() const { return normalOffset_; }
    unsigned GetPositionOffset() const { return positionOffset_; }

private:
    /// Shared vertex/index data used in the collision
    Vector<SharedArrayPtr<unsigned char> > dataArrays_;
    unsigned normalOffset_;
    unsigned positionOffset_;
};

}
