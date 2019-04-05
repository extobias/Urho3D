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

private:
    void Update();

    float suspensionRest_;
    float radius_;

    Vector3 hardPointWS_;
    Vector3 direction_;

    CollisionShape* shape_;
    Node* shapeNode_;
};
