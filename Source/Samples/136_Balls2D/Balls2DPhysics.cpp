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
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionPolygon2D.h>
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
#include <Urho3D/IO/Log.h>

#include "Balls2DPhysics.h"
#include "Ball2D.h"
#include "BallSucker.h"
#include "BallRacket.h"
#include "Sinkhole.h"
#include "Pumper.h"
#include "InputManager.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(Balls2DPhysics)

static const unsigned NUM_OBJECTS = 100;

Node* gPickedNode = nullptr;

Node* gTailNode = nullptr;

Node* gCameraNode = nullptr;

RigidBody2D* gDummyBody = nullptr;

float gPhysicsScale;

Rect gField;
/// type/count
HashMap<unsigned, unsigned> gCounters;
/// type/minimum
HashMap<unsigned, unsigned> gLevelTargets;

Balls2DPhysics::Balls2DPhysics(Context* context) :
    Sample(context),
    sinkNode_(nullptr),
    textNode_(nullptr),
    ballsNode_(nullptr),
    ballSuckerNode_(nullptr),
    ballRacketNode_(nullptr),
    debugDraw_(true)
{
    gLevelTargets[0] = 5;
    gLevelTargets[1] = 5;
    gLevelTargets[2] = 5;

    gCounters[0] = 0;
    gCounters[1] = 0;
    gCounters[2] = 0;
}

void Balls2DPhysics::Start()
{
    Ball2D::RegisterObject(context_);
    BallSucker::RegisterObject(context_);
    BallRacket::RegisterObject(context_);
    Sinkhole::RegisterObject(context_);
    Pumper::RegisterObject(context_);
    InputManager::RegisterObject(context_);

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
    // inputManager_ = new (context_);

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    scene_->CreateComponent<InputManager>();

    // Create camera node
    gCameraNode = scene_->CreateChild("Camera");
    // Set camera's position
    gCameraNode->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    camera_ = gCameraNode->CreateComponent<Camera>();
    camera_->SetOrthographic(true);

    editor_->SetScene(scene_);
    editor_->SetCameraNode(gCameraNode);

    auto* graphics = GetSubsystem<Graphics>();
    Vector3 dpi = graphics->GetDisplayDPI();
    //scalePhysics_ = dpi.z_ / 160.0f;
    gPhysicsScale = dpi.z_ / 160.0f;

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

    CreateScore();

    CreateSink();

    ballsNode_ = scene_->CreateChild("Balls");

    ballSuckerNode_ = scene_->CreateChild("Suckers");

    ballRacketNode_ = scene_->CreateChild("Rackets");

    // CreateSucker();

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

void Balls2DPhysics::TestNodes()
{
    const char* names[] = {
        "Urho2D/Aster.png",
        "Urho2D/Ball.png",
        "Urho2D/Box.png",
        "Urho2D/GoldIcon.png",
    };

    auto* cache = GetSubsystem<ResourceCache>();
    for (int i = 0; i < 4; i++)
    {
        String name;
        Node* node = scene_->CreateChild(name.AppendWithFormat("Test-%i", i));
        auto* sprite2d = cache->GetResource<Sprite2D>(names[i]);
        auto* sprite = node->CreateComponent<StaticSprite2D>();
        sprite->SetSprite(sprite2d);

        if (i == 3)
        {
            PODVector<Vector2> vertices;
            vertices.Push(Vector2(-0.1f, 0.0f));
            vertices.Push(Vector2(0.1f, 0.0f));
            vertices.Push(Vector2(0.0f, 0.1f));

            CollisionPolygon2D* poly = node->CreateComponent<CollisionPolygon2D>();
            poly->SetVertices(vertices);
        }

        // octree->AddManualDrawable(sprite);

        URHO3D_LOGERRORF("Balls2DPhysics::CreateScene: BOUNDINGBOX <%s>", sprite->GetWorldBoundingBox().ToString().CString());
    }

    Node* node = scene_->CreateChild("Cube");
    StaticModel* mushroom= node->CreateComponent<StaticModel>();
    Model* mushroomModel = cache->GetResource<Model>("Models/Mushroom.mdl");
    mushroom->SetModel(mushroomModel);
    mushroom->SetCastShadows(true);
    mushroom->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));

    URHO3D_LOGERRORF("Balls2DPhysics::CreateScene: BOUNDINGBOX <%s>", mushroom->GetBoundingBox().ToString().CString());
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

    gDummyBody = edgeNode->CreateComponent<RigidBody2D>();

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

    gField = Rect(Vector2((-halfViewSize + paddingX) * aspect, -halfViewSize + paddingY), Vector2((halfViewSize - paddingX) * aspect, halfViewSize - paddingY));

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

void Balls2DPhysics::CreateScore()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    timerText_ = ui->GetRoot()->CreateChild<Text>();
    timerText_->SetText("");
    timerText_->SetFont(cache->GetResource<Font>("Fonts/BarcadeBrawlRegular-plYD.ttf"), 15);

    // Position the text relative to the screen center
    timerText_->SetHorizontalAlignment(HA_CENTER);
    timerText_->SetVerticalAlignment(VA_TOP);
    // timerText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
    timerText_->SetPosition(0, 0);
}

void Balls2DPhysics::CreateSucker()
{
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    Node* node  = ballSuckerNode_->CreateChild("Sucker");
    node->SetPosition2D(1.0f * aspectRatio * gPhysicsScale, 0.0f);
    BallSucker* sucker = node->CreateComponent<BallSucker>();
}

void Balls2DPhysics::CreateRacket(const Vector2& pos)
{
    auto* graphics = GetSubsystem<Graphics>();
    float aspectRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    Node* node  = scene_->CreateChild("Racket");
    node->SetPosition2D(pos);

    BallRacket* racket = node->CreateComponent<BallRacket>();
}

void Balls2DPhysics::CreateSink()
{
    sinkNode_ = scene_->CreateChild("Sink");
    sinkNode_->GetOrCreateComponent<Sinkhole>();

    Node* pumperNode = scene_->CreateChild("Pumper");
    pumperNode->CreateComponent<Pumper>();
}

void Balls2DPhysics::CreateEditor()
{
    editor_ = new EditorWindow(context_);
    editor_->SetName("editor");
    //    editor_->SetCameraNode(cameraNode_);
    UI* ui = GetSubsystem<UI>();
    ui->SetNonFocusedMouseWheel(true);
    ui->GetRootModalElement()->AddChild(editor_);
    //    editor_->SetScene(scene_);

    editor_->BringToFront();
    editor_->SetPriority(100);
    editor_->CreateGuizmo();
    editor_->SetVisible(false);
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
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, gCameraNode->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    engine_->SetMaxFps(30);
}

void Balls2DPhysics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
//    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Balls2DPhysics, HandleUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    // SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Balls2DPhysics, HandleSceneUpdate));
    // UnsubscribeFromEvent(E_SCENEUPDATE);

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Balls2DPhysics, HandlePostUpdate));

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

void Balls2DPhysics::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    String counterText;
    for(auto c: gCounters)
    {
        counterText.AppendWithFormat("type %i: %i\n", c.first_, c.second_);
    }
    timerText_->SetText(counterText);

    if (debugDraw_)
    {
        DebugDraw();
    }
}

void Balls2DPhysics::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    Sample::HandleKeyDown(eventType, eventData);

    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    if (key == KEY_R)
        debugDraw_ = !debugDraw_;

    if (key == KEY_P)
    {
        bool enabled = scene_->IsUpdateEnabled();
        scene_->SetUpdateEnabled(!enabled);
    }
}

