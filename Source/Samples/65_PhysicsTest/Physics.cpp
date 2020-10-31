//
// Copyright (c) 2008-2017 the Urho3D project.
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
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "Physics.h"
#include "Raycastest.h"
#include "ConvexCast.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(Physics)

const float CAMERA_DISTANCE = 10.0f;

Physics::Physics(Context* context) :
    Sample(context),
    drawDebug_(false),
    mass_(100.0f),
    length_(2.0f),
    width_(1.0f),
    suspensionStiffness_(500.0f),
    suspensionLength_(2.0f),
    suspensionRest_(0.8f),
    suspensionMinRest_(0.5f),
    suspensionDelta_(0.1f),
    suspensionDamping_(1.0f),
    updateLinSuspension_(false),
    updateRotSuspension_(false),
    floatHeight_(1.2f),
    k_(3500.0f),
    kFactor_(0),
    maxImpulse_(0.0f),
    timeElapsed_(0.0f),
    debugText_(nullptr)
{
    ConvexCast::RegisterObject(context);
}

void Physics::Start()
{
    engine_->SetMaxFps(60.0f);
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    editor_ = new EditorWindow(context_);
    UI* ui = GetSubsystem<UI>();
    ui->GetRootModalElement()->AddChild(editor_);

    editor_->SetName("editor");
    editor_->SetScene(scene_);
    editor_->BringToFront();
    editor_->SetPriority(100);
    editor_->CreateGuizmo();
    editor_->SetOrthographic(false);
    editor_->SetVisible(false);
    // Create the UI content
    // CreateInstructions();
    CreateDebugText();
    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Physics::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    engine_->SetMaxFps(30);
    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Create a physics simulation world with default parameters, which will update at 60fps. Like the Octree must
    // exist before creating drawable components, the PhysicsWorld must exist before creating physics components.
    // Finally, create a DebugRenderer component so that we can draw physics debug geometry
    scene_->CreateComponent<Octree>();
    PhysicsWorld* pw = scene_->CreateComponent<PhysicsWorld>();
    pw->SetFps(30.0f);
    scene_->CreateComponent<DebugRenderer>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    //zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    //zone->SetFogColor(Color(1.0f, 1.0f, 1.0f));
    //zone->SetFogStart(300.0f);
    //zone->SetFogEnd(500.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
//    Node* skyNode = scene_->CreateChild("Sky");
//    skyNode->SetScale(500.0f); // The scale actually does not matter
//    Skybox* skybox = skyNode->CreateComponent<Skybox>();
//    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
//    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    unsigned collisionMask = 1 << 0;
    // Create a floor object, 1000 x 1000 world units. Adjust position so that the ground is at zero Y
    Node* floorNode = scene_->CreateChild("Floor");
    floorNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    floorNode->SetScale(Vector3(100.0f, 0.0f, 100.0f));
    StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
    floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

    // Make the floor physical by adding RigidBody and CollisionShape components. The RigidBody's default
    // parameters make the object static (zero mass.) Note that a CollisionShape by itself will not participate
    // in the physics simulation
    RigidBody* body = floorNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(collisionMask);
    body->SetMass(0.0f);
    CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
    // Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene node scale, so the
    // rendering and physics representation sizes should match (the box model is also 1 x 1 x 1.)
    shape->SetBox(Vector3::ONE);

    //Node* terrainNode = scene_->CreateChild("Terrain");
    //terrainNode->SetPosition(Vector3::ZERO);
    //auto* terrain = terrainNode->CreateComponent<Terrain>();
    //terrain->SetPatchSize(64);
    //terrain->SetSpacing(Vector3(2.0f, 0.1f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
    //terrain->SetSmoothing(true);
    //terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
    //terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
    //// The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
    //// terrain patches and other objects behind it
    //terrain->SetOccluder(true);

    //auto* body = terrainNode->CreateComponent<RigidBody>();
    //body->SetCollisionLayer(1 << 0); // Use layer bitmask 2 for static geometry
    //auto* shape = terrainNode->CreateComponent<CollisionShape>();
    //shape->SetTerrain();

    Vector3 targetPosition(Vector3(0.0f, 5.0f, 0.0f));

    node_ = scene_->CreateChild("Box");
    node_->SetScale(Vector3(width_, 0.7f, length_));
    node_->SetPosition(targetPosition);

    StaticModel* boxObject = node_->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
    boxObject->SetCastShadows(true);

    // Create RigidBody and CollisionShape components like above. Give the RigidBody mass to make it movable
    // and also adjust friction. The actual mass is not important; only the mass ratios between colliding
    // objects are significant
    body_ = node_->CreateComponent<RigidBody>();
    body_->SetCollisionLayer(1 << 1);
    body_->SetCollisionMask(collisionMask);
    body_->SetMass(mass_);
    body_->SetRestitution(1.0f);
    // body_->SetKinematic(true);
    // body_->SetFriction(0.75f);
    // body_->SetAngularFactor(Vector3::ZERO);
    // body_->SetLinearFactor(Vector3::UP);

    CollisionShape* boxShape = node_->CreateComponent<CollisionShape>();
    boxShape->SetBox(Vector3::ONE);

    Vector<Vector3> wheelPos;
    float offset = suspensionDelta_;
    wheelPos.Push(Vector3(0.5f - offset, 0.0f, 1.0f - offset));
    wheelPos.Push(Vector3(-0.5f + offset, 0.0f, 1.0f - offset));
    wheelPos.Push(Vector3(0.5f - offset, 0.0f, -1.0f + offset));
    wheelPos.Push(Vector3(-0.5f + offset, 0.0f, -1.0f + offset));

    for (unsigned i = 0; i < wheelPos.Size(); i++)
    {
        WheelInfo wheel;
        wheel.chassisConnectionPointCS_ = wheelPos.At(i);
        wheel.raycastInfo_.suspensionLength_ = suspensionLength_;
        wheel.raycastInfo_.suspensionMinRest_ = suspensionMinRest_;
        wheel.raycastInfo_.suspensionRelativeVelocity_ = 0.0f;
        wheel.raycastInfo_.suspensionMaxRest_ = 0.8f;
        wheel.wheelDirectionCS_ = Vector3::DOWN;
        wheel.wheelAxle_ = Vector3::RIGHT;
        wheelsInfo_.Push(wheel);

        Node* wheelNode = scene_->CreateChild(String().AppendWithFormat("wheel-%i", i));
        wheelsNode_.Push(SharedPtr<Node>(wheelNode));

        wheelNode->CreateComponent<ConvexCast>();
    }

    // Create the camera. Set far clip to match the fog. Note: now we actually create the camera node outside the scene, because
    // we want it to be unaffected by scene load / save
    cameraNode_ = scene_->CreateChild("CameraNode");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(500.0f);
    cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -5.0f));
    cameraNode_->LookAt(Vector3::ZERO);
    cameraNode_->SetRotation(Quaternion(45.0f, Vector3::RIGHT));
}

void Physics::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse/touch to move\n"
        "LMB to spawn physics objects\n"
        "F5 to save scene, F7 to load\n"
        "Space to toggle physics debug geometry"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 5);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Physics::CreateDebugText()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    debugText_ = ui->GetRoot()->CreateChild<Text>();

    debugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 10);
    // The text has multiple rows. Center them in relation to each other
    debugText_->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    debugText_->SetHorizontalAlignment(HA_LEFT);
    debugText_->SetVerticalAlignment(VA_CENTER);
    debugText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Physics::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

//    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
//    effectRenderPath->Load(cache->GetResource<XMLFile>("CoreData/RenderPaths/PBRDeferred.xml"));
//    viewport->SetRenderPath(effectRenderPath);
}

void Physics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Physics, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Physics, HandlePostRenderUpdate));

    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Physics, HandleKeyDown));

    Component* world = scene_->GetComponent<PhysicsWorld>();
    SubscribeToEvent(world, E_PHYSICSPRESTEP, URHO3D_HANDLER(Physics, HandlePhysicsPreStep));

    SubscribeToEvent(world, E_PHYSICSPOSTSTEP, URHO3D_HANDLER(Physics, HandlePhysicsPostStep));
}

void Physics::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 1.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
        IntVector2 mouseMove = input->GetMouseMove();
        Quaternion camRot = cameraNode_->GetRotation();

        yaw_ = camRot.YawAngle() + MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ = camRot.PitchAngle() + MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }


    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_E))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_Q))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_C))
    {
        Vector3 camPos = cameraNode_->GetPosition();
        // cameraNode_->SetPosition(Vector3(camPos.x_, 0.0f, camPos.z_));
        cameraNode_->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
    }

    if (input->GetKeyDown(KEY_DOWN))
    {
        pitch_ += MOUSE_SENSITIVITY * 10.0f;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }
    if (input->GetKeyDown(KEY_UP))
    {
        pitch_ -= MOUSE_SENSITIVITY * 10.0f;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // "Shoot" a physics object with left mousebutton
//    if (input->GetMouseButtonDown(MOUSEB_LEFT))
//        SpawnObject();

    // Check for loading/saving the scene. Save the scene to the file Data/Scenes/Physics.xml relative to the executable
    // directory
    if (input->GetKeyPress(KEY_F5))
    {
        File saveFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Physics.xml", FILE_WRITE);
        scene_->SaveXML(saveFile);
    }
    if (input->GetKeyPress(KEY_F7))
    {
        File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Physics.xml", FILE_READ);
        scene_->LoadXML(loadFile);
    }
}

void Physics::SpawnObject(bool camera)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Create a smaller box at camera position
    Node* boxNode = scene_->CreateChild("SmallBox");
    if (camera)
    {
        boxNode->SetPosition(cameraNode_->GetPosition());
        boxNode->SetRotation(cameraNode_->GetRotation());
    }
    else
    {
        boxNode->SetPosition(Vector3::ZERO);
        boxNode->SetRotation(Quaternion::IDENTITY);
    }

    boxNode->SetScale(0.1f);
    StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
    boxObject->SetCastShadows(true);

    // Create physics components, use a smaller mass also
    RigidBody* body = boxNode->CreateComponent<RigidBody>();
    body->SetMass(0.25f);
    body->SetFriction(0.75f);
    body->SetCollisionLayer(1 << 0);
    CollisionShape* shape = boxNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    const float OBJECT_VELOCITY = 1.0f;

    // Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
    // to overcome gravity better
    if (camera)
    {
        body->SetLinearVelocity(cameraNode_->GetRotation() * Vector3(0.0f, 0.25f, 1.0f) * OBJECT_VELOCITY);
    }
    else
    {

    }

    boxNodes_.Push(boxNode);
}

void Physics::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    //Node* vehicleNode = body_->GetNode();

    //// Physics update has completed. Position camera behind vehicle
    //Quaternion dir(vehicleNode->GetRotation().YawAngle(), Vector3::UP);
    //dir = dir * Quaternion(-90.0f, Vector3::UP);
    ////dir = dir * Quaternion(vehicle_->controls_.pitch_, Vector3::RIGHT);

    //Vector3 cameraTargetPos = vehicleNode->GetPosition() - dir * Vector3(0.0f, 0.0f, CAMERA_DISTANCE);
    //Vector3 cameraStartPos = vehicleNode->GetPosition();

    //// Raycast camera against static objects (physics collision mask 2)
    //// and move it closer to the vehicle if something in between
    //Ray cameraRay(cameraStartPos, cameraTargetPos - cameraStartPos);
    //float cameraRayLength = (cameraTargetPos - cameraStartPos).Length();
    //PhysicsRaycastResult result;
    //scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, cameraRay, cameraRayLength, 2);
    //if (result.body_)
    //    cameraTargetPos = cameraStartPos + cameraRay.direction_ * (result.distance_ - 0.5f);

    //cameraNode_->SetPosition(cameraTargetPos);
    //cameraNode_->SetRotation(dir);

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void Physics::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    int key = eventData[KeyDown::P_KEY].GetInt();

    if (key == KEY_T)
    {
        updateLinSuspension_ = !updateLinSuspension_;
    }

    if (key == KEY_Y)
    {
        updateRotSuspension_ = !updateRotSuspension_;
    }

    if (key == KEY_R)
    {
        maxHeight_ = 0.0f;
        k_ = 0.0f;
        kFactor_ = 0;
        impulse_ = Vector3::ZERO;
        maxImpulse_ = 0.0f;
        timeElapsed_ = 0.0f;
        // scene_->SetTimeScale(0.1f);
    }

    if (key == KEY_M)
    {
        scene_->SetUpdateEnabled(!scene_->IsUpdateEnabled());
    }

    if (key == KEY_Z)
    {
        Quaternion rot = body_->GetRotation();
        Vector3 pos = body_->GetPosition();
        Vector3 vel = rot * Vector3::FORWARD;

        Vector3 relPos = rot * Vector3::ZERO;
        float forwardFactor_ = 2000.0f;

        body_->ApplyForce(vel * forwardFactor_, relPos);
    }

    if (key == KEY_LEFT)
    {
        SpawnObject(false);
    }

    // Toggle physics debug geometry with space
    if (key == KEY_SPACE)
        drawDebug_ = !drawDebug_;

    if (key == KEY_X)
    {
        for(unsigned i = 0; i < boxNodes_.Size(); i++)
        {
            boxNodes_.At(i)->Remove();
        }
        boxNodes_.Clear();
    }

    Sample::HandleKeyDown(eventType, eventData);
}

void Physics::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
    if (drawDebug_)
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(false);

    if (debugText_)
    {
        Vector3 com = Vector3::ZERO;
        DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();

        for(unsigned i = 0; i < wheelsNode_.Size(); i++)
        {
            Node* node = wheelsNode_.At(i);
            ConvexCast* caster = node->GetComponent<ConvexCast>();
            const WheelInfo& wheel = wheelsInfo_.At(i);
            caster->DebugDraw(wheel, com);

            debug->AddNode(node, 1.0f, false);
        }

//        int index = caster->hitIndex_;
//        if (index < 0)
//            return;

//        float posy = caster->hitDistance_.At(index) + caster->GetRadius();

//        String text;
//        char buff[128];

//        float vel = body_->GetLinearVelocity().y_;
//        sprintf(buff, "height <%.2f>\n max height <%.2f>\n vel <%.2f>\n k <%.2f>\n c <%.2f>\n error <%.2f>\n",
//            posy, maxHeight_, vel, k_, c_, floatHeight_ - posy);
//        text.AppendWithFormat("%s", buff);

//        text.AppendWithFormat("\nhits <%i> index <%i>", caster->hitPoints_, caster->hitIndex_);

//        text.AppendWithFormat("\nradius <%f> ", caster->GetRadius());
//        memset(buff, 0, 128);
//        Vector3 hp = caster->hardPointWS_;
//        sprintf(buff, "\nfrom <%.2f,%.2f,%.2f> ", hp.x_, hp.y_, hp.z_);
//        text.AppendWithFormat("%s", buff);

//        Vector3 to = hp + caster->suspensionRest_ * caster->direction_;
//        memset(buff, 0, 128);
//        sprintf(buff, "\nto <%.2f,%.2f,%.2f> ", to.x_, to.y_, to.z_);
//        text.AppendWithFormat("%s", buff);

//        // for (int i = 0; i < convexCastTest_->hitPoints_; i++)
//        int i = index;
//        {
//            Vector3 h = caster->hitPointWorld_.At(i);
//            char buff[128];
//            memset(buff, 0, 128);
//            sprintf(buff, "\nhit <%.2f,%.2f,%.2f> ", h.x_, h.y_, h.z_);
//            text.AppendWithFormat("%s", buff);

//            Vector3 l = caster->hitPointLocal_.At(i);
//            memset(buff, 0, 128);
//            sprintf(buff, "local <%.2f,%.2f    ,%.2f> ", l.x_, l.y_, l.z_);
//            text.AppendWithFormat("%s", buff);

//            memset(buff, 0, 128);
//            float d = caster->hitDistance_.At(i);
//            sprintf(buff, "f <%.2f> d <%.2f>", caster->hitFraction_.At(i), d);
//            text.AppendWithFormat("%s", buff);
//        }

//        if(caster->hitBody_ && caster->hitBody_->GetNode())
//            text.AppendWithFormat("\nbody <%s> ", caster->hitBody_->GetNode()->GetName().CString());

//        debugText_->SetText(text);
    }
}

void Physics::UpdateLineal(float timeStep)
{
    Quaternion rot = body_->GetRotation();
    Vector3 pos = body_->GetPosition();
    float posy = pos.y_ - 0.5;

    float k = k_;
    float x = floatHeight_ - posy;
    float springForce = k * x;

    float vel = GetVelocity(pos);
    float d = 1.0f;
    float c = -sqrt(mass_ * k * 4) * d;
    c_ = c;

    float antiG = mass_ * 9.81f;
    float dampingForce = c * vel;
    float suspensionForce = springForce + dampingForce + antiG;

    Vector3 direction = Vector3::UP;
    impulse_ = direction * suspensionForce; // *timeStep;

    body_->ApplyForce(impulse_);
}

void Physics::UpdateLinealCast(float timeStep, ConvexCast* caster)
{
    float mass = mass_ / wheelsNode_.Size();

    if (caster->hasHit_)
    {
        int index = caster->hitIndex_;

        Quaternion rot = body_->GetRotation();
        Vector3 pos = body_->GetPosition();

        float posy = caster->hitDistance_.At(index);
        float k = k_;
        float x = floatHeight_ - (posy); // +caster->GetRadius());
        // x = 0.0f;
        float springForce = k * x;

        float vel = GetVelocity(pos + caster->offset_);
        float d = 1.0f;
        float c = -sqrt(mass * k * 4.0f) * d;
        c_ = c;

        float antiG = mass * 9.81f;
        float dampingForce = c * vel;
        float suspensionForce = springForce + dampingForce + antiG;

        Vector3 direction = Vector3::UP;
        Vector3 normal(caster->hitNormalWorld_.At(index));
        //Vector3 direction = normal;
        impulse_ = direction * suspensionForce; // *timeStep;
    }
    else
    {
        impulse_ = Vector3::ZERO;
    }

    Vector3 relpos = caster->offset_;

    body_->ApplyForce(impulse_, relpos);

    //URHO3D_LOGERRORF("time, pos <%f, %f>", timeElapsed_, posy);
}

void Physics::UpdateRotation(float timeStep)
{
    timeElapsed_ += timeStep;

    Quaternion rot = body_->GetRotation();
    Vector3 angVel = body_->GetAngularVelocity();

    float k = 500.0f;
    float x = (45.0f - rot.EulerAngles().y_) * M_DEGTORAD;
    float springForce = k * x;
    // btMatrix3x3 inertia =  hullBody_->GetBody()->getInvInertiaTensorWorld();
    float d = 1.0f;
    float moi = mass_ / 12.0f * (width_ * width_ + length_ * length_);

    float c = -sqrt(moi * k * 4.0f) * d;
    float v = angVel.y_;
    float dampingForce = c * v;

    float steeringForce = springForce + dampingForce;

    char buff[512];
    sprintf(buff, "x <%.3f> timestep <%.3f> moi <%.2f>", (45.0f - rot.EulerAngles().y_), timeElapsed_, moi);
    // URHO3D_LOGERRORF("%s", buff);

    Vector3 torqueDir = rot * Vector3::UP;
    Vector3 impulse = torqueDir * steeringForce;// *timeStep;

    body_->ApplyTorque(impulse);
}

void Physics::UpdateWheelTransformsWS(WheelInfo& wheel)
{
    Vector3 wsPos = body_->GetPosition();
    Quaternion wsRot = body_->GetRotation();

    wheel.raycastInfo_.hardPointWS_ = wsRot * wheel.chassisConnectionPointCS_ + wsPos;
    wheel.raycastInfo_.wheelDirectionWS_ = wsRot * wheel.wheelDirectionCS_;
}

void Physics::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsPreStep;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    if (updateLinSuspension_)
    {
        Vector3 comPos = body_->GetPosition();
        float mass = (mass_ / 4.0f);

        float suspension[4];
        unsigned ll = 0;
        for (unsigned i = 0; i < wheelsInfo_.Size(); i++)
        {
            WheelInfo& wheel = wheelsInfo_[i];
            Vector3 impulse;
            float suspensionForce = 0.0;

            Vector3 relpos = wheel.raycastInfo_.contactPoint_ - comPos;
            Vector3 forceDir = wheel.raycastInfo_.contactNormal_;

            if(wheel.raycastInfo_.isInContact_)
            {
                float distance = wheel.raycastInfo_.distance_ + suspensionDelta_;
                float currentDiff = suspensionRest_ - distance + (mass * 9.81f / suspensionStiffness_);

                // float x = currentDiff > 0.0f ? currentDiff : 0.0f;
                float x = currentDiff;
                float springForce = suspensionStiffness_ * x;

                float suspensionDampStiffness = suspensionStiffness_;
                float c = -sqrt(mass * suspensionDampStiffness * 4) * suspensionDamping_;
                float v = wheel.raycastInfo_.suspensionRelativeVelocity_;
                float dampingForce = c * v;

                float maxSuspensionForce = 6000.0f;
                if (dampingForce > maxSuspensionForce)
                {
                    dampingForce = maxSuspensionForce;
                }

                if ((springForce + dampingForce) > 0.0f)
                {
                    suspensionForce = (springForce + dampingForce);

                    if (suspensionForce > maxSuspensionForce)
                    {
                        suspensionForce = maxSuspensionForce;
                    }
                }
                else
                {
                    suspensionForce = -1.0f;
                }
            }
            else
            {
                ll = 1;
                suspensionForce = -1.0f;
            }
            suspension[i] =  wheel.raycastInfo_.suspensionRelativeVelocity_;

            impulse =  forceDir * suspensionForce * timeStep;

            body_->ApplyImpulse(impulse, relpos);
        }
    }
    if (updateRotSuspension_)
    {
        UpdateRotation(timeStep);
    }
}

void Physics::HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData)
{
    for (unsigned i = 0; i < wheelsInfo_.Size(); i++)
    {
        WheelInfo& wheel = wheelsInfo_[i];
        Node* node = wheelsNode_.At(i);

        UpdateWheelTransformsWS(wheel);

        ConvexCast* cc = node->GetComponent<ConvexCast>();
        cc->Update(wheel, body_);

        node->SetRotation(node_->GetRotation());
        node->SetPosition(wheel.raycastInfo_.hardPointWS_);
    }
}

float Physics::GetVelocity(const Vector3& relPos)
{
    Quaternion rot = body_->GetRotation();
    Vector3 pos = body_->GetPosition();

    Vector3 normal = rot * Vector3::UP;
    float dd = normal.DotProduct(-Vector3::UP);
    // Vector3 relPos = body_->GetPosition();

    Vector3 contactVel = body_->GetVelocityAtPoint(relPos);
    float projVel = normal.DotProduct(contactVel);
    float vel = 0.0f;
    if (dd >= -0.1f)
    {
        vel = 0.0f;
    }
    else
    {
        float inv = btScalar(-1.) / dd;
        vel = projVel * inv;
    }

    return vel;
}
