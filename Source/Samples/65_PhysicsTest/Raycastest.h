#pragma once

#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Physics/CollisionShape.h>

using namespace Urho3D;

class Raycastest : public LogicComponent
{
	URHO3D_OBJECT(Raycastest, LogicComponent)

public:
	explicit Raycastest(Context* context);
	
	static void RegisterObject(Context* context);

	void OnNodeSet(Node* node) override;

	virtual void FixedUpdate(float timeStep);

    virtual void PostUpdate(float timeStep);

	void RayCast();

	void DebugDraw();

	unsigned hitPoints_;
	Vector<float> distance_;
	Vector<Vector3> hitPointWorld_;
	Vector<Vector3> hitPointLocal_;
	Vector<Vector3> hitNormalWorld_;
	Vector<float> hitFraction_;

	float suspensionRest_;
	Vector3 hardPointWS_;
	Vector3 direction_;
	RigidBody* hitBody_;

private:
    void Update();

    
    float radius_;


	Vector3 hitPoint_;
	Vector3 hitNormal_;
	bool hasHit_;

	CollisionShape* shape_;
    Node* shapeNode_;
};
