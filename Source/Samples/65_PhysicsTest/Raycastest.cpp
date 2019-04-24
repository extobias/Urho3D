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

    btVector3	m_convexFromWorld;//used to calculate hitPointWorld from hitFraction
    btVector3	m_convexToWorld;

    // btVector3	m_hitNormalWorld;
    // btVector3	m_hitPointWorld;
    const btCollisionObject* m_hitCollisionObject;
    btAlignedObjectArray<const btCollisionObject*> m_collisionObjects;

    btAlignedObjectArray<btVector3>	m_hitNormalWorld;
    btAlignedObjectArray<btVector3>	m_hitPointWorld;
	btAlignedObjectArray<btVector3>	m_hitPointLocal;
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
            hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }
        m_hitNormalWorld.push_back(hitNormalWorld);
        btVector3 hitPointWorld;
		//URHO3D_LOGERRORF("from <%f, %f, %f> to <%f, %f, %f> local <%f, %f, %f> fraction <%f>", 
		//	m_convexFromWorld.getX(), m_convexFromWorld.getY(), m_convexFromWorld.getZ(),
		//	m_convexToWorld.getX(), m_convexToWorld.getY(), m_convexToWorld.getZ(), 
		//	convexResult.m_hitPointLocal.getX(), convexResult.m_hitPointLocal.getY(), convexResult.m_hitPointLocal.getZ(),
		//	convexResult.m_hitFraction);

        hitPointWorld.setInterpolate3(m_convexFromWorld, m_convexToWorld, convexResult.m_hitFraction);
        m_hitPointWorld.push_back(hitPointWorld);
        m_hitFractions.push_back(convexResult.m_hitFraction);
		m_hitPointLocal.push_back(convexResult.m_hitPointLocal);
        // return m_closestHitFraction;
        return convexResult.m_hitFraction;
    }
};

Raycastest::Raycastest(Context* context)
    : LogicComponent(context),
    suspensionRest_(2.0f),
    radius_(0.15f),
    hardPointWS_(Vector3::ZERO),
    direction_(Vector3::DOWN),
	hitPoints_(0),
	hasHit_(false),
	hitBody_(nullptr)
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
    // shapeNode_->SetScale(1.0f);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // auto* wheelObject = shapeNode_->CreateComponent<StaticModel>();
    // wheelObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));

    // auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
    shape_ = shapeNode_->CreateComponent<CollisionShape>();

    Quaternion rot(90.0f, Vector3::FORWARD);
    // Quaternion rot = Quaternion::IDENTITY;
    shape_->SetCylinder(radius_ * 2, radius_ * 4, Vector3::ZERO, rot);
	//shape_->SetSphere(radius_ * 2);
}

void Raycastest::FixedUpdate(float timeStep)
{
	RayCast();
}

void Raycastest::PostUpdate(float timeStep)
{
	DebugDraw();
}

void Raycastest::DebugDraw()
{
	DebugRenderer* debug = GetScene()->GetComponent<DebugRenderer>();

	PhysicsWorld* pw = GetScene()->GetComponent<PhysicsWorld>();
	btCollisionWorld* world = pw->GetWorld();

	// shape_->DrawDebugGeometry(debug, false);
	
	Ray ray(hardPointWS_, direction_);
	Vector3 startPos = ray.origin_;
	Vector3 endPos = ray.origin_ + suspensionRest_ * ray.direction_;
	//Vector3 startPos = Vector3(0.0f, 1.0f, 0.0f);
	//Vector3 endPos = Vector3(0.0f, -1.0f, 0.0f);

	Sphere startSphere(startPos, 0.1f);
	debug->AddSphere(startSphere, Color::RED, false);

	Sphere endSphere(endPos, 0.1f);
	debug->AddSphere(endSphere, Color::GREEN, false);

	// contact point
	// Sphere sphere(hitPoint_ + (suspensionRest_ / 2.0f) * direction_, 0.5f);
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
	if (hasHit_)
	{
		// for (int i = 0; i < hitPointWorld_.Size(); i++)
		int i = hitIndex_;
		{
			Vector3 local(hitPointLocal_.At(i));
			Color color = colors.At(i % 9);
			Sphere sphere(local, 0.01f);
			debug->AddSphere(sphere, color, false);

			// cylinder, el hitpointworld es relativo al comienzo del rayo (?)
			// Vector3 hit(hardPointWS_ - hitPointWorld_.At(i));
			Vector3 hit(hitPointWorld_.At(i));
			Sphere sphereHit(hit, 0.01f);
			debug->AddSphere(sphereHit, Color::BLUE, false);


			// debug->AddCylinder(hit, radius_, radius_, Color::BLUE, false);
			pw->SetDebugRenderer(debug);
			pw->SetDebugDepthTest(true);

			Matrix3x4 worldTransform = node_->GetTransform();
			Quaternion rotation = shape_->GetRotation();

			Vector3 worldPosition(hit);
			Quaternion worldRotation(worldTransform.Rotation() * rotation);
			world->debugDrawObject(btTransform(ToBtQuaternion(worldRotation), ToBtVector3(worldPosition)), shape_->GetCollisionShape(), btVector3(0.0f, 0.0f, 1.0f));

			pw->SetDebugRenderer(nullptr);

			// normal
			Vector3 normal(hitNormalWorld_.At(i));
			debug->AddLine(local, local + normal.Normalized(), Color::YELLOW, false);
		}
		
		// cylinder
		Matrix3x4 worldTransform = node_->GetTransform();
		Vector3 position = hitPoint_;
		Quaternion rotation = shape_->GetRotation();

		Vector3 worldPosition(position);
		Quaternion worldRotation(worldTransform.Rotation() * rotation);

		bool bodyActive = false;

		pw->SetDebugRenderer(debug);
		pw->SetDebugDepthTest(false);

		// world->debugDrawObject(btTransform(ToBtQuaternion(worldRotation), ToBtVector3(worldPosition)), shape_->GetCollisionShape(), bodyActive ? WHITE : GREEN);

		pw->SetDebugRenderer(nullptr);
	}
}

void Raycastest::Update()
{
	// va relativo al centro del nodo
	// sino habria que hacer la transformacion con hardPointCS_
	// respecto a la posicion/rotacion del nodo
	Quaternion rot = node_->GetRotation();
    hardPointWS_ = node_->GetPosition() + rot * offset_;
}

void Raycastest::RayCast()
{
    Update();

    unsigned mask = 1 << 0;
    PhysicsWorld* pw = GetScene()->GetComponent<PhysicsWorld>();
    btCollisionWorld* world = pw->GetWorld();

    Quaternion worldRotation = node_->GetRotation() * shape_->GetRotation();

    Ray ray(hardPointWS_, direction_);
    Vector3 startPos = ray.origin_;
    Vector3 endPos = ray.origin_ + suspensionRest_ * ray.direction_;
	//Vector3 startPos = Vector3(0.0f, 1.0f, 0.0f);
	//Vector3 endPos = Vector3(0.0f, -1.0f, 0.0f);
    Quaternion startRot = worldRotation;
    Quaternion endRot = worldRotation;

    AllConvexResultCallback convexCallback(ToBtVector3(startPos), ToBtVector3(endPos));
    convexCallback.m_collisionFilterGroup = (short)0xffff;
    convexCallback.m_collisionFilterMask = (short)mask;

    btCollisionShape* shape = shape_->GetCollisionShape();
    world->convexSweepTest(reinterpret_cast<btConvexShape*>(shape),
        btTransform(ToBtQuaternion(startRot), convexCallback.m_convexFromWorld),
        btTransform(ToBtQuaternion(endRot), convexCallback.m_convexToWorld),
        convexCallback, world->getDispatchInfo().m_allowedCcdPenetration);

	hasHit_ = false;
	hitPoints_ = 0;
	hitIndex_ = -1;
	hitDistance_.Clear();
	hitFraction_.Clear();
	hitPointWorld_.Clear();
	hitNormalWorld_.Clear();
	hitPointLocal_.Clear();

	float distance = 1000.0f;
	if (convexCallback.hasHit())
    {
		hasHit_ = true;
		hitPoints_ = convexCallback.m_hitPointWorld.size();
		for (int i = 0; i < convexCallback.m_hitPointWorld.size(); i++)
		{
			hitPoint_ = ToVector3(convexCallback.m_hitPointWorld.at(i));
			hitPointWorld_.Push(hitPoint_);
			hitPointLocal_.Push(ToVector3(convexCallback.m_hitPointLocal.at(i)));
			hitNormal_ = ToVector3(convexCallback.m_hitNormalWorld.at(i));
			hitNormalWorld_.Push(hitNormal_);
			hitFraction_.Push((float)convexCallback.m_hitFractions.at(i));
			
			float d = (hitPoint_ - startPos).Length();
			hitDistance_.Push(d);
			if (distance > d)
			{
				distance = d;
				hitIndex_ = i;
			}
				
			hitBody_ = static_cast<RigidBody*>(convexCallback.m_collisionObjects[i]->getUserPointer());
		}
    }
}

void Raycastest::SetRadius(float r)
{
	radius_ = r;
	Quaternion rot(90.0f, Vector3::FORWARD);
	shape_->SetCylinder(radius_ * 2, radius_ * 4, Vector3::ZERO, rot);
	// shape_->SetSphere(radius_ * 2);
}
