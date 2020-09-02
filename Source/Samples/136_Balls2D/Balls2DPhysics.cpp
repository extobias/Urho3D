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

float gScreenRatio;

Rect gField;
/// type/count
HashMap<unsigned, unsigned> gCounters;
/// type/minimum
HashMap<unsigned, unsigned> gLevelTargets;

Balls2DPhysics::Balls2DPhysics(Context* context) :
    Sample(context),
    wallsNode_(nullptr),
    debugDraw_(true)
{
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
    // CreateInstructions();

    // Setup the viewport for displaying the scene
//    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Balls2DPhysics::CreateScene()
{
    scene_ = new Scene(context_);

    CalcSizeScales();

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SharedPtr<File> file = cache->GetFile("Data/Scenes/goals_3.xml");
    scene_->LoadAsyncXML(file);

    editor_->SetVisible(false);

//    auto* spriterAnimationSet = cache->GetResource<AnimationSet2D>("Urho2D/red-ball/red-ball.scml");
//    if (!spriterAnimationSet)
//        return;

//    spriterNode_ = scene_->CreateChild("SpriterAnimation");
//    auto* spriterAnimatedSprite = spriterNode_->CreateComponent<AnimatedSprite2D>();
//    spriterAnimatedSprite->SetAnimationSet(spriterAnimationSet);
//    spriterAnimatedSprite->SetAnimation(spriterAnimationSet->GetAnimation(0));
}

Node* Balls2DPhysics::CreateCameraNode()
{
    Node* node = scene_->CreateChild("Camera");
    node->SetTemporary(true);
    node->SetPosition(Vector3(0.0f, 0.0f, -1.0f));

    camera_ = node->CreateComponent<Camera>();
    camera_->SetOrthographic(true);

    editor_->SetCameraNode(node);

    Graphics* graphics = GetSubsystem<Graphics>();

    camera_->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera_->SetZoom(1.0f);

    return node;
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
    wallsNode_ = scene_->CreateChild("Walls");
    wallsNode_->SetTemporary(true);

    StaticSprite2D* sprite = wallsNode_->CreateComponent<StaticSprite2D>();
    sprite->SetDrawRect(gField);
    sprite->SetUseDrawRect(true);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Sprite2D* sprite2d = cache->GetResource<Sprite2D>("Urho2D/fondo-2d.png");
    sprite->SetSprite(sprite2d);

    // left
    Node* edgeNode = wallsNode_->CreateChild("VerticalEdge");
    auto* edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    auto* edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2(gField.Left(), gField.Top()), Vector2(gField.Left(), gField.Bottom()));
    edgeShape->SetFriction(0.5f); // Set friction

    // right
    edgeNode = wallsNode_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2(gField.Right(), gField.Top()), Vector2(gField.Right(), gField.Bottom()));
    edgeShape->SetFriction(0.5f); // Set friction

    // top
    edgeNode = wallsNode_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2(gField.Left(), gField.Top()), Vector2(gField.Right(), gField.Top()));
    edgeShape->SetFriction(0.5f); // Set friction

    // bottom
    edgeNode = wallsNode_->CreateChild("VerticalEdge");
    edgeBody = edgeNode->CreateComponent<RigidBody2D>();
    edgeShape = edgeNode->CreateComponent<CollisionEdge2D>();
    edgeShape->SetVertices(Vector2(gField.Left(), gField.Bottom()), Vector2(gField.Right(), gField.Bottom()));
    edgeShape->SetFriction(0.5f); // Set friction

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
    editor_->SetVisible(true);
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
    Renderer* renderer = GetSubsystem<Renderer>();

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

    SubscribeToEvent(E_ASYNCLOADFINISHED, URHO3D_HANDLER(Balls2DPhysics, HandleSceneLoaded));
}

void Balls2DPhysics::DebugDraw()
{
    PhysicsWorld2D* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    physicsWorld->DrawDebugGeometry();

//    if (!suckerNode_)
//        return;

//    const Vector<SharedPtr<Node> > children = suckerNode_->GetChildren();
//    for (Node* suckerNode: children) {
//        BallSucker* sucker = suckerNode->GetComponent<BallSucker>();
//        if (sucker)
//            sucker->DebugDraw();
//    }
}

void Balls2DPhysics::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
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

void Balls2DPhysics::HandleSceneLoaded(StringHash eventType, VariantMap& eventData)
{
    using namespace AsyncLoadFinished;

    URHO3D_LOGERRORF("async scene loaded!");

    Scene* scene = static_cast<Scene*>(eventData[P_SCENE].GetPtr());

    gCameraNode = CreateCameraNode();

    editor_->SetCameraNode(gCameraNode);
    editor_->SetScene(scene);

    SetupViewport();

    Camera* camera = gCameraNode->GetComponent<Camera>();
    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->GetViewport(0)->SetCamera(camera);

    CreateWalls();

    Node* edgeNode = scene_->GetChild("VerticalEdge", true);
    gDummyBody = edgeNode->CreateComponent<RigidBody2D>();

    LoadLevelGoals();
}

void Balls2DPhysics::LoadLevelGoals()
{
    String goal = scene_->GetVar(StringHash("goals")).GetString();
    Vector<String> goals = goal.Split(';');
    for(unsigned i = 0; i < goals.Size(); i++)
    {
        gLevelTargets[i] = (unsigned)atoi(goals.At(i).CString());
        gCounters[i] = 0;
    }
}

void Balls2DPhysics::CalcSizeScales()
{
    Graphics* graphics = GetSubsystem<Graphics>();

    float logicalWidth = 360;
    float logicalHeight = 640;

    bool mobile = GetPlatform() == "Android" || GetPlatform() == "iOS";
    float screenWidth = mobile ? (float)graphics->GetWidth() : 360;
    float screenHeight = mobile ? (float)graphics->GetHeight() : 640;

    float xRatio = screenWidth / logicalWidth;
    float yRatio = screenHeight / logicalHeight;

    // smaller size fits into the display
    float scaleFactor = 0.0f;
    float borderPaddingX = 0.0f;
    float borderPaddingY = 0.0f;
    if (xRatio < yRatio)
    {
        scaleFactor = xRatio;
        borderPaddingX = 0.0f;
        borderPaddingY = screenHeight - (logicalHeight * scaleFactor);
    }
    else
    {
        scaleFactor = yRatio;
        borderPaddingX = screenWidth - (logicalWidth * scaleFactor);
        borderPaddingY = 0.0f;
    }

    gScreenRatio = scaleFactor;

    // float aspect =  screenWidth / screenHeight;
    float paddingY = screenHeight * PIXEL_SIZE * 0.0f;
    float paddingX = screenHeight * PIXEL_SIZE * 0.0f;

    float halfViewSizeX = (screenWidth - borderPaddingX)* PIXEL_SIZE / 2.0f;
    float halfViewSizeY = (screenHeight - borderPaddingY) * PIXEL_SIZE / 2.0f;

    gField = Rect(Vector2(-halfViewSizeX + paddingX, -halfViewSizeY + paddingY), Vector2(halfViewSizeX - paddingX, halfViewSizeY - paddingY));

    URHO3D_LOGERRORF("Balls2DPhysics::CalcField: rect field <%s>", gField.ToString().CString());

    Vector3 dpi = graphics->GetDisplayDPI();
    // 160.0f normal screen densities
    gPhysicsScale = dpi.z_ / 160.0f;

    float screenRatio = (float)graphics->GetWidth() / (float)graphics->GetHeight();

    URHO3D_LOGERRORF("dpi ddpi <%f> hdpi <%f> vdpi <%f> width <%i> height <%i> scale <%f> orthoSize <%f> aspectRatio <%f> screenRatio <%f>",
                     dpi.x_, dpi.y_, dpi.z_, graphics->GetWidth(), graphics->GetHeight(), gPhysicsScale, (float)graphics->GetHeight() * PIXEL_SIZE, screenRatio, gScreenRatio);
}


