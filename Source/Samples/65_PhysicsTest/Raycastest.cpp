#include "Raycastest.h"

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

#include <Bullet/BulletDynamics/Dynamics/btRigidBody.h>
#include <Bullet/BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <Bullet/BulletCollision/CollisionDispatch/btCollisionWorld.h>

struct AllConvexResultCallback : public btCollisionWorld::ConvexResultCallback
{
    AllConvexResultCallback(const btVector3&	convexFromWorld, const btVector3&	convexToWorld)
        :m_convexFromWorld(convexFromWorld),
        m_convexToWorld(convexToWorld),
        m_hitCollisionObject(0)
    {
    }

    btVector3	m_convexFromWorld;//used to calculate hitPointWorld from hitFraction
    btVector3	m_convexToWorld;

    // btVector3	m_hitNormalWorld;
    // btVector3	m_hitPointWorld;
    const btCollisionObject* m_hitCollisionObject;
    btAlignedObjectArray<const btCollisionObject*> m_collisionObjects;

    btAlignedObjectArray<btVector3>	m_hitNormalWorld;
    btAlignedObjectArray<btVector3>	m_hitPointWorld;
    btAlignedObjectArray<btScalar> m_hitFractions;

    virtual	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        //caller already does the filter on the m_closestHitFraction
        // Assert(convexResult.m_hitFraction <= m_closestHitFraction);

        //m_closestHitFraction = convexResult.m_hitFraction;
        //m_hitCollisionObject = convexResult.m_hitCollisionObject;
        //if (normalInWorldSpace)
        //{
        //	m_hitNormalWorld = convexResult.m_hitNormalLocal;
        //}
        //else
        //{
        //	///need to transform normal into worldspace
        //	m_hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        //}
        //m_hitPointWorld = convexResult.m_hitPointLocal;
        //return convexResult.m_hitFraction;

        // URHO3D_LOGERRORF("addSingleResult <%i>", m_hitPointWorld.size());

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
            hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }
        m_hitNormalWorld.push_back(hitNormalWorld);
        btVector3 hitPointWorld;
        hitPointWorld.setInterpolate3(m_convexFromWorld, m_convexToWorld, convexResult.m_hitFraction);
        m_hitPointWorld.push_back(hitPointWorld);
        m_hitFractions.push_back(convexResult.m_hitFraction);
        // return m_closestHitFraction;
        return convexResult.m_hitFraction;
    }
};

Raycastest::Raycastest(Context* context)
    : LogicComponent(context),
    suspensionRest_(1.0f),
    radius_(0.3f),
    hardPointWS_(Vector3::ZERO),
    direction_(Vector3::DOWN)
{
	URHO3D_LOGERRORF("Raycastest::Raycastest");
    SetUpdateEventMask(USE_FIXEDUPDATE | USE_POSTUPDATE);
}

void Raycastest::RegisterObject(Context* context)
{
	context->RegisterFactory<Raycastest>();
}

void Raycastest::OnNodeSet(Node* node)
{
    if(!node)
        return;

    shapeNode_ = node->GetScene()->CreateChild("wheel");
    Vector3 direction = Vector3::DOWN;
    Vector3 pos(direction * suspensionRest_);
    //wheelNode->SetPosition(node_->LocalToWorld(pos));
    shapeNode_->SetPosition(node_->GetPosition());
    shapeNode_->SetScale(1.0f);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // auto* wheelObject = shapeNode_->CreateComponent<StaticModel>();
    // wheelObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));

    // auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
    shape_ = shapeNode_->CreateComponent<CollisionShape>();

    // Quaternion rot(90.0f, Vector3::FORWARD);
    Quaternion rot = Quaternion::IDENTITY;
    shape_->SetCylinder(radius_ * 4, 1.0f, Vector3::ZERO, rot);
}

void Raycastest::FixedUpdate(float timeStep)
{
}

void Raycastest::PostUpdate(float timeStep)
{
    DebugRenderer* debug = GetScene()->GetComponent<DebugRenderer>();
    shape_->DrawDebugGeometry(debug, false);

    RayCast();

    Ray ray(hardPointWS_, direction_);
    Vector3 startPos = ray.origin_;
    Vector3 endPos = ray.origin_ + suspensionRest_ * ray.direction_;

    Sphere startSphere(startPos, 0.1f);
    debug->AddSphere(startSphere, Color::RED, false);

    Sphere endSphere(endPos, 0.1f);
    debug->AddSphere(endSphere, Color::GREEN, false);
}

void Raycastest::Update()
{
    hardPointWS_ = node_->GetPosition();
    shapeNode_->SetPosition(hardPointWS_);

    // URHO3D_LOGERRORF("raycastest: pos <%f, %f, %f>", pos.x_, pos.y_, pos.z_);
}

void Raycastest::RayCast()
{
    Update();

    unsigned mask = 1 << 0;
    PhysicsWorld* pw = GetScene()->GetComponent<PhysicsWorld>();
    btCollisionWorld* world = pw->GetWorld();

    Quaternion worldRotation = shape_->GetRotation();

    Ray ray(hardPointWS_, direction_);
    Vector3 startPos = ray.origin_;
    Vector3 endPos = ray.origin_ + suspensionRest_ * ray.direction_;
    Quaternion startRot = worldRotation;
    Quaternion endRot = worldRotation;

    AllConvexResultCallback convexCallback(ToBtVector3(startPos), ToBtVector3(endPos));
    convexCallback.m_collisionFilterGroup = (short)0xffff;
    convexCallback.m_collisionFilterMask = (short)0xffff;

    // btCollisionShape* shape = shape_->GetCollisionShape();
    world->convexSweepTest(reinterpret_cast<btConvexShape*>(shape_->GetCollisionShape()),
        btTransform(ToBtQuaternion(startRot), convexCallback.m_convexFromWorld),
        btTransform(ToBtQuaternion(endRot), convexCallback.m_convexToWorld),
        convexCallback);

    if (convexCallback.hasHit())
    {
        URHO3D_LOGERRORF("hit point <%i>", convexCallback.m_hitPointWorld.size());
    }
}
