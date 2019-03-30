#include "Raycastest.h"

#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>

Raycastest::Raycastest(Context* context)
	: LogicComponent(context)
{
	URHO3D_LOGERRORF("Raycastest::Raycastest");
}

void Raycastest::RegisterObject(Context* context)
{
	context->RegisterFactory<Raycastest>();
}

void Raycastest::OnNodeSet(Node* node)
{
	Node* wheelNode = node->GetScene()->CreateChild("wheel");
	Vector3 direction = Vector3::DOWN;
	Vector3 pos( + direction * suspensionRest_);
	wheelNode->SetPosition(node_->LocalToWorld(pos));

	//auto* wheelObject = wheelNode->CreateComponent<StaticModel>();
	//wheelObject->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));

	//auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
	wheel.wheelShape_ = wheelNode->CreateComponent<CollisionShape>();

	Quaternion rot(90.0f, Vector3::FORWARD);
	wheel.wheelShape_->SetCylinder(wheelRadius_ * 2, 0.5f, pos, rot);
}

void Raycastest::FixedUpdate(float timeStep)
{

}

void Raycastest::RayCast()
{

}