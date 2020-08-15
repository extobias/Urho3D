#include <Urho3D/Core/Context.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>

#include "Sinkhole.h"
#include "Ball2D.h"
#include "BallDefs.h"

Sinkhole::Sinkhole(Context* context)
    : LogicComponent (context),
      sinkTimer_(0.0f),
      sinkMaxTimer_(0.0f)
{
}

Sinkhole::~Sinkhole() = default;

void Sinkhole::RegisterObject(Context* context)
{
    context->RegisterFactory<Sinkhole>();
}

void Sinkhole::FixedUpdate(float timeStep)
{
    sinkTimer_ += timeStep;

    char buf[16];
    sprintf(buf, "%.1f", sinkMaxTimer_ - sinkTimer_);
    text3d_->SetText(buf);

    if (sinkTimer_ > sinkMaxTimer_)
    {
//        sinkMaxTimer_ = Random(1.0f, 5.0f);
        sinkMaxTimer_ = 5.0f;
        sinkTimer_ = 0.0f;

        // update position
        float x = Random(gField.Left(), gField.Right());
        float y = Random(gField.Top(), gField.Bottom());
        node_->SetPosition(Vector3(x, y, 0.0f));

        unsigned color = (unsigned)Random(0, 3);
        node_->SetVar("Color", color);

        UpdateSprite(color);
    }
}

void Sinkhole::PostUpdate(float timeStep)
{
    bool finish = true;
    unsigned count = gLevelTargets.Size();
    for(unsigned i = 0; i < count; i++)
    {
        if (gCounters[i] < gLevelTargets[i])
        {
            finish = false;
        }
    }

    if (finish)
    {
        GetScene()->SetUpdateEnabled(false);
    }
}

void Sinkhole::UpdateSprite(unsigned type)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Sprite2D* ballSprite = cache->GetResource<Sprite2D>(gSprites[type]);
    sprite_->SetSprite(ballSprite);

    for(unsigned i = 0; i < 3; i++)
    {
        shapes_.At(i)->SetEnabled(type == i);
    }
}

void Sinkhole::OnNodeSet(Node* node)
{
    if (!node)
        return;

    node->SetPosition(Vector3::ZERO);
    node->SetVar("Type", "Sink");
    node->SetScale(gPhysicsScale);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetTemporary(true);
    body_->SetBodyType(BT_STATIC);

    Vector2 force(Random(-10.0f, 10.0f), Random(-10.0f, 10.0f));
    body_->ApplyForce(force, Vector2::ZERO, true);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    sprite_->SetTemporary(true);

    for(unsigned i = 0; i < 3; i++)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Sprite2D* ballSprite = cache->GetResource<Sprite2D>(gSprites[i]);
        IntRect r = ballSprite->GetRectangle();

        CollisionShape2D* shape;
        if (i == 0)
        {
            CollisionBox2D* box = node_->CreateComponent<CollisionBox2D>();
            box->SetSize(Vector2(r.Bottom() * PIXEL_SIZE, r.Right() * PIXEL_SIZE));

            shape = box;
        }
        else
        {
            CollisionCircle2D* circle = node_->CreateComponent<CollisionCircle2D>();
            circle->SetRadius((float )r.Right() * PIXEL_SIZE / 2.0f);

            shape = circle;
        }
        shape->SetEnabled(false);
        shape->SetTemporary(true);
        shape->SetDensity(1.0f);
        shape->SetFriction(0.5f);
        shape->SetRestitution(0.1f);

        shapes_.Push(SharedPtr<CollisionShape2D>(shape));
    }

    UpdateSprite(0);

    auto* cache = GetSubsystem<ResourceCache>();
    Node* textNode = node->CreateChild("TextNode");
    textNode->SetPosition(Vector3(-0.15f, 0.3f, 0.0));
    text3d_ = textNode->CreateComponent<Text3D>();
    text3d_ ->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 10.0f);

    SubscribeToEvent(GetNode(), E_NODEBEGINCONTACT2D, URHO3D_HANDLER(Sinkhole, HandlePhysicsBegin2D));
}

void Sinkhole::HandlePhysicsBegin2D(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeBeginContact2D;

    Node* otherNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());

    unsigned colorA = node_->GetVar("Color").GetUInt();
    unsigned colorB = otherNode->GetVar("Color").GetUInt();

    if (colorA == colorB)
    {
        Node* node = otherNode;

        if (gPickedNode == node)
        {
            unsigned type = colorA;
            unsigned count = 0;

            while (node)
            {
                Ball2D *ball = node->GetComponent<Ball2D>();

                Node* node2 = ball->next_;
                node->Remove();

                ball = node2 ? node2->GetComponent<Ball2D>() : nullptr;
                node = node2;

                count++;
            }

            gCounters[type] += count;

            gPickedNode = nullptr;
            gTailNode = nullptr;
        }
    }
}

