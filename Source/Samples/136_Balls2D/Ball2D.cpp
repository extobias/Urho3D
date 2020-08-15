#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "Ball2D.h"
#include "BallDefs.h"

Ball2D::Ball2D(Context* context)
    : LogicComponent (context),
      next_(nullptr),
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
    if (chained_)
    {
        sprite_->SetColor(Color::RED);
    }
    else
    {
        sprite_->SetColor(Color::WHITE);
    }
}

void Ball2D::OnNodeSet(Node* node)
    {
    if (!node)
        return;

    node->SetPosition(Vector3::ZERO);
    node->SetScale(gPhysicsScale);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetBodyType(BT_DYNAMIC);

    Vector2 force(Random(-10.0f, 10.0f), Random(-10.0f, 10.0f));
    body_->ApplyForce(force, Vector2::ZERO, true);

    sprite_ = node->CreateComponent<StaticSprite2D>();

    UpdateArea();

    SubscribeToEvent(node, E_NODEBEGINCONTACT2D, URHO3D_HANDLER(Ball2D, HandleBeginContact2D));
}

void Ball2D::SetType(unsigned type)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Sprite2D* ballSprite = cache->GetResource<Sprite2D>(gSprites[type]);
    IntRect r = ballSprite->GetRectangle();
    // URHO3D_LOGERRORF("ball rect l <%i> r <%i> t <%i> b <%i>", r.Left(), r.Right(), r.Top(), r.Bottom());
    sprite_->SetSprite(ballSprite);

    if (type == 0)
    {
        CollisionBox2D* boxShape = node_->CreateComponent<CollisionBox2D>();
        boxShape->SetSize(Vector2(r.Bottom() * PIXEL_SIZE, r.Right() * PIXEL_SIZE));

        shape_ = boxShape;
    }
    else
    {
        CollisionCircle2D* circleShape = node_->CreateComponent<CollisionCircle2D>();
        circleShape->SetRadius((float )r.Right() * PIXEL_SIZE / 2.0f);

        shape_ = circleShape;
    }

    shape_->SetDensity(1.0f);
    shape_->SetFriction(0.5f);
    shape_->SetRestitution(0.1f);

    shape_->SetTemporary(true);
}

void Ball2D::UpdateArea()
{
    float r = node_->GetScale2D().x_ * (1.0f + gPhysicsScale);
    area_ = M_PI * r * r;
}

void Ball2D::HandleBeginContact2D(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeBeginContact2D;

    Node* otherNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());

    String typeA = node_->GetVar("Type").GetString();
    String typeB = otherNode->GetVar("Type").GetString();

    unsigned colorA = node_->GetVar("Color").GetUInt();
    unsigned colorB = otherNode->GetVar("Color").GetUInt();

    // collision between same colors balls
    if (gPickedNode && colorA == colorB && (node_ == gPickedNode || otherNode == gPickedNode) && !typeB.StartsWith("Ground") && typeB.Length())
    {
        if (gTailNode && (gTailNode == node_ || gTailNode == otherNode))
            return;

        Node* other = node_ == gPickedNode ? otherNode : node_;
        Ball2D* ball = other->GetComponent<Ball2D>();
        if (!ball)
            return;

        if (ball->IsChained())
            return;

        ball->SetChained(true);
        RigidBody2D* otherRigidBody = other->GetComponent<RigidBody2D>();

        Node* followNode = gTailNode ? gTailNode : gPickedNode;

        IntRect r = sprite_->GetSprite()->GetRectangle();
        float shapeRadius = (float)r.right_ * PIXEL_SIZE * gPhysicsScale;
        // chequear que no quede fuera de la cancha
        Vector2 newPos(followNode->GetPosition2D() + Vector2(shapeRadius, 0.0f));
        if (!gField.IsInside(newPos))
        {
            Vector2 min = gField.min_;
            Vector2 max = gField.max_;

            // right
            if (newPos.x_ > max.x_)
                newPos = followNode->GetPosition2D() + Vector2(0.0f, shapeRadius);
        }

        other->SetPosition2D(newPos);

        ConstraintDistance2D* constraintDistance = followNode->CreateComponent<ConstraintDistance2D>();
//        constraintDistance->SetCollideConnected(true);
        constraintDistance->SetOtherBody(otherRigidBody);
        Vector2 p1 = followNode->GetPosition2D();
        Vector2 p2 = other->GetPosition2D();
        constraintDistance->SetOwnerBodyAnchor(p1);
        constraintDistance->SetOtherBodyAnchor(p2);

        Ball2D* otherBall = followNode->GetComponent<Ball2D>();
        otherBall->next_ = other;

        gTailNode = other;

//        MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
//        while(!contacts.IsEof())
//        {
//            Vector2 contactPosition = contacts.ReadVector2();
//            Vector2 contactNormal = contacts.ReadVector2();
//            float overlapDistance = contacts.ReadFloat();
//            Vector2 p3 = ToVector2(otherRigidBody->GetBody()->GetLocalPoint(ToB2Vec2(contactPosition)));
//            Vector2 p4 = ToVector2(followNode->GetComponent<RigidBody2D>()->GetBody()->GetLocalPoint(ToB2Vec2(contactPosition)));
////            constraintDistance->SetOwnerBodyAnchor(contactPosition);
////            constraintDistance->SetOtherBodyAnchor(contactPosition);
//        }
        // Make the constraint soft (comment to make it rigid, which is its basic behavior)
//        constraintDistance->SetFrequencyHz(1.0f);
//        constraintDistance->SetDampingRatio(0.1f);
    }
}
