#include "MaterialTriangleMeshInterface.h"

#include "../Graphics/Geometry.h"

namespace Urho3D
{

static const unsigned QUANTIZE_MAX_TRIANGLES = 1000000;

MaterialTriangleMeshInterface::MaterialTriangleMeshInterface(Model* model,
        unsigned lodLevel) :
        btTriangleIndexVertexMaterialArray()
{
    unsigned numGeometries = model->GetNumGeometries();
    unsigned totalTriangles = 0;

    URHO3D_LOGERRORF("Materialtrianglemeshinterface.Materialtrianglemeshinterface: model name <%s> numGeometries <%u>", model->GetName().CString(), numGeometries);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        Geometry* geometry = model->GetGeometry(i, lodLevel);
        if (!geometry)
        {
            URHO3D_LOGWARNING("Skipping null geometry for triangle mesh collision");
            continue;
        }

        SharedArrayPtr<unsigned char> vertexData;
        SharedArrayPtr<unsigned char> indexData;
        unsigned vertexSize;
        unsigned indexSize;
        const PODVector<VertexElement>* elements;

        geometry->GetRawDataShared(vertexData, vertexSize, indexData, indexSize, elements);

        positionOffset_ = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_POSITION);
        if (!vertexData || !indexData || !elements || positionOffset_ != 0)
        {
            URHO3D_LOGWARNING("Skipping geometry with no or unsuitable CPU-side geometry data for triangle mesh collision");
            continue;
        }
        normalOffset_ = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_NORMAL);

        // Keep shared pointers to the vertex/index data so that if it's unloaded or changes size, we don't crash
        dataArrays_.Push(vertexData);
        dataArrays_.Push(indexData);

        unsigned indexStart = geometry->GetIndexStart();
        unsigned indexCount = geometry->GetIndexCount();

        unsigned vertexStart = geometry->GetVertexStart();
        unsigned vertexCount = geometry->GetVertexCount();

        // URHO3D_LOGINFOF("Materialtrianglemeshinterface.customtrianglemeshinterface: vertexStart <%u> vertexCount <%u>", vertexStart, vertexCount);
        // URHO3D_LOGINFOF("Materialtrianglemeshinterface.customtrianglemeshinterface: indexStart <%u> indexCount <%u>", indexStart, indexCount);

        btIndexedMesh meshIndex;
        meshIndex.m_numTriangles = indexCount / 3;
        meshIndex.m_triangleIndexBase = &indexData[indexStart * indexSize];
        meshIndex.m_triangleIndexStride = 3 * indexSize;
        meshIndex.m_numVertices = vertexCount;
        meshIndex.m_vertexBase = vertexData;
        meshIndex.m_vertexStride = vertexSize;
        meshIndex.m_indexType = (indexSize == sizeof(unsigned short)) ? PHY_SHORT : PHY_INTEGER;
        meshIndex.m_vertexType = PHY_FLOAT;
        m_indexedMeshes.push_back(meshIndex);

        colorOffset_ = VertexBuffer::GetElementOffset(*elements, TYPE_INT, SEM_OBJECTINDEX);
        if (colorOffset_ != M_MAX_UNSIGNED)
        {
            btMaterialProperties matProp;
            matProp.m_numMaterials = 1; // TYPE_UBYTE4_NORM * 4 = 1 FLOAT
            matProp.m_materialBase = vertexData;
            matProp.m_materialStride = vertexSize;
            matProp.m_materialType = PHY_FLOAT;

            matProp.m_numTriangles = indexCount / 3;
            matProp.m_triangleMaterialsBase = &indexData[indexStart * indexSize];
            matProp.m_triangleMaterialStride = 3 * indexSize;
            addMaterialProperties(matProp);

            totalTriangles += meshIndex.m_numTriangles;
        }
    }

    // Bullet will not work properly with quantized AABB compression, if the triangle count is too large. Use a conservative
    // threshold value
    useQuantize_ = totalTriangles <= QUANTIZE_MAX_TRIANGLES;
}

MaterialTriangleMeshInterface::MaterialTriangleMeshInterface(CustomGeometry* custom) :
        btTriangleIndexVertexMaterialArray()
{
    const Vector<PODVector<CustomGeometryVertex> >& srcVertices = custom->GetVertices();
    unsigned totalVertexCount = 0;
    unsigned totalTriangles = 0;

    for (unsigned i = 0; i < srcVertices.Size(); ++i)
        totalVertexCount += srcVertices[i].Size();

    if (totalVertexCount)
    {
        // CustomGeometry vertex data is unindexed, so build index data here
        SharedArrayPtr<unsigned char> vertexData(new unsigned char[totalVertexCount * sizeof(Vector3)]);
        SharedArrayPtr<unsigned char> indexData(new unsigned char[totalVertexCount * sizeof(unsigned)]);
        dataArrays_.Push(vertexData);
        dataArrays_.Push(indexData);

        Vector3* destVertex = reinterpret_cast<Vector3*>(&vertexData[0]);
        unsigned* destIndex = reinterpret_cast<unsigned*>(&indexData[0]);
        unsigned k = 0;

        for (unsigned i = 0; i < srcVertices.Size(); ++i)
        {
            for (unsigned j = 0; j < srcVertices[i].Size(); ++j)
            {
                *destVertex++ = srcVertices[i][j].position_;
                *destIndex++ = k++;
            }
        }

        btIndexedMesh meshIndex;
        meshIndex.m_numTriangles = totalVertexCount / 3;
        meshIndex.m_triangleIndexBase = indexData;
        meshIndex.m_triangleIndexStride = 3 * sizeof(unsigned);
        meshIndex.m_numVertices = totalVertexCount;
        meshIndex.m_vertexBase = vertexData;
        meshIndex.m_vertexStride = sizeof(Vector3);
        meshIndex.m_indexType = PHY_INTEGER;
        meshIndex.m_vertexType = PHY_FLOAT;
        m_indexedMeshes.push_back(meshIndex);

        totalTriangles += meshIndex.m_numTriangles;
    }

    useQuantize_ = totalTriangles <= QUANTIZE_MAX_TRIANGLES;
}

}
