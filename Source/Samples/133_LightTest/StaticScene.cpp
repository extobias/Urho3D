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

#include "StaticScene.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(StaticScene)

StaticScene::StaticScene(Context* context) :
    Sample(context),
	commandIndexSaoCopy_(-1),
    commandIndexSaoMain_(-1),
    aoOnly_(false)
{
}

void StaticScene::Start()
{
    // Execute base class startup
    Sample::Start();

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
	// zone->SetBoundingBox(BoundingBox(Vector3(-200.0f, -10.0f, -200.0f), Vector3(200, 10.0f, 200.0f)));
	zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
	Node* zoneChild = zoneNode->CreateChild("ZoneChild");

	// ground
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    // planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
	planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

	// light
    Node* lightNode = scene_->CreateChild("DirectionalLight");
	lightNode->SetPosition(Vector3(0.0f, 1.0f, -1.0f));
    // lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    light_ = lightNode->CreateComponent<Light>();
    light_->SetLightType(LIGHT_SPOT);
	light_->SetRadius(10.0f);
	light_->SetFov(50.0f);
	light_->SetBrightness(50.0f);
	light_->SetUsePhysicalValues(true);
	light_->SetTemperature(6500.0f);
	light_->SetCastShadows(true);
	light_->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));

	// sky
	//Node* skyNode = scene_->CreateChild("Sky");
	//skyNode->SetScale(500.0f); // The scale actually does not matter
	//Skybox* skybox = skyNode->CreateComponent<Skybox>();
	//skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	//skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	// particles
	Node* emitterNode = scene_->CreateChild("Emitter");
	emitterNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));
	auto* particleEmitter = emitterNode->CreateComponent<ParticleEmitter>();
	effect_ = cache->GetResource<ParticleEffect>("Particle/SmokeTrail.xml");
	effectMaterial_ = effect_->GetMaterial();
	effect_->SetEmitterType(EmitterType::EMITTER_SPHEREVOLUME);
	effect_->SetEmitterSize(Vector3::ONE * 0.5f);
	particleEmitter->SetEffect(effect_);
	particleEmitter->SetEmitting(true);

    const unsigned NUM_OBJECTS = 2;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode = scene_->CreateChild("Mushroom");
        // mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
		mushroomNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        StaticModel* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
		mushroomObject->SetCastShadows(true);
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(-5.0f, 5.0f, -5.0f));
	// cameraNode_->LookAt(Vector3::ZERO);
	cameraNode_->SetRotation(Quaternion(30.0f, 47.5f, 0.0f));

	// CreateDepthTexture();

	//SharedPtr<RenderSurface> surface(renderTexture->GetRenderSurface());
	//SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, camera));
	//surface->SetViewport(0, rttViewport);
	//surface->SetUpdateMode(SURFACE_UPDATEALWAYS);
}

void StaticScene::CreateDepthTexture()
{
	Graphics* graphics = GetSubsystem<Graphics>();
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	URHO3D_LOGERRORF("> > > > > > depth support <%u>", graphics->GetReadableDepthSupport());
	// depth buffer texture
	SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
	float div = 2.0f;
	renderTexture->SetSize(graphics->GetWidth() / div, graphics->GetHeight() / div, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
	// renderTexture->SetFilterMode(FILTER_BILINEAR);
	renderTexture->SetName("DepthBuffer");

	cache->AddManualResource(renderTexture);

	UI* ui = GetSubsystem<UI>();
	Sprite* textSprite = ui->GetRoot()->CreateChild<Sprite>();

	// textSprite->SetScale(256.0f / renderTexture->GetWidth());
	textSprite->SetTexture(renderTexture);
	textSprite->SetSize(renderTexture->GetWidth(), renderTexture->GetHeight());
	textSprite->SetHotSpot(renderTexture->GetWidth(), renderTexture->GetHeight());
	textSprite->SetAlignment(HA_RIGHT, VA_BOTTOM);
	textSprite->SetOpacity(1.0f);
	textSprite->SetVisible(true);
}

void StaticScene::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

	EditorWindow::RegisterObject(context_);
	ImGuiElement::RegisterObject(context_);

	EditorWindow* imgui = new EditorWindow(context_);
	imgui->SetName("editor");
	ui->GetRoot()->AddChild(imgui);
	imgui->SetScene(scene_);

	EditorGuizmo* guizmo = new EditorGuizmo(context_);
	guizmo->SetName("guizmo");
	guizmo->SetFocusMode(FM_NOTFOCUSABLE);
	ui->GetRoot()->AddChild(guizmo);
	guizmo->SetPosition(0, 0);
	guizmo->SetScene(scene_);

	imgui->BringToFront();
	imgui->SetPriority(100);
	imgui->SetGuizmo(guizmo);

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
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

	//RenderPath* effectRenderPath = viewport->GetRenderPath();
	SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    //effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredSAO.xml"));
	effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/Forward.xml"));
	// effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredRenderDepth.xml"));
	// effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/DeferredHWDepth2.xml"));

	//effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Bloom.xml"));
	//effectRenderPath->SetShaderParameter("BloomMix", Vector2(0.9f, 1.9f));

	//effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA2.xml"));

	//effectRenderPath->SetEnabled("Bloom", true);
	//effectRenderPath->SetEnabled("FXAA2", true);

	viewport->SetRenderPath(effectRenderPath);

	//for (int i = 0; i < effectRenderPath->GetNumCommands(); i++)
	//{
	//	RenderPathCommand* command = effectRenderPath->GetCommand(i);
	//	if (command->tag_ == "SAO_copy")
	//		commandIndexSaoCopy_ = i;
	//	if (command->tag_ == "SAO_main")
	//		commandIndexSaoMain_ = i;
	//}
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
 //   if (GetSubsystem<UI>()->GetFocusElement())
 //       return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float MOVE_SPEED = 1.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
	if (input->GetMouseButtonDown(MOUSEB_RIGHT))
	{
		IntVector2 mouseMove = input->GetMouseMove();
		yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
		pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);

		// Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
		cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
		// printf("camera pitch <%f> yaw <%f>", pitch_, yaw_);
	}

	if (input->GetKeyDown(KEY_SHIFT))
		MOVE_SPEED *= 10.0f;
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
	DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
	light_->DrawDebugGeometry(debugRenderer, false);
}
