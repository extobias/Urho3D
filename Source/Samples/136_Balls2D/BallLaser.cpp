#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/Log.h>

#include "BallLaser.h"
#include "Ball2D.h"

BallLaser::BallLaser(Context* context)
    : LogicComponent (context),
      scaleFactor_(1.0f),
      magnetRadius_(1.5f),
      magnetForce_(1.0f)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

BallLaser::~BallLaser()
{

}

void BallLaser::RegisterObject(Context* context)
{
    context->RegisterFactory<BallLaser>();
}

void BallLaser::FixedUpdate(float timeStep)
{
    Vector2 pos = node_->GetPosition2D();
    for (int i = 0; i < balls_->Size(); i++)
    {
        Ball2D* b = balls_->At(i);
        if (b->IsMagnetized())
            continue;

        Node* node = b->GetNode();
        Vector2 ballPos = node->GetPosition2D();

        Vector2 d = pos - ballPos;
        float distance = d.Length();

        if (distance < magnetRadius_ * scaleFactor_)
        {
            RigidBody2D* rigidBody = node->GetComponent<RigidBody2D>();
            CollisionCircle2D* shape = node->GetComponent<CollisionCircle2D>();

            float vecSum = Abs(d.x_) + Abs(d.y_);
            Vector2 force = Vector2((magnetForce_ * d.x_ * ((1 / vecSum) * magnetRadius_ / distance)), (magnetForce_* d.y_ * ((1 / vecSum) * magnetRadius_ / distance)));
            // player.applyForceToCenter();
            rigidBody->ApplyForceToCenter(force, true);
        }
    }
}

void BallLaser::DebugDraw()
{
    auto* debug = node_->GetScene()->GetComponent<DebugRenderer>();
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    debug->AddCircle(node_->GetPosition(), Vector3::FORWARD, GetMagnetRadius() * scaleFactor_, Color::RED, 64, false);
}

void BallLaser::OnNodeSet(Node* node)
{
    if (!node)
        return;

    auto* cache = GetSubsystem<ResourceCache>();

//    node->SetPosition(Vector3::ZERO);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetBodyType(BT_STATIC);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    Sprite2D* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Sucker.png");
    IntRect r = ballSprite->GetRectangle();

    sprite_->SetSprite(ballSprite);

    // Create circle
    CollisionCircle2D *shape = node->CreateComponent<CollisionCircle2D>();
    // Set radius
    shape->SetRadius((float )r.Right() * PIXEL_SIZE / 2.0f);
    // Set density
    shape->SetDensity(1.0f);
    // Set friction.
    shape->SetFriction(0.5f);
    // Set restitution
    shape->SetRestitution(0.1f);

    shape_ = shape;
}

void BallLaser::SetScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;

   node_->SetScale(scaleFactor);
}

void BallLaser::SetBalls(Vector<WeakPtr<class Ball2D> >& balls)
{
    balls_ = &balls;
}
