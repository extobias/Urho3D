//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/ConstraintMouse2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/IO/MemoryBuffer.h>

#include "Balls2DPhysics.h"
#include "Ball2D.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(Balls2DPhysics)

static const unsigned NUM_OBJECTS = 100;

Balls2DPhysics::Balls2DPhysics(Context* context) :
    Sample(context),
    pickedNode_(nullptr),
    tailNode_(nullptr),
    ballTimer_(0.0f)
{
    colors_.Push(Color::RED);
    colors_.Push(Color::GREEN);
    colors_.Push(Color::BLUE);
}

void Balls2DPhysics::Start()
{
    Ball2D::RegisterObject(context_);
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
//    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Balls2DPhysics::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    camera_ = cameraNode_->CreateComponent<Camera>();
    camera_->SetOrthographic(true);

    auto* graphics = GetSubsystem<Graphics>();
    camera_->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)
    camera_->SetZoom(Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f));

    // Create 2D physics world component
    PhysicsWorld2D* physicsWorld = scene_->CreateComponent<PhysicsWorld2D>();
    physicsWorld->SetGravity(Vector2(0.0f, 3.0f));

    auto* cache = GetSubsystem<ResourceCache>();
    auto* boxSprite = cache->GetResource<Sprite2D>("Urho2D/Box.png");
    auto* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Ball.png");

    // Create top.
    Node* topNode = scene_->CreateChild("TopGround");
    topNode->SetPosition(Vector3(0.0f, 3.0f, 0.0f));
    topNode->SetScale(Vector3(10.0f, 1.0f, 0.0f));
    dummyBody_ = topNode->CreateComponent<RigidBody2D>();

    // Create box collider for ground
    auto* topShape = topNode->CreateComponent<CollisionBox2D>();
    // Set box size
    topShape->SetSize(Vector2(0.32f, 0.32f));
    // Set friction
    topShape->SetFriction(1.0f);

    auto* topSprite = topNode->CreateComponent<StaticSprite2D>();
    topSprite->SetSprite(boxSprite);

    // Create left.
    Node* leftNode = scene_->CreateChild("LeftGround");
    leftNode->SetPosition(Vector3(-2.0f, 0.0f, 0.0f));
    leftNode->SetScale(Vector3(1.0f, 20.0f, 0.0f));
    leftNode->CreateComponent<RigidBody2D>();

    auto* leftShape = leftNode->CreateComponent<CollisionBox2D>();
    // Set box size
    leftShape->SetSize(Vector2(0.32f, 0.32f));
    // Set friction
    leftShape->SetFriction(1.0f);

    auto* leftSprite = leftNode->CreateComponent<StaticSprite2D>();
    leftSprite->SetSprite(boxSprite);

    // Create right.
    Node* rightNode = scene_->CreateChild("RightGround");
    rightNode->SetPosition(Vector3(2.0f, 0.0f, 0.0f));
    rightNode->SetScale(Vector3(1.0f, 20.0f, 0.0f));
    rightNode->CreateComponent<RigidBody2D>();

    // Create box collider for ground
    auto* rightShape = rightNode->CreateComponent<CollisionBox2D>();
    // Set box size
    rightShape->SetSize(Vector2(0.32f, 0.32f));
    // Set friction
    rightShape->SetFriction(1.0f);

    auto* rightSprite = rightNode->CreateComponent<StaticSprite2D>();
    rightSprite->SetSprite(boxSprite);

    // Create ground.
//    for (int i = 0; i < 3; i++)
//    {
//        String name;
//        Node* groundNode = scene_->CreateChild(name.AppendWithFormat("GroundNode-%i", i));
//        groundNode->SetPosition(Vector3(1.0f * (i - 1), 2.5f, 0.0f));
//        groundNode->SetScale(Vector3(1.0f, 1.0f, 0.0f));
//        groundNode->CreateComponent<RigidBody2D>();
//        groundNode->SetVar("Type", name);
//        groundNode->SetVar("Color", i);

//        // Create box collider for ground
//        CollisionCircle2D* groundShape = groundNode->CreateComponent<CollisionCircle2D>();
//        // Set box size
//        groundShape->SetRadius(0.16f);
//        // Set friction
//        groundShape->SetFriction(1.0f);

//        auto* groundSprite = groundNode->CreateComponent<StaticSprite2D>();
//        groundSprite->SetColor(colors_[i]);
//        groundSprite->SetSprite(ballSprite);
//    }
}

void Balls2DPhysics::CreateBall()
{
    Node* node  = scene_->CreateChild("Ball");
    node->SetVar("Type", "Ball");

    Ball2D* ball = node->CreateComponent<Ball2D>();
    unsigned color = (unsigned)Random(0, 3);
    ball->SetColor(colors_[color]);
    // ball->SetScaleFactor(Random(0.0001f, 0.0009f));

    node->SetVar("Color", color);

    balls_.Push(WeakPtr<Ball2D>(ball));

    float areaTotal = 0.0f;
    for (int i = 0; i < balls_.Size(); i++)
    {
        Ball2D* b = balls_.At(i);
        if (b)
        {
            areaTotal += b->GetArea();
        }
        else
        {
            URHO3D_LOGERRORF("node not found <%p>", b);
        }
    }

    if (areaTotal > 400.0f)
        scene_->SetUpdateEnabled(false);
}

void Balls2DPhysics::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Balls2DPhysics::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Balls2DPhysics::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Balls2DPhysics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Balls2DPhysics, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);

    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Balls2DPhysics, HandleMouseButtonDown));

    SubscribeToEvent(E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(Balls2DPhysics, HandlePhysicsBegin2D));
}

void Balls2DPhysics::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    PhysicsWorld2D* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    physicsWorld->DrawDebugGeometry();

    ballTimer_ += timeStep;

    if (ballTimer_ > 1.0f && scene_->IsUpdateEnabled())
    {
        ballTimer_ = 0.0f;
        CreateBall();
    }
}

Vector2 Balls2DPhysics::GetMousePositionXY()
{
    auto* input = GetSubsystem<Input>();
    auto* graphics = GetSubsystem<Graphics>();
    Vector3 screenPoint = Vector3((float)input->GetMousePosition().x_ / graphics->GetWidth(), (float)input->GetMousePosition().y_ / graphics->GetHeight(), 0.0f);
    Vector3 worldPoint = camera_->ScreenToWorldPoint(screenPoint);
    return Vector2(worldPoint.x_, worldPoint.y_);
}

void Balls2DPhysics::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    auto* input = GetSubsystem<Input>();
    auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    RigidBody2D* rigidBody = physicsWorld->GetRigidBody(input->GetMousePosition().x_, input->GetMousePosition().y_); // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        pickedNode_ = rigidBody->GetNode();
        //log.Info(pickedNode.name);
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        // staticSprite->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f)); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        ConstraintMouse2D* constraintMouse = pickedNode_->CreateComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(GetMousePositionXY());
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody_);
    }
    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(Balls2DPhysics, HandleMouseMove));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(Balls2DPhysics, HandleMouseButtonUp));
}

void Balls2DPhysics::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode_)
    {
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        // staticSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Restore picked sprite color

        pickedNode_->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode_ = nullptr;
    }
    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONUP);
}

void Balls2DPhysics::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode_)
    {
        auto* constraintMouse = pickedNode_->GetComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(GetMousePositionXY());
    }
}

void Balls2DPhysics::HandlePhysicsBegin2D(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;
    Node* nodeA = static_cast<Node*>(eventData[P_NODEA].GetPtr());
    Node* nodeB = static_cast<Node*>(eventData[P_NODEB].GetPtr());

    if (!nodeA || !nodeB)
        return;

    String typeA, typeB;
    if (!nodeA->GetVar("Type").IsEmpty())
        typeA = nodeA->GetVar("Type").GetString();

    if (!nodeB->GetVar("Type").IsEmpty())
        typeB = nodeB->GetVar("Type").GetString();

    unsigned colorA = nodeA->GetVar("Color").GetUInt();
    unsigned colorB = nodeB->GetVar("Color").GetUInt();

    if (typeA.StartsWith("GroundNode") || typeB.StartsWith("GroundNode"))
    {
        if (colorA == colorB)
        {
            Node* node = typeA.StartsWith("GroundNode") ? nodeB : nodeA;

            if (pickedNode_ == node)
                pickedNode_ = nullptr;

            Ball2D *ball = node->GetComponent<Ball2D>();

            balls_.Remove(WeakPtr<Ball2D>(ball));

            node->Remove();
        }
    }

    if (pickedNode_ && colorA == colorB && (nodeA == pickedNode_ || nodeB == pickedNode_))
    {
        if (tailNode_ && (tailNode_ == nodeA || tailNode_ == nodeB))
            return;

        Node* otherNode = nodeA == pickedNode_ ? nodeB : nodeA;
        RigidBody2D* otherRigidBody = otherNode->GetComponent<RigidBody2D>();

        Node* followNode = tailNode_ ? tailNode_ : pickedNode_;
        ConstraintDistance2D* constraintDistance = followNode->CreateComponent<ConstraintDistance2D>();
        constraintDistance->SetCollideConnected(true);
        constraintDistance->SetOtherBody(otherRigidBody);
        Vector2 p1 = followNode->GetPosition2D();
        Vector2 p2 = otherNode->GetPosition2D();
        constraintDistance->SetOwnerBodyAnchor(p1);
        constraintDistance->SetOtherBodyAnchor(p2);

        tailNode_ = otherNode;



        MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
        while(!contacts.IsEof())
        {
            Vector2 contactPosition = contacts.ReadVector2();
            Vector2 contactNormal = contacts.ReadVector2();
            float overlapDistance = contacts.ReadFloat();

            Vector2 p3 = ToVector2(otherRigidBody->GetBody()->GetLocalPoint(ToB2Vec2(contactPosition)));
            Vector2 p4 = ToVector2(followNode->GetComponent<RigidBody2D>()->GetBody()->GetLocalPoint(ToB2Vec2(contactPosition)));

//            constraintDistance->SetOwnerBodyAnchor(contactPosition);
//            constraintDistance->SetOtherBodyAnchor(contactPosition);
        }
        // Make the constraint soft (comment to make it rigid, which is its basic behavior)
//        constraintDistance->SetFrequencyHz(4.0f);
//        constraintDistance->SetDampingRatio(0.5f);
    }
}

