#ifndef CONVEXCAST_H
#define CONVEXCAST_H


#include <Urho3D/Scene/Component.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Physics/CollisionShape.h>

using namespace Urho3D;

struct WheelInfo
{
    struct RaycastInfo
    {
        // WS : world coords
        Vector3 hardPointWS_;
        Vector3 wheelDirectionWS_;
        float suspensionLength_;
        float suspensionMinRest_;
        float suspensionMaxRest_;
        float suspensionRelativeVelocity_;

        // output
        Vector3 contactPoint_;
        Vector3 contactPointLocal_;
        Vector3 contactNormal_;
        Vector3 contactNormalOld_;
        float compressionRatio_;
        float distance_;

        bool isInContact_;
        int contactNodeID_;
        unsigned contactMaterial_;
    };
    // CS : chassis coords
    RaycastInfo raycastInfo_;
    Vector3 chassisConnectionPointCS_;
    Vector3 wheelDirectionCS_;
    Vector3 wheelAxle_;
    SharedPtr<Node> node_;
    float steering_;
    unsigned frameNumber_;
};

class ConvexCast : public Component
{
    URHO3D_OBJECT(ConvexCast, Component);
public:

    explicit ConvexCast(Context* context);

    ~ConvexCast();

    static void RegisterObject(Context* context);

    void OnNodeSet(Node* node) override;

    float Update(WheelInfo& info, RigidBody *hullBody, float steeringTimer = 0.0, bool debug = false, bool interpolateNormal = false);

    void DebugDraw(const WheelInfo &wheel, Vector3 centerOfMass, bool first = false, float speed = 0.0f, Vector3 angularVel = Vector3::ZERO);

    void SetOffset(const Vector3& offset) { offset_ = offset; }

    Vector3 GetOffset() const { return offset_; }

    void SetRadius(float r);

    float GetRadius() const { return radius_; }

    unsigned hitPointsSize_;
    Vector<float> hitDistance_;
    Vector<Vector3> hitPointWorld_;
    Vector<Vector3> hitPointLocal_;
    Vector<Vector3> hitNormalWorld_;
    Vector<float> hitFraction_;
    Vector<int> hitShapePart_;
    Vector<int> hitTriangleIndex_;

    int hitIndex_;
    bool hasHit_;

    Vector3 offset_;

    RigidBody* hitBody_;

    SharedPtr<CollisionShape> shape_;

private:

    void UpdateTransform(WheelInfo& wheel, float steeringTimer);

    Vector3 hitPoint_;
    Vector3 hitNormal_;
    float radius_;
    Quaternion rotation_;

};

#endif // CONVEXCAST_H
