
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
      next_(nullptr),
      scaleFactor_(1.0f),
      chained_(false),
      magnetized_(false)
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
//    if (node_->GetScale2D().x_ < 3.0f)
//        node_->Scale(1.0f + scaleFactor_);
//    UpdateArea();
}

void Ball2D::OnNodeSet(Node* node)
{
    if (!node)
        return;

    node->SetPosition(Vector3::ZERO);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetBodyType(BT_DYNAMIC);

    Vector2 force(Random(-10.0f, 10.0f), Random(-10.0f, 10.0f));
    body_->ApplyForce(force, Vector2::ZERO, true);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    sprite_->SetColor(color_);

    auto* cache = GetSubsystem<ResourceCache>();
    auto* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Ball.png");
    IntRect r = ballSprite->GetRectangle();
    // URHO3D_LOGERRORF("ball rect l <%i> r <%i> t <%i> b <%i>", r.Left(), r.Right(), r.Top(), r.Bottom());
    sprite_->SetSprite(ballSprite);

    // Create circle
    shape_ = node->CreateComponent<CollisionCircle2D>();
    // Set radius
    shape_->SetRadius((float )r.Right() * PIXEL_SIZE / 2.0f);
    // Set density
    shape_->SetDensity(1.0f);
    // Set friction.
    shape_->SetFriction(0.5f);
    // Set restitution
    shape_->SetRestitution(0.1f);

    UpdateArea();
}

void Ball2D::SetColor(const Color& color)
{
     color_ = color;

     if(sprite_)
         sprite_->SetColor(color);
}

void Ball2D::SetScaleFactor(float scaleFactor)
{
     scaleFactor_ = scaleFactor;

    node_->SetScale(scaleFactor);

//    shape_->SetRadius(0.16f * scaleFactor);
}

void Ball2D::SetSprite(Sprite2D* sprite)
{
    if (sprite_)
        sprite_->SetSprite(sprite);
}

void Ball2D::UpdateArea()
{
    float r = node_->GetScale2D().x_ * (1.0f + scaleFactor_);
    area_ = M_PI * r * r;
}

