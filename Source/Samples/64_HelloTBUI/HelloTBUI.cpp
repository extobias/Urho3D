//
// Copyright (c) 2008-2016 the Urho3D project.
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
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/ToolTip.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/TBElement.h>
#include <Urho3D/UI/ImGuiElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

#include "HelloTBUI.h"

#include <Urho3D/DebugNew.h>
#include <animation/tb_widget_animation.h>
#include <tb_color_picker.h>
#include <tb_progress_bar.h>

URHO3D_DEFINE_APPLICATION_MAIN(HelloTBUI)

HelloTBUI::HelloTBUI(Context* context) :
    Sample(context),
    uiRoot_(GetSubsystem<UI>()->GetRoot()),
    dragBeginPosition_(IntVector2::ZERO)
{
}

void HelloTBUI::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();

    int windowWidth = g->GetWidth();
    int windowHeight = g->GetHeight();
    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);
    uiRoot_->SetLayout(LM_VERTICAL);

    TBUIElement::RegisterObject(context_);

    if (1)
    {
        tbelement = new TBUIElement(context_);
        tbelement->LoadResources();
        tbelement->LoadWidgets("Data/UI/TB/layout/settings.txt");
        tbelement->LoadLanguage("Data/UI/TB/language/lng_en.tb.txt");
        tbelement->SetPosition(0, 0);
        // tbelement->SetSize(windowWidth / 2, windowHeight);
        GetSubsystem<UI>()->GetRoot()->AddChild(tbelement);

        SubscribeToEvent(tbelement->GetRoot(), E_TBUI_WIDGET_CHANGED, URHO3D_HANDLER(HelloTBUI, HandleTBUIChanged));

        TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        if (pb) 
        {
            pb->AddColorValue(0.5f, TBColor(0, 255, 0));
            pb->AddColorValue(0.25f, TBColor(255, 255, 0));
            pb->AddColorValue(0.15f, TBColor(255, 0, 0));
            pb->AddColorValue(0.05f, TBColor(0, 0, 255));
        }
    }
    else
    {
        Button* button = new Button(context_);
        // button->SetEnableAnchor(true);
        button->SetStyleAuto();
        button->SetName("BUTTON 1");
        Text* text = new Text(context_);
        text->SetText("|BUTTON 1|");
        button->AddChild(text);
        button->SetMinHeight(24);
        button->SetMinWidth(124);
        button->SetAlignment(HA_LEFT, VA_BOTTOM);

        uiRoot_->AddChild(button);
    }

    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(HelloTBUI, HandleKeyDown));

    CreateScene();

    SetupViewport();
    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void HelloTBUI::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(50.0f, 1.0f, 50.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetColor(Color(0.5f, 0.5f, 0.5f));
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create animated models
    const unsigned NUM_MODELS = 30;
    const float MODEL_MOVE_SPEED = 2.0f;
    const float MODEL_ROTATE_SPEED = 100.0f;
    const BoundingBox bounds(Vector3(-20.0f, 0.0f, -20.0f), Vector3(20.0f, 0.0f, 20.0f));

    for (unsigned i = 0; i < NUM_MODELS; ++i)
    {
        Node* modelNode = scene_->CreateChild("Jill");
        modelNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 0.0f, Random(40.0f) - 20.0f));
        modelNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));

        auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
        modelObject->SetModel(cache->GetResource<Model>("Models/Kachujin/Kachujin.mdl"));
        modelObject->SetMaterial(cache->GetResource<Material>("Models/Kachujin/Materials/Kachujin.xml"));
        modelObject->SetCastShadows(true);

        // Create an AnimationState for a walk animation. Its time position will need to be manually updated to advance the
        // animation, The alternative would be to use an AnimationController component which updates the animation automatically,
        // but we need to update the model's position manually in any case
        auto* walkAnimation = cache->GetResource<Animation>("Models/Kachujin/Kachujin_Walk.ani");

        AnimationState* state = modelObject->AddAnimationState(walkAnimation);
        // The state would fail to create (return null) if the animation was not found
        if (state)
        {
            // Enable full blending weight and looping
            state->SetWeight(1.0f);
            state->SetLooped(true);
            state->SetTime(Random(walkAnimation->GetLength()));
        }

        // Create our custom Mover component that will move & animate the model during each frame's update
        // auto* mover = modelNode->CreateComponent<Mover>();
        // mover->SetParameters(MODEL_MOVE_SPEED, MODEL_ROTATE_SPEED, bounds);
    }

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(-5.0f, 0.0f, 0.0f));
}

void HelloTBUI::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    int key = eventData[KeyDown::P_KEY].GetInt();

//    if (key == KEY_F1)
//        GetSubsystem<Console>()->Toggle();

    if (key == KEY_F4)
    {
#ifdef TB_RUNTIME_DEBUG_INFO
        ShowDebugInfoSettingsWindow(tbelement->GetRoot());
#else
        URHO3D_LOGERRORF("gamestate.handlekeydown: TB_RUNTIME_DEBUG_INFO not defined");
#endif
    }

    if (key == KEY_A)
    {
        // animTest_->core_->widgets_animation_manager::AbortAnimations();
        // TBWidgetsAnimationManager::AbortAnimations(animTest_->root_);
        ANIMATION_CURVE curve = ANIMATION_CURVE_BEZIER;
        double duration = 1500;

        TBContainer* cont = animTest_->GetRoot()->GetWidgetByIDAndType<TBContainer>(TBIDC("load_container"));
        TBRect rect = cont->GetRect();

        if (TBAnimationObject *anim = new TBWidgetAnimationRect(cont, rect.Offset(0, -rect.y - rect.h), rect))
        {
                TBAnimationManager::StartAnimation(anim, curve, duration);
        }

    }

    if (key == KEY_LEFT)
    {
        // TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        // float val = (float)pb->GetValueDouble();
        // val -= 0.01f;
        // pb->SetValueDouble(val);        

        TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        if (pb) 
        {
            pb->AddColorValue(0.01f, TBColor(0, 255, 255));
        }
    }

    if (key == KEY_RIGHT)
    {
        // TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        // float val = (float)pb->GetValueDouble();
        // val += 0.01f;
        // pb->SetValueDouble(val);        
    }

    Sample::HandleKeyDown(eventType, eventData);
}

void HelloTBUI::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void HelloTBUI::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    // Get UIElement relative position where input (touch or click) occurred (top-left = IntVector2(0,0))
    dragBeginPosition_ = IntVector2(eventData["ElementX"].GetInt(), eventData["ElementY"].GetInt());
}

void HelloTBUI::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetInt(), eventData["Y"].GetInt());
    UIElement* draggedElement = static_cast<UIElement*>(eventData["Element"].GetPtr());
    draggedElement->SetPosition(dragCurrentPosition - dragBeginPosition_);
}

void HelloTBUI::HandleDragEnd(StringHash eventType, VariantMap& eventData) // For reference (not used here)
{
}

void HelloTBUI::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    tbelement->Remove();
    tbelement2->Remove();

    if (GetPlatform() != "Web")
        engine_->Exit();
}

void HelloTBUI::HandleTBUIReleased(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("HelloTBUI::HandleTBUIReleased");
    tbelement->Clear();
}

void HelloTBUI::HandleTBUIChanged(StringHash eventType, VariantMap& eventData)
{
    uint32 widgetId = eventData[P_WIDGET_ID].GetUInt();
    TBColorPicker* cp = dynamic_cast<TBColorPicker*>(tbelement->GetRoot()->GetWidgetByID(widgetId));
    if (cp)
    {
        TBColor c = cp->GetColor();
        URHO3D_LOGERRORF("HelloTBUI::HandleTBUIChanged: <%p> found cp <%i, %i, %i, %i> uint <%i> val <%f>", cp, c.r, c.g, c.b, c.a, (c), cp->m_valueColor);
    }
}

void HelloTBUI::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    // Get the Text control acting as the Window's title
    Text* windowTitle = window_->GetChildStaticCast<Text>("WindowTitle", true);

    // Get control that was clicked
    UIElement* clicked = static_cast<UIElement*>(eventData[UIMouseClick::P_ELEMENT].GetPtr());

    String name = "...?";
    if (clicked)
    {
        // Get the name of the control that was clicked
        name = clicked->GetName();
    }

    // Update the Window's title text
    windowTitle->SetText("Hello " + name + "!");
}
