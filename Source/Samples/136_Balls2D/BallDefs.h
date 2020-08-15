#pragma once

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

// sigsegv
// volatile int *p = reinterpret_cast<volatile int*>(0);
// *p = 0x1337D00D;

using namespace Urho3D;

extern float gPhysicsScale;

extern Node* gPickedNode;

extern Node* gTailNode;

extern Node* gCameraNode;

extern RigidBody2D* gDummyBody;

extern HashMap<unsigned, unsigned> gCounters;

extern HashMap<unsigned, unsigned> gLevelTargets;

extern Rect gField;

const static char* gSprites[] =
{
    "Urho2D/Box.png",
    "Urho2D/Ball.png",
    "Urho2D/Aster.png"
};
