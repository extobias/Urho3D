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
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/EditorWindow.h>
#include <Urho3D/UI/EditorGuizmo.h>
#include <Urho3D/UI/EditorModelDebug.h>
#include <Urho3D/Physics/RigidBody.h>

#include "StaticScene.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(StaticScene)

StaticScene::StaticScene(Context* context) :
    Sample(context),
    commandIndexSaoCopy_(-1),
    commandIndexSaoMain_(-1),
    aoOnly_(false),
    cameraDistance_(8.0f)
{
}

void StaticScene::Start()
{
    // Execute base class startup
    Sample::Start();

    EditorWindow::RegisterObject(context_);
    ImGuiElement::RegisterObject(context_);
    EditorGuizmo::RegisterObject(context_);
    EditorModelDebug::RegisterObject(context_);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void StaticScene::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    engine_->SetMaxFps(60.0f);

    // zone
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(Vector3(-200.0f, -10.0f, -200.0f), Vector3(200, 10.0f, 200.0f)));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    Node* zoneChild = zoneNode->CreateChild("ZoneChild");

    // ground
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    // planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
    planeNode->SetEnabled(true);

    // light
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetPosition(Vector3(0.0f, 1.0f, -1.0f));
    lightNode->SetRotation(Quaternion(90.0f, Vector3::RIGHT));
    // lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    light_ = lightNode->CreateComponent<Light>();
    light_->SetLightType(LIGHT_DIRECTIONAL);
    light_->SetRadius(10.0f);
    light_->SetFov(50.0f);
    light_->SetBrightness(25.0f);
    light_->SetUsePhysicalValues(true);
    light_->SetTemperature(6500.0f);
    light_->SetCastShadows(true);
    light_->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));

    // sky
//    Node* skyNode = scene_->CreateChild("Sky");
//    skyNode->SetScale(500.0f); // The scale actually does not matter
//    Skybox* skybox = skyNode->CreateComponent<Skybox>();
//    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
//    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // particles
    Node* emitterNode = scene_->CreateChild("Emitter");
    emitterNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
    auto* particleEmitter = emitterNode->CreateComponent<ParticleEmitter>();
    effect_ = cache->GetResource<ParticleEffect>("Particle/SmokeTrail.xml");
    effectMaterial_ = effect_->GetMaterial();
    effect_->SetEmitterType(EmitterType::EMITTER_SPHEREVOLUME);
    effect_->SetEmitterSize(Vector3::ONE);
    particleEmitter->SetEffect(effect_);
    particleEmitter->SetEmitting(false);

    const unsigned NUM_OBJECTS = 1;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode = scene_->CreateChild("Mushroom");
        // mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        // mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        // mushroomNode->SetScale(0.5f + Random(2.0f));
        mushroomNode->SetScale(1.0f);
        StaticModel* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
        Model* mushroomModel = cache->GetResource<Model>("Models/Mushroom.mdl");
        mushroomObject->SetModel(mushroomModel);
        mushroomObject->SetCastShadows(true);
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));

        editorModel_ = mushroomNode->CreateComponent<EditorModelDebug>();
        editorModel_->SetModel(mushroomModel);
        editorModel_->SetMaterial(cache->GetResource<Material>("Materials/plane-collision.xml"));
    }

//    Node* riderNode = scene_->CreateChild("Rider");
//    // mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
//    riderNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
//    // riderNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
//    // riderNode->SetScale(0.5f + Random(2.0f));
//    AnimatedModel* riderObject = riderNode->CreateComponent<AnimatedModel>();
//    riderObject->SetModel(cache->GetResource<Model>("Models/retro_car_B3D.mdl"));
//    riderObject->SetCastShadows(true);
//    riderObject->SetMaterial(0, cache->GetResource<Material>("Materials/Mushroom.xml"));

//    Node* meshNode = scene_->CreateChild("Mesh");
//    meshNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
//    meshNode->SetScale(0.005f);
//    StaticModel* meshObject = meshNode->CreateComponent<StaticModel>();
////    // Model* meshModel = cache->GetResource<Model>("Models/plane-collision-mod.mdl");
//    Model* meshModel = cache->GetResource<Model>("Models/Mesh.mdl");
//    meshObject->SetModel(meshModel);
//    meshObject->SetCastShadows(true);
//////     meshObject->SetMaterial(cache->GetResource<Material>("Materials/PBR/Check.xml"));

//    editorModel_ = meshNode->CreateComponent<EditorModelDebug>();
//    editorModel_->SetModel(meshModel);
//    editorModel_->SetMaterial(cache->GetResource<Material>("Materials/plane-collision.xml"));

    // RigidBody* meshBody = meshNode->CreateComponent<RigidBody>();
//    meshShape = meshNode->CreateComponent<CollisionShape>();
//    meshShape->SetTriangleMesh(meshModel);
    // riderObject->SetMaterial(0, cache->GetResource<Material>("Materials/Mushroom.xml"));

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    // camera->SetAspectRatio()

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(-5.0f, 5.0f, -5.0f));
    // cameraNode_->LookAt(Vector3::ZERO);
    cameraNode_->SetRotation(Quaternion(30.0f, 47.5f, 0.0f));
    // Set an initial position for the front camera scene node above the plane
    cameraNode_->SetPosition(Vector3(-2.0f, 2.0f, -2.0f));

    Graphics* graphics = GetSubsystem<Graphics>();
//    graphics->Maximize();

    rearCameraNode_ = scene_->CreateChild("RearCamera");
    rearCameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    rearCameraNode_->SetRotation(Quaternion(-90.0f, Vector3::LEFT));
    // rearCameraNode_->Rotate(Quaternion(180.0f, Vector3::UP));
    auto* rearCamera = rearCameraNode_->CreateComponent<Camera>();
    rearCamera->SetFarClip(300.0f);

    // Because the rear viewport is rather small, disable occlusion culling from it. Use the camera's
    // "view override flags" for this. We could also disable eg. shadows or force low material quality
    // if we wanted
    rearCamera->SetViewOverrideFlags(VO_DISABLE_OCCLUSION);

    //SharedPtr<RenderSurface> surface(renderTexture->GetRenderSurface());
    //SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, camera));
    //surface->SetViewport(0, rttViewport);
    //surface->SetUpdateMode(SURFACE_UPDATEALWAYS);
}

void StaticScene::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    EditorWindow* imgui = new EditorWindow(context_);
    imgui->SetName("editor");
    ui->GetRootModalElement()->AddChild(imgui);
    imgui->SetScene(scene_);

    imgui->BringToFront();
    imgui->SetPriority(100);
    imgui->CreateGuizmo();
    imgui->SetVisible(true);
    // imgui->SetGuizmo(guizmo);

    //// Construct new Text object, set string to display and font to use
    //Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    //instructionText->SetText("Use WASD keys and mouse/touch to move");
    //instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    //// Position the text relative to the screen center
    //instructionText->SetHorizontalAlignment(HA_CENTER);
    //instructionText->SetVerticalAlignment(VA_CENTER);
    //instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void StaticScene::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    renderer->SetNumExtraInstancingBufferElements(1);
    renderer->SetMinInstances(1);
    renderer->SetNumViewports(2);

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    //RenderPath* effectRenderPath = viewport->GetRenderPath();

    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredHWDepth.xml"));
//     effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/Forward.xml"));

//    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Bloom.xml"));
    //effectRenderPath->SetShaderParameter("BloomMix", Vector2(0.9f, 1.9f));

    //effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA2.xml"));

    //effectRenderPath->SetEnabled("Bloom", true);
    //effectRenderPath->SetEnabled("FXAA2", true);

    viewport->SetRenderPath(effectRenderPath);

    //for (int i = 0; i < effectRenderPath->GetNumCommands(); i++)
    //{
    //    RenderPathCommand* command = effectRenderPath->GetCommand(i);
    //    if (command->tag_ == "SAO_copy")
    //        commandIndexSaoCopy_ = i;
    //    if (command->tag_ == "SAO_main")
    //        commandIndexSaoMain_ = i;
    //}

//    Graphics* graphics = GetSubsystem<Graphics>();
//    SharedPtr<Viewport> rearViewport(new Viewport(context_, scene_, rearCameraNode_->GetComponent<Camera>(),
//        IntRect(graphics->GetWidth() * 2 / 3, 32, graphics->GetWidth() - 32, graphics->GetHeight() / 3)));
//    renderer->SetViewport(1, rearViewport);
}

void StaticScene::UpdateRenderPath(float timeStep)
{
    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* viewport = renderer->GetViewport(0);
    RenderPath* rp = viewport->GetRenderPath();

    rp->SetEnabled("SAO_copy", true);

    if(commandIndexSaoCopy_ != -1)
    {
        RenderPathCommand command = rp->commands_[commandIndexSaoCopy_];
        if(aoOnly_)
            command.pixelShaderDefines_ = "AO_ONLY";
        else
            command.pixelShaderDefines_ = "";
        rp->RemoveCommand(commandIndexSaoCopy_);
        rp->InsertCommand(commandIndexSaoCopy_, command);
    }
}

void StaticScene::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    UIElement* focusElement = GetSubsystem<UI>()->GetFocusElement();
    if (focusElement)
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float MOVE_SPEED = 1.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }
    else
    {
//        Quaternion camRot = cameraNode_->GetRotation();
//        pitch_ = camRot.PitchAngle();
//        yaw_ = camRot.YawAngle();

//        cameraDistance_ += input->GetMouseMoveWheel();

//        Vector3 lookAt = cameraNode_->GetDirection(); // pos - rot * Vector3(0.0f, 0.0f, -3.0f);
//        Quaternion dir(yaw_, Vector3::UP);
//        dir = dir * Quaternion(pitch_, Vector3::RIGHT);

//        Vector3 cameraTargetPos = lookAt - dir * Vector3(0.0f, 0.0f, cameraDistance_);

//        cameraNode_->SetPosition(cameraTargetPos);
    }

    if (input->GetKeyDown(KEY_SHIFT))
        MOVE_SPEED *= 50.0f;
    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
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
}

void StaticScene::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(StaticScene, HandleUpdate));

    // SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(StaticScene, HandleRenderUpdate));

    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(StaticScene, HandleKeyDown));

    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(StaticScene, HandlePostRenderUpdate));
}

void StaticScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
    editorModel_->DrawDebugGeometry(debugRenderer, true);

    // UpdateRenderPath(timeStep);
}

void StaticScene::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    Camera* camera = cameraNode_->GetComponent<Camera>();

    Renderer* renderer = GetSubsystem<Renderer>();
    Viewport* viewport = renderer->GetViewport(0);
    RenderPath* rp = viewport->GetRenderPath();
    Graphics* graphics = GetSubsystem<Graphics>();

    Matrix4 p = camera->GetProjection();
    Vector4 projInfo;
    bool opengl = true;
    if(opengl)
    {
        projInfo = Vector4( 2.0f / p.m00_, 2.0f / p.m11_,
                            -(1.0f + p.m02_) / p.m00_,
                            -(1.0f + p.m12_) / p.m11_ );
    }
    else
    {
        projInfo = Vector4(2.0f / p.m00_, -2.0f / p.m11_,
            -(1.0f + p.m02_ + 1.0f / graphics->GetWidth()) / p.m00_,
            (1.0f - p.m12_ + 1.0f / graphics->GetHeight()) / p.m11_);
    }

    rp->SetShaderParameter("ProjInfo", Variant(projInfo));

    float viewSize = camera->GetHalfViewSize();
    rp->SetShaderParameter("ProjScale", Variant(graphics->GetHeight() / viewSize));

    Matrix4 v = camera->GetView().ToMatrix4();
    rp->SetShaderParameter("View", Variant(v));
}

void StaticScene::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    int key = eventData[P_KEY].GetInt();

    if (key == KEY_O)
    {
        aoOnly_ = !aoOnly_;
    }

    if (key == KEY_P)
    {
        scene_->SetUpdateEnabled(!scene_->IsUpdateEnabled());
    }

    if (key == KEY_DOWN)
    {
        Color col = effectMaterial_->GetShaderParameter("MatSpecColor").GetColor();
        URHO3D_LOGERRORF("col down <%f, %f, %f>", col.r_, col.g_, col.b_);
        col.r_ -= 0.1;
        col.g_ -= 0.1;
        col.b_ -= 0.1;
        effectMaterial_->SetShaderParameter("MatSpecColor", col);
        URHO3D_LOGERRORF("col down <%f, %f, %f>", col.r_, col.g_, col.b_);
    }

    if (key == KEY_UP)
    {
        Color col = effectMaterial_->GetShaderParameter("MatSpecColor").GetColor();
        col.r_ += 1.0f;
        // col.g_ += 0.1;
        // col.b_ += 0.1;
        effectMaterial_->SetShaderParameter("MatSpecColor", col);
        // effectMaterial_->
        URHO3D_LOGERRORF("col up <%f, %f, %f>", col.r_, col.g_, col.b_);
    }

    if (key == KEY_F)
    {
        float maxRot = effect_->GetMaxRotation();
        if (maxRot != 0)
        {
            effect_->SetMaxRotation(0.0f);
            effect_->SetMaxRotationSpeed(0.0f);
        }
        else
        {
            effect_->SetMaxRotation(360.0f);
            effect_->SetMaxRotationSpeed(50.0f);
        }
    }

    Sample::HandleKeyDown(eventType, eventData);
}

void StaticScene::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
//    DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
//    light_->DrawDebugGeometry(debugRenderer, false);
}
