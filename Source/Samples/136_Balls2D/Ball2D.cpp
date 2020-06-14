
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/Log.h>

#include "Ball2D.h"

Ball2D::Ball2D(Context* context)
    : LogicComponent (context),
      scaleFactor_(0.0f)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

Ball2D::~Ball2D()
{

}

void Ball2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Ball2D>();
}

void Ball2D::FixedUpdate(float timeStep)
{
    if (node_->GetScale2D().x_ < 3.0f)
        node_->Scale(1.0f + scaleFactor_);

    UpdateArea();
}

void Ball2D::SetColor(const Color& color)
{
     color_ = color;

     if(sprite_)
         sprite_->SetColor(color);
}

void Ball2D::OnNodeSet(Node* node)
{
    if (!node)
        return;

    node->SetPosition(Vector3(Random(-0.1f, 0.1f), -3.0f, 0.0f));

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetBodyType(BT_DYNAMIC);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    sprite_->SetColor(color_);

    auto* cache = GetSubsystem<ResourceCache>();
    auto* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Ball.png");
    sprite_->SetSprite(ballSprite);

    // Create circle
    shape_ = node->CreateComponent<CollisionCircle2D>();
    // Set radius
    shape_->SetRadius(0.16f);
    // Set density
    shape_->SetDensity(1.0f);
    // Set friction.
    shape_->SetFriction(0.5f);
    // Set restitution
    shape_->SetRestitution(0.1f);

    UpdateArea();
}

void Ball2D::UpdateArea()
{
    float r = node_->GetScale2D().x_ * (1.0f + scaleFactor_);
    area_ = M_PI * r * r;
}

