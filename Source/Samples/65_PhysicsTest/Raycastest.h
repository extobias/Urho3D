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

	void SetOffset(const Vector3& offset) { offset_ = offset; }

	unsigned hitPoints_;
	Vector<float> hitDistance_;
	Vector<Vector3> hitPointWorld_;
	Vector<Vector3> hitPointLocal_;
	Vector<Vector3> hitNormalWorld_;
	Vector<float> hitFraction_;

	int hitIndex_;
	bool hasHit_;

	float suspensionRest_;
	Vector3 hardPointWS_;
	Vector3 offset_;
	Vector3 direction_;
	RigidBody* hitBody_;

	void SetRadius(float r);
	float GetRadius() const { return radius_; }

private:
    void Update();

	Vector3 hitPoint_;
	Vector3 hitNormal_;
	float radius_;

	CollisionShape* shape_;
    Node* shapeNode_;
};
