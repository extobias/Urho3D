#include "ConvexCast.h"

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsUtils.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Editor/EditorModelDebug.h>

#include <Bullet/BulletDynamics/Dynamics/btRigidBody.h>
#include <Bullet/BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <Bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h>

static const btVector3 WHITE(1.0f, 1.0f, 1.0f);
static const btVector3 GREEN(0.0f, 1.0f, 0.0f);

struct AllConvexResultCallback : public btCollisionWorld::ConvexResultCallback
{
    AllConvexResultCallback(const btVector3& convexFromWorld, const btVector3& convexToWorld)
        :m_convexFromWorld(convexFromWorld),
        m_convexToWorld(convexToWorld),
        m_hitCollisionObject(0)
    {
        // URHO3D_LOGERRORF("AllConvexResultCallback ctor <%i>", m_hitPointWorld.size());
    }

    btVector3 m_convexFromWorld;//used to calculate hitPointWorld from hitFraction
    btVector3 m_convexToWorld;

    // btVector3    m_hitNormalWorld;
    // btVector3    m_hitPointWorld;
    const btCollisionObject* m_hitCollisionObject;
    btAlignedObjectArray<const btCollisionObject*> m_collisionObjects;

    btAlignedObjectArray<btVector3> m_hitNormalWorld;
    btAlignedObjectArray<btVector3> m_hitPointWorld;
    btAlignedObjectArray<btVector3> m_hitPointLocal;
    btAlignedObjectArray<btScalar> m_hitFractions;
    btAlignedObjectArray<int> m_hitShapePart;
    btAlignedObjectArray<int> m_hitTriangleIndex;

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        //caller already does the filter on the m_closestHitFraction
        // Assert(convexResult.m_hitFraction <= m_closestHitFraction);

        //m_closestHitFraction = convexResult.m_hitFraction;
        //m_hitCollisionObject = convexResult.m_hitCollisionObject;
        //if (normalInWorldSpace)
        //{
        //    m_hitNormalWorld = convexResult.m_hitNormalLocal;
        //}
        //else
        //{
        //    ///need to transform normal into worldspace
        //    m_hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        //}
        //m_hitPointWorld = convexResult.m_hitPointLocal;
        //return convexResult.m_hitFraction;
        // return m_closestHitFraction;
        // URHO3D_LOGERRORF("addSingleResult fraction <%f>", convexResult.m_hitFraction);

        m_closestHitFraction = convexResult.m_hitFraction;
        m_hitCollisionObject = convexResult.m_hitCollisionObject;
        m_collisionObjects.push_back(convexResult.m_hitCollisionObject);

        btVector3 hitNormalWorld;
        if (normalInWorldSpace)
        {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        }
        else
        {
            ///need to transform normal into worldspace
            hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }
        m_hitNormalWorld.push_back(hitNormalWorld);
        btVector3 hitPointWorld;
        hitPointWorld.setInterpolate3(m_convexFromWorld, m_convexToWorld, convexResult.m_hitFraction);
        m_hitPointWorld.push_back(hitPointWorld);
        m_hitFractions.push_back(convexResult.m_hitFraction);
        m_hitPointLocal.push_back(convexResult.m_hitPointLocal);

        if (convexResult.m_localShapeInfo)
        {
            m_hitShapePart.push_back(convexResult.m_localShapeInfo->m_shapePart);
            m_hitTriangleIndex.push_back(convexResult.m_localShapeInfo->m_triangleIndex);
        }
        else
        {
            m_hitShapePart.push_back(-1);
            m_hitTriangleIndex.push_back(-1);
        }

        return convexResult.m_hitFraction;
    }
};

ConvexCast::ConvexCast(Context* context)
    : Component(context),
    hasHit_(false),
    radius_(0.40f),
    hitPointsSize_(0),
    hitBody_(nullptr)
{
}


ConvexCast::~ConvexCast()
{
}

void ConvexCast::RegisterObject(Context* context)
{
    context->RegisterFactory<ConvexCast>();
}

void ConvexCast::OnNodeSet(Node* node)
{
    if (!node)
        return;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // auto* wheelObject = shapeNode_->CreateComponent<StaticModel>();
    // wheelObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
    // auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
    shape_ = node->CreateComponent<CollisionShape>();

//    shape_->SetCylinder(radius_ * 2, radius_, Vector3::ZERO);
    shape_->SetSphere(radius_ * 2);
}

void ConvexCast::SetRadius(float r)
{
    radius_ = r;
    shape_->SetCylinder(radius_ * 2, radius_, Vector3::ZERO);
}

void ConvexCast::UpdateTransform(WheelInfo& wheel, float steeringTimer)
{
    // cylinder rotation
    Quaternion rot(90.0f, Vector3::FORWARD);
    // steering rotation
    rot = rot * Quaternion(45.0f * Urho3D::Sign(wheel.steering_) * 1.0f, Vector3::RIGHT);

    shape_->SetRotation(rot);

    // va relativo al centro del nodo, sino habria que hacer la transformacion con hardPointCS_
    // respecto a la posicion/rotacion del nodo
//    Quaternion rot = node_->GetRotation();
//    hardPointWS_ = node_->GetPosition() + rot * offset_;
}

float ConvexCast::Update(WheelInfo& wheel, RigidBody* hullBody, float steeringTimer, bool debug, bool interpolateNormal)
{
    URHO3D_PROFILE(ConvexCastUpdate);

    UpdateTransform(wheel, steeringTimer);

    Scene* scene = GetScene();
    if (!scene)
        return 0.0f;

    PhysicsWorld* pw = scene->GetComponent<PhysicsWorld>();
    btCollisionWorld* world = pw->GetWorld();

    Quaternion worldRotation = node_->GetRotation() * shape_->GetRotation();
    Vector3 direction = wheel.raycastInfo_.wheelDirectionWS_.Normalized();

    // Ray ray(wheel.raycastInfo_.hardPointWS_, wheel.wheelDirectionCS_);
    Ray ray(wheel.raycastInfo_.hardPointWS_, direction);
    Vector3 startPos = ray.origin_;
    Vector3 endPos = ray.origin_ + wheel.raycastInfo_.suspensionLength_ * 1.0f * ray.direction_;
    //Vector3 startPos = Vector3(0.0f, 1.0f, 0.0f);
    //Vector3 endPos = Vector3(0.0f, -1.0f, 0.0f);
    Quaternion startRot = worldRotation;
    Quaternion endRot = worldRotation;

    AllConvexResultCallback convexCallback(ToBtVector3(startPos), ToBtVector3(endPos));
    convexCallback.m_collisionFilterGroup = (short)0xffff;
    convexCallback.m_collisionFilterMask = (short)1 << 0;

    btCollisionShape* shape = shape_->GetCollisionShape();

    world->convexSweepTest(reinterpret_cast<btConvexShape*>(shape),
        btTransform(ToBtQuaternion(startRot), convexCallback.m_convexFromWorld),
        btTransform(ToBtQuaternion(endRot), convexCallback.m_convexToWorld),
        convexCallback, world->getDispatchInfo().m_allowedCcdPenetration);

    hasHit_ = false;
    hitPointsSize_ = 0;
    hitIndex_ = -1;
    hitDistance_.Clear();
    hitFraction_.Clear();
    hitPointWorld_.Clear();
    hitNormalWorld_.Clear();
    hitPointLocal_.Clear();
    hitShapePart_.Clear();
    hitTriangleIndex_.Clear();

    float distance = 1000.0f;
    hitPointsSize_ = convexCallback.m_hitPointWorld.size();
    for (int i = 0; i < hitPointsSize_; i++)
    {
        hitPoint_ = ToVector3(convexCallback.m_hitPointWorld.at(i));
        hitPointWorld_.Push(hitPoint_);
        hitPointLocal_.Push(ToVector3(convexCallback.m_hitPointLocal.at(i)));
        hitNormal_ = ToVector3(convexCallback.m_hitNormalWorld.at(i));
        hitNormalWorld_.Push(hitNormal_);
        hitFraction_.Push((float)convexCallback.m_hitFractions.at(i));
        if (i < convexCallback.m_hitShapePart.size())
        {
            hitShapePart_.Push(convexCallback.m_hitShapePart.at(i));
            hitTriangleIndex_.Push(convexCallback.m_hitTriangleIndex.at(i));
        }

        // use most closest to startPos point as index
        float d = (hitPoint_ - startPos).Length();
        hitDistance_.Push(d);
        if (distance > d)
        {
            distance = d;
            hitIndex_ = i;
        }
    }

    float angNormal = 0.0f;
    if (convexCallback.hasHit() && hitIndex_ != -1)
    {
        hasHit_ = true;
        hitBody_ = static_cast<RigidBody*>(convexCallback.m_collisionObjects.at(hitIndex_)->getUserPointer());

        wheel.raycastInfo_.isInContact_ = true;
        wheel.raycastInfo_.distance_ = Max(hitDistance_.At(hitIndex_), wheel.raycastInfo_.suspensionMinRest_);

        if (hitDistance_.At(hitIndex_) < wheel.raycastInfo_.suspensionMinRest_)
        {
            // wheel.raycastInfo_.contactPoint_ = wheel.raycastInfo_.hardPointWS_ + wheel.wheelDirectionCS_ * wheel.raycastInfo_.suspensionMinRest_;
            wheel.raycastInfo_.contactPoint_ = wheel.raycastInfo_.hardPointWS_ + direction * wheel.raycastInfo_.suspensionMinRest_;
        }
//            else if (hitDistance_.At(hitIndex_) > wheel.raycastInfo_.suspensionMaxRest_)
//            {
//                wheel.raycastInfo_.contactPoint_ = wheel.raycastInfo_.hardPointWS_ + wheel.wheelDirectionCS_ * wheel.raycastInfo_.suspensionMaxRest_;
//            }
        else
        {
            wheel.raycastInfo_.contactPoint_ = hitPointWorld_.At(hitIndex_);
        }

        wheel.raycastInfo_.contactPointLocal_ = hitPointLocal_.At(hitIndex_);
        //
        angNormal = hitNormalWorld_.At(hitIndex_).DotProduct(wheel.raycastInfo_.contactNormal_);
//        if((acos(angNormal) * M_RADTODEG) > 30.0f)
//        {
//            wheel.raycastInfo_.contactNormal_ = -wheel.wheelDirectionCS_;
//        }
//        else
        {
            if (interpolateNormal)
            {
                const btRigidBody* hitBody = btRigidBody::upcast(convexCallback.m_collisionObjects.at(hitIndex_));
                btCollisionShape* hitShape = (btCollisionShape*)hitBody->getCollisionShape();
                if (hitShape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
                {
                    btVector3 in = pw->InterpolateMeshNormal(hitBody->getWorldTransform(), hitShape, hitShapePart_.At(hitIndex_), hitTriangleIndex_.At(hitIndex_),
                                                  ToBtVector3(hitPointWorld_.At(hitIndex_)), GetComponent<DebugRenderer>());

                    wheel.raycastInfo_.contactNormal_ = ToVector3(in);
                }
                else
                {
//                    result.normal_ = ToVector3(rayCallback.m_hitNormalWorld);
                    wheel.raycastInfo_.contactNormal_ = hitNormalWorld_.At(hitIndex_);
                }
            }
            else
            {
                wheel.raycastInfo_.contactNormal_ = hitNormalWorld_.At(hitIndex_);
            }
        }

//        Node* node = hitBody_->GetNode();
//        btVector3 scale = ToBtVector3(node->GetScale());
//        IntVector3 vc;
//        if (hitIndex_ < hitShapePart_.Size())
//        {
//            bool collisionOk = PhysicsWorld::GetCollisionMask(convexCallback.m_collisionObjects[hitIndex_],
//                                                    ToBtVector3(hitPointWorld_.At(hitIndex_)), hitShapePart_.At(hitIndex_),
//                                                    hitTriangleIndex_.At(hitIndex_), scale, vc);
//            if (collisionOk)
//            {
//                const VertexCollisionMaskFlags& c0 = (const VertexCollisionMaskFlags)(vc.x_);
//                const VertexCollisionMaskFlags& c1 = (const VertexCollisionMaskFlags)(vc.y_);
//                const VertexCollisionMaskFlags& c2 = (const VertexCollisionMaskFlags)(vc.z_);

//                // color = GetFaceColor(c0);
//                if((c0 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY))
//                   && (c1 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY))
//                   && (c2 & (TRACK_ROAD | TRACK_BORDER | OFFTRACK_WEAK | OFFTRACK_HEAVY))
//                   && c0 == c1 && c0 == c2 && c1 == c2)
//                {
//                    wheel.raycastInfo_.contactMaterial_ = c0.AsInteger();
//                }
//                else
//                {
//                    wheel.raycastInfo_.contactMaterial_ = TRACK_ROAD;
//                }
//            }
//            else
//            {
//                wheel.raycastInfo_.contactMaterial_ = TRACK_ROAD;
//            }
//        }
//        else
//        {
//            wheel.raycastInfo_.contactMaterial_ = TRACK_ROAD;
//        }

        float project = wheel.raycastInfo_.contactNormal_.DotProduct(wheel.raycastInfo_.wheelDirectionWS_);
        angNormal = project;
        Vector3 relPos = wheel.raycastInfo_.contactPoint_ - hullBody->GetPosition();

        Vector3 contactVel = hullBody->GetVelocityAtPoint(relPos);
        float projVel = wheel.raycastInfo_.contactNormal_.DotProduct(contactVel);

        if (project >= -0.1f)
        {
            wheel.raycastInfo_.suspensionRelativeVelocity_ = 0.0f;
        }
        else
        {
            float inv = btScalar(-1.) / project;
            wheel.raycastInfo_.suspensionRelativeVelocity_ = projVel * inv;
        }
    }
    else
    {
        hitBody_ = nullptr;
        wheel.raycastInfo_.isInContact_ = false;
        wheel.raycastInfo_.distance_ = 0.0f;
        wheel.raycastInfo_.contactNormal_ = -wheel.raycastInfo_.wheelDirectionWS_;
        wheel.raycastInfo_.contactPoint_ = wheel.raycastInfo_.hardPointWS_ + wheel.raycastInfo_.wheelDirectionWS_;
//        wheel.raycastInfo_.suspensionRelativeVelocity_ = 0.0f;
    }

    return wheel.raycastInfo_.suspensionRelativeVelocity_;
}

void ConvexCast::DebugDraw(const WheelInfo& wheel, Vector3 centerOfMass, bool first, float speed, Vector3 angularVel)
{
    DebugRenderer* debug = GetScene()->GetComponent<DebugRenderer>();

    PhysicsWorld* pw = GetScene()->GetComponent<PhysicsWorld>();
    btCollisionWorld* world = pw->GetWorld();

    Ray ray(wheel.raycastInfo_.hardPointWS_, wheel.raycastInfo_.wheelDirectionWS_);
    Vector3 startPos = ray.origin_;
    Vector3 endPos = ray.origin_ + wheel.raycastInfo_.suspensionLength_ * 1.0f * ray.direction_;

    Sphere startSphere(startPos, 0.1f);
    debug->AddSphere(startSphere, Color::GREEN, false);

    Sphere endSphere(endPos, 0.15f);
    debug->AddSphere(endSphere, Color::RED, false);

    Vector<Color> colors;
    colors.Push(Color::WHITE);
    colors.Push(Color::GRAY);
    colors.Push(Color::BLACK);
    colors.Push(Color::RED);
    colors.Push(Color::GREEN);
    colors.Push(Color::BLUE);
    colors.Push(Color::CYAN);
    colors.Push(Color::MAGENTA);
    colors.Push(Color::YELLOW);

    // Vector3 local(hitPointLocal_.At(i));
    Vector3 local(wheel.raycastInfo_.contactPointLocal_);
    // Color color = colors.At(i % 9);
    Sphere sphere(local, 0.01f);
    debug->AddSphere(sphere, Color::YELLOW, false);

//    Vector3 hit(wheel.raycastInfo_.contactPoint_ - centerOfMass);
    Vector3 hit(wheel.raycastInfo_.contactPoint_);
    Sphere sphereHit(hit, 0.005f);
    debug->AddSphere(sphereHit, wheel.raycastInfo_.isInContact_ ? Color::GREEN : Color::BLUE, false);

    // normal
    Vector3 normal(wheel.raycastInfo_.contactNormal_);
    debug->AddLine(hit, hit + normal.Normalized(), Color::YELLOW, false);

    // debug->AddCylinder(hit, radius_, radius_, Color::BLUE, false);
    pw->SetDebugRenderer(debug);
    pw->SetDebugDepthTest(true);

    Matrix3x4 worldTransform = node_->GetTransform();
    Quaternion rotation = shape_->GetRotation();

    Quaternion torqueRot(speed, Vector3::UP);
    Quaternion worldRotation(worldTransform.Rotation() * rotation * torqueRot);
    Vector3 worldPosition(hit);

    world->debugDrawObject(btTransform(ToBtQuaternion(worldRotation), ToBtVector3(worldPosition)),
    shape_->GetCollisionShape(), btVector3(0.0f, first ? 1.0 : 0.0f, !first ? 1.0f : 0.0f));

    pw->SetDebugRenderer(nullptr);

    // cylinder
//    Vector3 shapePosition(hitPoint_);
//    Quaternion shapeRotation(worldTransform.Rotation() * shape_->GetRotation());

//    bool bodyActive = false;

//    pw->SetDebugRenderer(debug);
//    pw->SetDebugDepthTest(false);

    // world->debugDrawObject(btTransform(ToBtQuaternion(shapeRotation), ToBtVector3(shapePosition)), shape_->GetCollisionShape(), bodyActive ? WHITE : GREEN);

//    pw->SetDebugRenderer(nullptr);
}

