#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionPolygon2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/Log.h>

#include "BallRacket.h"
#include "Ball2D.h"
#include "BallDefs.h"

namespace Urho3D
{
extern const char* URHO2D_CATEGORY;
}

BallRacket::BallRacket(Context* context)
    : LogicComponent (context),
      rotationVelocity_(50.0f),
      clockwise_(false)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

BallRacket::~BallRacket()
{

}

void BallRacket::RegisterObject(Context* context)
{
    context->RegisterFactory<BallRacket>(URHO2D_CATEGORY);

    URHO3D_ATTRIBUTE("Rotation Velocity", float, rotationVelocity_, 50.0f, AM_FILE);
    URHO3D_ATTRIBUTE("Clockwise", bool, clockwise_, false, AM_FILE);
}

void BallRacket::FixedUpdate(float timeStep)
{
    float angle = timeStep * rotationVelocity_;
    angle *= clockwise_ ? -1.0f : 1.0f;
    Quaternion rot(angle, Vector3(0.0f, 0.0f, 1.0f));
    // URHO3D_LOGERRORF("angle <%f>", angle);
    node_->Rotate(rot);
}

void BallRacket::DebugDraw()
{
    auto* debug = node_->GetScene()->GetComponent<DebugRenderer>();
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();
}

void BallRacket::OnNodeSet(Node* node)
{
    if (!node)
        return;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    node_->SetScale(gPhysicsScale);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetTemporary(true);
    body_->SetBodyType(BT_STATIC);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    sprite_->SetTemporary(true);
    Sprite2D* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Racket.png");
    IntRect r = ballSprite->GetRectangle();

    sprite_->SetSprite(ballSprite);

    // Create circle
    CollisionBox2D *shape = node->CreateComponent<CollisionBox2D>();
    shape->SetTemporary(true);
    shape->SetCenter(0.0f, 0.0f);
    shape->SetSize(Vector2((float )r.Width() * PIXEL_SIZE, r.Height() * PIXEL_SIZE));
    shape->SetDensity(1.0f);
    shape->SetFriction(0.5f);
    shape->SetRestitution(0.1f);

    CollisionCircle2D *circle = node->CreateComponent<CollisionCircle2D>();
    circle->SetTemporary(true);
    circle->SetRadius(0.32f);
    circle->SetDensity(1.0f);
    circle->SetFriction(0.5f);
    circle->SetRestitution(0.1f);

    shape_ = shape;
}

