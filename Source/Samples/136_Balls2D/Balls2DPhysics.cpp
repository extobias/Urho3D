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
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/ConstraintMouse2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>
#include <Urho3D/Urho2D/CollisionEdge2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/IO/MemoryBuffer.h>

#include "Balls2DPhysics.h"
#include "Ball2D.h"
#include "BallSucker.h"
#include "BallRacket.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(Balls2DPhysics)

static const unsigned NUM_OBJECTS = 100;

Balls2DPhysics::Balls2DPhysics(Context* context) :
    Sample(context),
    pickedNode_(nullptr),
    tailNode_(nullptr),
    sinkNode_(nullptr),
    textNode_(nullptr),
    ballsNode_(nullptr),
    ballSuckerNode_(nullptr),
    ballRacketNode_(nullptr),
    ballTimer_(0.0f),
    sinkTimer_(0.0f),
    sinkMaxTimer_(0.0f),
    scalePhysics_(0.0f),
    debugDraw_(false)
{

    colors_.Push(Color::RED);
    colors_.Push(Color::GREEN);
    colors_.Push(Color::BLUE);
}

void Balls2DPhysics::Start()
{
    Ball2D::RegisterObject(context_);
    BallSucker::RegisterObject(context_);
    BallRacket::RegisterObject(context_);

    // Execute base class startup
    Sample::Start();

    CreateEditor();
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

    editor_->SetScene(scene_);
    editor_->SetCameraNode(cameraNode_);

    auto* graphics = GetSubsystem<Graphics>();
    Vector3 dpi = graphics->GetDisplayDPI();
    scalePhysics_ = dpi.z_ / 160.0f;

    URHO3D_LOGERRORF("dpi ddpi <%f> hdpi <%f> vdpi <%f> width <%i> height <%i>", dpi.x_, dpi.y_, dpi.z_, graphics->GetWidth(), graphics->GetHeight());
    camera_->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
//    camera_->SetOrthoSize(10.0f);
    // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)
//     camera_->SetZoom(Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f));
    camera_->SetZoom(1.0f);

    // Create 2D physics world component
    PhysicsWorld2D* physicsWorld = scene_->CreateComponent<PhysicsWorld2D>();
    physicsWorld->SetGravity(Vector2(0.0f, 0.0f));
    physicsWorld->SetDrawJoint(true);

    CreateWalls();

//    CreateSucker();

//    UpdateTimer();

    ballsNode_ = scene_->CreateChild("Balls");

    ballSuckerNode_ = scene_->CreateChild("Suckers");

    ballRacketNode_ = scene_->CreateChild("Rackets");

//    CreateBall();

//    CreateRacket(Vector2(0.0f, 2.0f * scalePhysics_));

//    CreateRacket(Vector2(0.0f, 0.0f * scalePhysics_));

//    CreateRacket(Vector2(0.0f, -2.0f * scalePhysics_));

//    auto* spriterAnimationSet = cache->GetResource<AnimationSet2D>("Urho2D/red-ball/red-ball.scml");
//    if (!spriterAnimationSet)
//        return;

//    spriterNode_ = scene_->CreateChild("SpriterAnimation");
//    auto* spriterAnimatedSprite = spriterNode_->CreateComponent<AnimatedSprite2D>();
//    spriterAnimatedSprite->SetAnimationSet(spriterAnimationSet);
//    spriterAnimatedSprite->SetAnimation(spriterAnimationSet->GetAnimation(0));

//    CreateSink();
//    CreateSink();
//    CreateSink();
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

void Balls2DPhysics::CreateWalls()
{

    //    // Create top.
    //    Node* topNode = scene_->CreateChild("TopGround");
    //    topNode->SetVar("Type", "Ground");
    //    topNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    ////    topNode->SetScale(Vector3(worldWidth / 2.0f, worldHeight / 2.0f, 0.0f));
    //    dummyBody_ = topNode->CreateComponent<RigidBody2D>();

    //    // Create box collider for ground
    //    auto* topShape = topNode->CreateComponent<CollisionBox2D>();
    //    // Set box size
    //    topShape->SetSize(Vector2(0.32f, 0.32f));
    //    // Set friction
    //    topShape->SetFriction(1.0f);

    //    auto* topSprite = topNode->CreateComponent<StaticSprite2D>();
    //    topSprite->SetSprite(boxSprite);

    auto* graphics = GetSubsystem<Graphics>();

    bool mobile = GetPlatform() == "Android" || GetPlatform() == "iOS";
    float screenWidth = mobile ? (float)graphics->GetWidth() : 360;
    float screenHeight = mobile ? (float)graphics->GetHeight() : 640;

    float aspect =  screenWidth / screenHeight;
    float halfViewSize = screenHeight * PIXEL_SIZE / 2.0f;
//    float worldWidth = (worldBottomRight.x_ - worldTopLeft.x_) * aspect;
//    float worldHeight = (worldTopLeft.y_ - worldBottomRight.y_);
    float paddingY = screenHeight * PIXEL_SIZE * 0.01f;
    float paddingX = screenHeight * PIXEL_SIZE * 0.1f;

    // left
    Node* edgeNode = scene_->CreateChild("VerticalEdge");
    auto* edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    auto* edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2((-halfViewSize + paddingX) * aspect, -halfViewSize + paddingY), Vector2((-halfViewSize + paddingX) * aspect, halfViewSize - paddingY));
    edgeShape->SetFriction(0.5f); // Set friction

    dummyBody_ = edgeNode->CreateComponent<RigidBody2D>();

    // right
    edgeNode = scene_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2((halfViewSize - paddingX) * aspect, -halfViewSize + paddingY), Vector2((halfViewSize - paddingX) * aspect, halfViewSize - paddingY));
    edgeShape->SetFriction(0.5f); // Set friction

    // top
    edgeNode = scene_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2((-halfViewSize + paddingX) * aspect, halfViewSize - paddingY), Vector2((halfViewSize - paddingX) * aspect, halfViewSize - paddingY));
    edgeShape->SetFriction(0.5f); // Set friction

    // bottom
    edgeNode = scene_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2((-halfViewSize + paddingX) * aspect, -halfViewSize + paddingY), Vector2((halfViewSize - paddingX) * aspect, -halfViewSize + paddingY));
    edgeShape->SetFriction(0.5f); // Set friction

    field_ = Rect(Vector2((-halfViewSize + paddingX) * aspect, -halfViewSize + paddingY), Vector2((halfViewSize - paddingX) * aspect, halfViewSize - paddingY));

    //    // Create bottom.
    //    Node* bottomNode = scene_->CreateChild("BottomGround");
    //    bottomNode->SetVar("Type", "Ground");
    //    bottomNode->SetPosition(Vector3(0.0f, -3.0f, 0.0f));
    //    bottomNode->SetScale(Vector3(10.0f, 1.0f, 0.0f));
    //    bottomNode->CreateComponent<RigidBody2D>();

    //    // Create box collider for ground
    //    auto* bottomShape = bottomNode->CreateComponent<CollisionBox2D>();
    //    // Set box size
    //    bottomShape->SetSize(Vector2(0.32f, 0.32f));
    //    // Set friction
    //    bottomShape->SetFriction(1.0f);

    //    auto* bottomSprite = bottomNode->CreateComponent<StaticSprite2D>();
    //    bottomSprite->SetSprite(boxSprite);

    //    // Create left.
    //    Node* leftNode = scene_->CreateChild("LeftGround");
    //    leftNode->SetVar("Type", "Ground");
    //    leftNode->SetPosition(Vector3(worldTopLeft.x_, 0.0f, 0.0f));
    //    leftNode->SetScale(Vector3(1.0f, worldHeight, 0.0f));
    //    leftNode->CreateComponent<RigidBody2D>();

    //    auto* leftShape = leftNode->CreateComponent<CollisionBox2D>();
    //    // Set box size
    //    leftShape->SetSize(Vector2(0.32f, 0.32f));
    //    // Set friction
    //    leftShape->SetFriction(1.0f);

    //    auto* leftSprite = leftNode->CreateComponent<StaticSprite2D>();
    //    leftSprite->SetSprite(boxSprite);

    //    // Create right.
    //    Node* rightNode = scene_->CreateChild("RightGround");
    //    rightNode->SetVar("Type", "Ground");
    //    rightNode->SetPosition(Vector3(2.0f, 0.0f, 0.0f));
    //    rightNode->SetScale(Vector3(1.0f, 20.0f, 0.0f));
    //    rightNode->CreateComponent<RigidBody2D>();

    //    // Create box collider for ground
    //    auto* rightShape = rightNode->CreateComponent<CollisionBox2D>();
    //    // Set box size
    //    rightShape->SetSize(Vector2(0.32f, 0.32f));
    //    // Set friction
    //    rightShape->SetFriction(1.0f);

    //    auto* rightSprite = rightNode->CreateComponent<StaticSprite2D>();
    //    rightSprite->SetSprite(boxSprite);
}

void Balls2DPhysics::UpdateTimer()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    timerText_ = ui->GetRoot()->CreateChild<Text>();
    timerText_->SetText("9999");
    timerText_->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 30);

    // Position the text relative to the screen center
//    timerText_->SetHorizontalAlignment(HA_LEFT);
    timerText_->SetVerticalAlignment(VA_TOP);
//    timerText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);

    textNode_ = scene_->CreateChild("TextNode");
    textNode_->SetPosition(Vector3::ZERO);
    Text3D* text3d = textNode_->CreateComponent<Text3D>();
    text3d->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 20);
    text3d->SetText("TOBI");

}

void Balls2DPhysics::CreateSucker()
{
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    Node* node  = ballSuckerNode_->CreateChild("Sucker");
    node->SetPosition2D(1.0f * aspectRatio * scalePhysics_, 0.0f);
    BallSucker* sucker = node->CreateComponent<BallSucker>();
    sucker->SetScaleFactor(scalePhysics_);
    // ball->SetScaleFactor(Random(0.0001f, 0.0009f));
}

void Balls2DPhysics::CreateRacket(const Vector2& pos)
{
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    Node* node  = scene_->CreateChild("Racket");
    node->SetPosition2D(pos);
    BallRacket* racket = node->CreateComponent<BallRacket>();
    racket->SetScaleFactor(scalePhysics_);
}

void Balls2DPhysics::CreateBall()
{
    Node* node  = ballsNode_->CreateChild("Ball");
    node->SetVar("Type", "Ball");

    Ball2D* ball = node->CreateComponent<Ball2D>();
     unsigned color = (unsigned)Random(0, 3);
//    unsigned color = 0;
    ball->SetColor(colors_[color]);
    ball->SetScaleFactor(scalePhysics_);
    // ball->SetScaleFactor(Random(0.0001f, 0.0009f));

    node->SetVar("Color", color);

//    float areaTotal = 0.0f;
//    for (int i = 0; i < balls_.Size(); i++)
//    {
//        Ball2D* b = balls_.At(i);
//        if (b)
//        {
//            areaTotal += b->GetArea();
//        }
//        else
//        {
//            URHO3D_LOGERRORF("node not found <%p>", b);
//        }
//    }


//    if (areaTotal > 400.0f)
//        scene_->SetUpdateEnabled(false);
}

void Balls2DPhysics::CreateSink()
{
    if (!sinkNode_)
        sinkNode_ = scene_->CreateChild("Sink");

    sinkNode_->SetVar("Type", "Sink");

    auto* cache = GetSubsystem<ResourceCache>();
    auto* boxSprite = cache->GetResource<Sprite2D>("Urho2D/Box.png");

    Ball2D* ball = sinkNode_->GetOrCreateComponent<Ball2D>();
    ball->SetScaleFactor(scalePhysics_);
    ball->SetSprite(boxSprite);
    RigidBody2D* rigidBody = sinkNode_->GetComponent<RigidBody2D>();
    rigidBody->SetBodyType(BT_STATIC);

    unsigned color = (unsigned)Random(0, 3);
//    unsigned color = 0;
    ball->SetColor(colors_[color]);
    // ball->SetScaleFactor(Random(0.0001f, 0.0009f));
    sinkNode_->SetVar("Color", color);

    // URHO3D_LOGERRORF("RECT <%f, %f, %f, %f>", field_.Top(), field_.Left(), field_.Bottom(), field_.Right());
    float x = Random(field_.Left(), field_.Right());
    float y = Random(field_.Top(), field_.Bottom());
    sinkNode_->SetPosition(Vector3(x, y, 0.0f));
}

void Balls2DPhysics::CreateEditor()
{
    editor_ = new EditorWindow(context_);
    editor_->SetName("editor");
    //    editor_->SetCameraNode(cameraNode_);
    UI* ui = GetSubsystem<UI>();
    ui->GetRootModalElement()->AddChild(editor_);
    //    editor_->SetScene(scene_);

    editor_->BringToFront();
    editor_->SetPriority(100);
    editor_->CreateGuizmo();
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
//    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Balls2DPhysics, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Balls2DPhysics, HandleSceneUpdate));
//    UnsubscribeFromEvent(E_SCENEUPDATE);

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Balls2DPhysics, HandlePostUpdate));

    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Balls2DPhysics, HandleMouseButtonDown));

    SubscribeToEvent(E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(Balls2DPhysics, HandlePhysicsBegin2D));

    if (touchEnabled_)
        SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Balls2DPhysics, HandleTouchBegin3));

    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Balls2DPhysics, HandleKeyDown));
}

void Balls2DPhysics::DebugDraw()
{
    PhysicsWorld2D* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    physicsWorld->DrawDebugGeometry();

    const Vector<SharedPtr<Node> > children = ballSuckerNode_->GetChildren();
    for (Node* suckerNode: children) {
        BallSucker* sucker = suckerNode->GetComponent<BallSucker>();
        if (sucker)
            sucker->DebugDraw();
    }
}

void Balls2DPhysics::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    ballTimer_ += timeStep;
    if (ballTimer_ > 1.0f && scene_->IsUpdateEnabled())
    {
        ballTimer_ = 0.0f;
        CreateBall();
    }

    sinkTimer_ += timeStep;
//    char textBuffer[8];
//    sprintf(textBuffer, "%.1f", sinkMaxTimer_ - sinkTimer_);
//    timerText_->SetText(textBuffer);
    if (sinkTimer_ > sinkMaxTimer_)
    {
        sinkMaxTimer_ = Random(1.0f, 5.0f);
        sinkTimer_ = 0.0f;
        CreateSink();
    }

    // Quaternion rot = textNode_->GetRotation();
    const float ROT_VEL = 50.0f;
    float angle = timeStep * ROT_VEL;
    Quaternion rot(angle, Vector3(0.0f, 0.0f, 1.0f));
    // URHO3D_LOGERRORF("angle <%f>", angle);
//    textNode_->Rotate(rot);
}

void Balls2DPhysics::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (debugDraw_)
    {
        DebugDraw();
    }
}

Vector2 Balls2DPhysics::GetMousePositionXY()
{
    auto* input = GetSubsystem<Input>();
    auto* graphics = GetSubsystem<Graphics>();
    Vector3 screenPoint = Vector3((float)input->GetMousePosition().x_ / graphics->GetWidth(), (float)input->GetMousePosition().y_ / graphics->GetHeight(), 0.0f);
    Vector3 worldPoint = camera_->ScreenToWorldPoint(screenPoint);
//     URHO3D_LOGERRORF("worldpoint <%f, %f, %f>", worldPoint.x_, worldPoint.y_, worldPoint.z_);
    return Vector2(worldPoint.x_, worldPoint.y_);
}

void Balls2DPhysics::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    auto* input = GetSubsystem<Input>();
    auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    RigidBody2D* rigidBody = physicsWorld->GetRigidBody(input->GetMousePosition().x_, input->GetMousePosition().y_); // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        // evita seleccionar las paredes
        String name = rigidBody->GetNode()->GetName();
        if (name != "Ball")
            return;

        pickedNode_ = rigidBody->GetNode();
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        Ball2D *ball = pickedNode_->GetComponent<Ball2D>();
        ball->SetMagnetized(true);
        // staticSprite->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f)); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        ConstraintMouse2D* constraintMouse = pickedNode_->CreateComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(GetMousePositionXY());
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody_);
    }
//    Vector2 v2 = ToVector2(rigidBody->GetBody()->GetLocalPoint(ToB2Vec2(GetMousePositionXY())));
//    URHO3D_LOGERRORF("shape coords <%f, %f>", v2.x_, v2.y_);

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(Balls2DPhysics, HandleMouseMove));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(Balls2DPhysics, HandleMouseButtonUp));
}

void Balls2DPhysics::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode_)
    {
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        Ball2D *ball = pickedNode_->GetComponent<Ball2D>();
        ball->SetMagnetized(false);
        ball->SetChained(false);
        // staticSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Restore picked sprite color

        pickedNode_->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode_->RemoveComponent<ConstraintDistance2D>();
//        PODVector<ConstraintMouse2D*> comps;
//        pickedNode_->GetComponents<ConstraintMouse2D>(comps);
//        URHO3D_LOGERRORF("comps count <%i>", comps.Size());
        while (ball->next_)
        {
            Node* node = ball->next_;
            node->RemoveComponent<ConstraintDistance2D>();

            ball->next_ = nullptr;
            ball->SetChained(false);
            ball = node->GetComponent<Ball2D>();
        }
        pickedNode_ = nullptr;
        tailNode_ = nullptr;

        // sigsegv
//        volatile int *p = reinterpret_cast<volatile int*>(0);
//        *p = 0x1337D00D;
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

void Balls2DPhysics::HandleTouchBegin3(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGERRORF("Balls2DPhysics::HandleTouchBegin3");

    auto* graphics = GetSubsystem<Graphics>();
    auto* input = GetSubsystem<Input>();
    auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    using namespace TouchBegin;
    RigidBody2D* rigidBody = physicsWorld->GetRigidBody(eventData[P_X].GetInt(), eventData[P_Y].GetInt()); // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        String name = rigidBody->GetNode()->GetName();
        if (name == "TopGround" || name == "LeftGround" || name == "RightGround")
            return;

        pickedNode_ = rigidBody->GetNode();
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        // staticSprite->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f)); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        ConstraintMouse2D* constraintMouse = pickedNode_->CreateComponent<ConstraintMouse2D>();
        Vector3 pos = camera_->ScreenToWorldPoint(Vector3((float)eventData[P_X].GetInt() / graphics->GetWidth(), (float)eventData[P_Y].GetInt() / graphics->GetHeight(), 0.0f));
        constraintMouse->SetTarget(Vector2(pos.x_, pos.y_));
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody_);
    }
//    Vector2 v2 = ToVector2(rigidBody->GetBody()->GetLocalPoint(ToB2Vec2(GetMousePositionXY())));
//    URHO3D_LOGERRORF("shape coords <%f, %f>", v2.x_, v2.y_);

    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(Balls2DPhysics, HandleTouchMove3));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(Balls2DPhysics, HandleTouchEnd3));
}

void Balls2DPhysics::HandleTouchMove3(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGERRORF("Balls2DPhysics::HandleTouchMove3");

    if (pickedNode_)
    {
        auto* constraintMouse = pickedNode_->GetComponent<ConstraintMouse2D>();
        auto* graphics = GetSubsystem<Graphics>();
        using namespace TouchMove;
        Vector3 pos = camera_->ScreenToWorldPoint(Vector3(float(eventData[P_X].GetInt()) / graphics->GetWidth(), float(eventData[P_Y].GetInt()) / graphics->GetHeight(), 0.0f));
        constraintMouse->SetTarget(Vector2(pos.x_, pos.y_));
    }
}

void Balls2DPhysics::HandleTouchEnd3(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGERRORF("Balls2DPhysics::HandleTouchEnd3");

    if (pickedNode_)
    {
        auto* staticSprite = pickedNode_->GetComponent<StaticSprite2D>();
        // staticSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Restore picked sprite color

        pickedNode_->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode_->RemoveComponent<ConstraintDistance2D>();
//        PODVector<ConstraintMouse2D*> comps;
//        pickedNode_->GetComponents<ConstraintMouse2D>(comps);
//        URHO3D_LOGERRORF("comps count <%i>", comps.Size());
        Ball2D* ball = pickedNode_->GetComponent<Ball2D>();
        while (ball->next_)
        {
            Node* node = ball->next_;
            node->RemoveComponent<ConstraintDistance2D>();

            ball->next_ = nullptr;
            ball->SetChained(false);
            ball = node->GetComponent<Ball2D>();
        }
        pickedNode_ = nullptr;
        tailNode_ = nullptr;
    }

    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
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

    if (typeA.StartsWith("Sink") || typeB.StartsWith("Sink"))
    {
        if (colorA == colorB)
        {
            Node* node = typeA.StartsWith("Sink") ? nodeB : nodeA;

            if (pickedNode_ == node)
            {
                while (node)
                {
                    Ball2D *ball = node->GetComponent<Ball2D>();

                    Node* node2 = ball->next_;
                    node->Remove();

                    ball = node2 ? node2->GetComponent<Ball2D>() : nullptr;
                    node = node2;
                }

                pickedNode_ = nullptr;
                tailNode_ = nullptr;
            }
            return;
        }
    }

    if (pickedNode_ && colorA == colorB && (nodeA == pickedNode_ || nodeB == pickedNode_) && !typeA.StartsWith("Ground") && !typeB.StartsWith("Ground")
            && typeA.Length() && typeB.Length())
    {
        if (tailNode_ && (tailNode_ == nodeA || tailNode_ == nodeB))
            return;

        Node* otherNode = nodeA == pickedNode_ ? nodeB : nodeA;
        Ball2D* ball = otherNode->GetComponent<Ball2D>();
        if (!ball)
            return;

        if (ball->IsChained())
            return;

        ball->SetChained(true);
        RigidBody2D* otherRigidBody = otherNode->GetComponent<RigidBody2D>();

        Node* followNode = tailNode_ ? tailNode_ : pickedNode_;
        auto* graphics = GetSubsystem<Graphics>();
        float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();
        // chequear que no quede fuera de la cancha
        CollisionCircle2D *shape = followNode->GetComponent<CollisionCircle2D>();

        Vector2 newPos(followNode->GetPosition2D() + Vector2(0.32f * scalePhysics_, 0.0f));
        if (!field_.IsInside(newPos)) {
            Vector2 min = field_.min_;
            Vector2 max = field_.max_;

            // right
            if (newPos.x_ > max.x_)
                newPos = followNode->GetPosition2D() + Vector2(0.0f, 0.32f * scalePhysics_);
        }

        otherNode->SetPosition2D(newPos);

        ConstraintDistance2D* constraintDistance = followNode->CreateComponent<ConstraintDistance2D>();
//        constraintDistance->SetCollideConnected(true);
        constraintDistance->SetOtherBody(otherRigidBody);
        Vector2 p1 = followNode->GetPosition2D();
        Vector2 p2 = otherNode->GetPosition2D();
        constraintDistance->SetOwnerBodyAnchor(p1);
        constraintDistance->SetOtherBodyAnchor(p2);

        Ball2D* otherBall = followNode->GetComponent<Ball2D>();
        otherBall->next_ = otherNode;

        tailNode_ = otherNode;

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
        constraintDistance->SetFrequencyHz(1.0f);
        constraintDistance->SetDampingRatio(0.1f);
    }
}

void Balls2DPhysics::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    Sample::HandleKeyDown(eventType, eventData);

    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    if (key == KEY_R)
        debugDraw_ = !debugDraw_;
}

