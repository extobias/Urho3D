#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>
#include <Urho3D/Urho2D/ConstraintRevolute2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>

// borrar
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include "Ball2D.h"
#include "BallDefs.h"

Ball2D::Ball2D(Context* context)
    : LogicComponent (context),
      next_(nullptr),
      chained_(false),
      magnetized_(false)
{
    SetUpdateEventMask(USE_FIXEDUPDATE | USE_POSTUPDATE);
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

void Ball2D::PostUpdate(float timeStep)
{
    DebugRenderer* debugRenderer = GetScene()->GetComponent<DebugRenderer>();
    Quaternion rot = node_->GetRotation();
    Vector3 anchor = rot * Vector3(anchor_.x_, anchor_.y_, 0.0f);
    debugRenderer->AddCross(node_->GetPosition() + anchor, 0.10f, Color::GREEN);

    debugRenderer->AddLine(node_->GetPosition(), node_->GetPosition() + anchor, Color::GREEN);
}

void Ball2D::OnNodeSet(Node* node)
{
    if (!node)
        return;

//    URHO3D_LOGERRORF("Ball2D::OnNodeSet: physics scale <%f> node scale <%s>", gPhysicsScale, node->GetScale().ToString().CString());

    node->SetScale(gPhysicsScale);

    // Create rigid body
    body_ = node->CreateComponent<RigidBody2D>();
    body_->SetTemporary(true);
    body_->SetBodyType(BT_DYNAMIC);
    body_->SetMass(0.01f);
    body_->SetLinearDamping(0.0f);
    body_->SetAngularDamping(0.0f);

    sprite_ = node->CreateComponent<StaticSprite2D>();
    sprite_->SetTemporary(true);

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
        boxShape->SetTemporary(true);
        boxShape->SetSize(Vector2(r.Bottom() * PIXEL_SIZE, r.Right() * PIXEL_SIZE));

        shape_ = boxShape;
    }
    else
    {
        CollisionCircle2D* circleShape = node_->CreateComponent<CollisionCircle2D>();
        circleShape->SetTemporary(true);
        circleShape->SetRadius((float )r.Right() * PIXEL_SIZE / 2.0f);

        shape_ = circleShape;
    }

    shape_->SetDensity(1.0f);
    shape_->SetFriction(0.5f);
    shape_->SetRestitution(0.1f);

    shape_->SetTemporary(true);
}

float Ball2D::GetAnchorLength() const
{
    ConstraintRevolute2D* constraintRevolute = node_->GetComponent<ConstraintRevolute2D>();
    if (constraintRevolute)
    {
        b2Joint* joint = constraintRevolute->GetJoint();
        b2Vec2 p1 = joint->GetAnchorA();
        b2Vec2 p2 = joint->GetAnchorB();
        b2Vec2 d = p1 - p2;
        return d.Length();
    }
    return 0.0f;
}

void Ball2D::Unanchored()
{
    node_->RemoveComponent<ConstraintRevolute2D>();

    if (next_)
    {
        Ball2D* nextBall = next_->GetComponent<Ball2D>();
        nextBall->Unanchored();
        nextBall->SetChained(false);

        next_ = nullptr;
    }
}


void Ball2D::SetForce(const Vector2& force)
{
    body_->ApplyForce(force, Vector2::ZERO, true);
}

void Ball2D::SetNextAnchor(const Vector2& prevAnchor)
{
    // Vector3 position = node_->GetPosition();
    float currentRotation = node_->GetRotation2D();

    Quaternion rot(180.0f + currentRotation, Vector3::BACK);
    Vector3 newAnchor = rot * (Vector3(prevAnchor.x_, prevAnchor.y_, 0.0f));
    anchor_ = Vector2(newAnchor.x_, newAnchor.y_);

//    Quaternion rot(currentRotation, Vector3::BACK);
//    Vector3 newAnchor = rot * (Vector3(prevAnchor.x_, prevAnchor.y_, 0.0f));
//    anchor_ = Vector2(newAnchor.x_, newAnchor.y_);

    URHO3D_LOGERRORF("ball2d::setnextanchor <%s>", anchor_.ToString().CString());
//    b2Body* otherBody = body_->GetBody();
//    b2Vec2 localPoint = otherBody->GetLocalPoint(ToB2Vec2(prevAnchor));
//    b2Vec2 centerPoint = otherBody->GetWorldCenter();

//    b2Vec2 opositePoint = localPoint;
//    anchor_ = ToVector2(otherBody->GetWorldPoint(opositePoint));// - followNode->GetPosition2D();
}

Vector2 Ball2D::GetAnchor() const
{
//    float currentRotation = node_->GetRotation2D();
//    Quaternion rot(currentRotation, Vector3::BACK);
//    Vector3 newAnchor = rot * (Vector3(prevAnchor.x_, prevAnchor.y_, 0.0f));
    Quaternion rot = node_->GetRotation();
    Vector3 anchor = rot * Vector3(anchor_.x_, anchor_.y_, 0.0f);

    return Vector2(anchor.x_, anchor.y_);
}

void Ball2D::SetChained(bool chain)
{
    chained_ = chain;

    if (!chain)
    {
        anchor_ = Vector2::ZERO;
    }
}

void Ball2D::UpdateArea()
{
    float r = node_->GetScale2D().x_ * (1.0f + gPhysicsScale);
    area_ = M_PI * r * r;
}

void Ball2D::HandleBeginContact2D(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeBeginContact2D;

    if (node_ != gPickedNode)
        return;

    Node* otherNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());

    String typeA = node_->GetVar("Type").GetString();
    String typeB = otherNode->GetVar("Type").GetString();

    unsigned colorA = node_->GetVar("Color").GetUInt();
    unsigned colorB = otherNode->GetVar("Color").GetUInt();

//    URHO3D_LOGERRORF("Ball2D::HandleBeginContact2D: node_ <%p> gPicked <%p>", node_, gPickedNode);
    // collision between same colors balls
    if (gPickedNode && colorA == colorB && (node_ == gPickedNode || otherNode == gPickedNode) && !typeB.StartsWith("Ground") && typeB.Length())
    {
        if (gTailNode && (gTailNode == node_ || gTailNode == otherNode))
        {
            // URHO3D_LOGERRORF("sale 1");
            return;
        }

        // Node* other = node_ == gPickedNode ? otherNode : node_;
        Node* other = otherNode;
        Ball2D* otherBall = other->GetComponent<Ball2D>();

        if (!otherBall || otherBall->IsChained())
        {
            return;
        }

        otherBall->SetChained(true);

        Node* followNode = gTailNode ? gTailNode : gPickedNode;

        IntRect r = sprite_->GetSprite()->GetRectangle();
        float shapeRadius = (float)r.right_ * PIXEL_SIZE * gPhysicsScale;

        Ball2D* followBall = followNode->GetComponent<Ball2D>();
        followBall->SetChained(true);

        Vector2 prevAnchor;
        if (gTailNode) {
            prevAnchor = followBall->GetAnchor();
        } else {
            prevAnchor = Vector2((shapeRadius / 2.0f) - 0.01f, 0.0f);
        }

        Vector2 anchor = followNode->GetPosition2D() + prevAnchor;
        // position and rotation matched before anchored
        other->SetRotation2D(followNode->GetRotation2D());
        other->SetPosition2D(followNode->GetPosition2D());

        RigidBody2D* otherRigidBody = other->GetComponent<RigidBody2D>();
        RigidBody2D* followRigidBody = followNode->GetComponent<RigidBody2D>();

        otherBall->SetNextAnchor(prevAnchor);

        ConstraintRevolute2D* constraintRevolute = followNode->CreateComponent<ConstraintRevolute2D>();
//        constraintRevolute->SetEnableMotor(true);
//        constraintRevolute->SetMaxMotorTorque(10.0f);
//        constraintRevolute->SetMotorSpeed(0.10f);

        // las dos condiciones juntas (collideConnected y enableLimit)
        // no tienen sentido

        constraintRevolute->SetCollideConnected(true);
//        constraintRevolute->SetEnableLimit(true);
//        constraintRevolute->SetLowerAngle(-45.0f * M_DEGTORAD);
//        constraintRevolute->SetUpperAngle(45.0f * M_DEGTORAD);
        constraintRevolute->SetLocalAnchorA(ToVector2(followRigidBody->GetBody()->GetLocalPoint(ToB2Vec2(anchor))));
        constraintRevolute->SetLocalAnchorB(ToVector2(otherRigidBody->GetBody()->GetLocalPoint(ToB2Vec2(anchor))));
        constraintRevolute->SetOtherBody(otherRigidBody);
        constraintRevolute->SetAnchor(anchor);

        followBall->next_ = other;

        gTailNode = other;

//        GetScene()->SetUpdateEnabled(false);

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
