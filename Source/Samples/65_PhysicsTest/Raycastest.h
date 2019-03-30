#pragma once

#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Core/Context.h>

using namespace Urho3D;

class Raycastest : public LogicComponent
{
	URHO3D_OBJECT(Raycastest, LogicComponent)

public:
	explicit Raycastest(Context* context);
	
	static void RegisterObject(Context* context);

	void OnNodeSet(Node* node) override;

	virtual void FixedUpdate(float timeStep);

	void RayCast();
};
