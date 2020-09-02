#include <Urho3D/Core/Context.h>
#include <Urho3D/UI/UI.h>
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

namespace Urho3D
{
extern const char* URHO2D_CATEGORY;
}

Sinkhole::Sinkhole(Context* context)
    : LogicComponent (context),
      scoreText_(nullptr),
      sinkTimer_(0.0f),
      sinkMaxTimer_(5.0f),
      started_(true),
      staticPosition_(true)
{
}

Sinkhole::~Sinkhole() = default;

void Sinkhole::RegisterObject(Context* context)
{
    context->RegisterFactory<Sinkhole>(URHO2D_CATEGORY);

    URHO3D_ATTRIBUTE("Started", bool, started_, false, AM_FILE);
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
        if (!staticPosition_)
        {
            float x = Random(gField.Left(), gField.Right());
            float y = Random(gField.Top(), gField.Bottom());
            node_->SetPosition(Vector3(x, y, 0.0f));
        }

        unsigned color = (unsigned)Random(0, 3);
        node_->SetVar("Color", color);

        UpdateSprite(color);
    }
}

void Sinkhole::PostUpdate(float timeStep)
{
    if (!started_)
        return;

    String counterText;
    for(auto c: gCounters)
    {
        counterText.AppendWithFormat("type %i: %i\n\n", c.first_, c.second_);
    }
    scoreText_->SetText(counterText);

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

void Sinkhole::DelayedStart()
{
    // Create rigid body
    body_ = node_->CreateComponent<RigidBody2D>();
    body_->SetTemporary(true);
    body_->SetBodyType(BT_STATIC);

//    Vector2 force(Random(-10.0f, 10.0f), Random(-10.0f, 10.0f));
//    body_->ApplyForce(force, Vector2::ZERO, true);

    sprite_ = node_->CreateComponent<StaticSprite2D>();
    sprite_->SetTemporary(true);

    CreateShapes();

    UpdateSprite(0);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Node* textNode = node_->CreateChild("TextNode");
    textNode->SetTemporary(true);
    textNode->SetPosition(Vector3(-0.15f, 0.3f, 0.0));
    text3d_ = textNode->CreateComponent<Text3D>();
    text3d_ ->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 10.0f);

    SubscribeToEvent(GetNode(), E_NODEBEGINCONTACT2D, URHO3D_HANDLER(Sinkhole, HandlePhysicsBegin2D));

    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    scoreText_ = ui->GetRoot()->CreateChild<Text>();
    scoreText_->SetText("");
    scoreText_->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 15);

    // Position the text relative to the screen center
    scoreText_->SetHorizontalAlignment(HA_CENTER);
    scoreText_->SetVerticalAlignment(VA_TOP);
    // scoreText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
    scoreText_->SetPosition(0, 0);
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

    node->SetVar("Type", "Sink");
    node->SetScale(gPhysicsScale);
    node->SetPosition(node->GetPosition() * gScreenRatio);
}

void Sinkhole::CreateShapes()
{
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

