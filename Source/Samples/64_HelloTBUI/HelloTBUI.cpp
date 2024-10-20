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
#include <image/tb_multiple_image.h>
#include <tb_color_picker.h>
#include <tb_progress_bar.h>

URHO3D_DEFINE_APPLICATION_MAIN(HelloTBUI)

HelloTBUI::HelloTBUI(Context* context) :
    Sample(context),
    uiRoot_(GetSubsystem<UI>()->GetRoot()),
    dragBeginPosition_(IntVector2::ZERO),
    tbelement_(nullptr),
    modalElement_(nullptr),
    modalElement2_(nullptr)
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
    // uiRoot_->SetDefaultStyle(style);
    uiRoot_->SetLayout(LM_VERTICAL);

    Input* input = GetSubsystem<Input>();
    Vector2 inputScale = input->GetInputScale();

    TBUIElement::RegisterObject(context_);

    context_->SetGlobalVar(SCREEN_DPI, windowWidth / inputScale.x_);

    if (true)
    {
        CreateTBElement();
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

void HelloTBUI::Stop()
{
    tbelement_->Remove();
    Sample::Stop();
}

void HelloTBUI::CreateTBElement()
{
    if (tbelement_)
    {
        tbelement_->Remove();
        tbelement_ = nullptr;
    }

    tbelement_ = new TBUIElement(context_);
    
    tbelement_->LoadLanguage("Data/UI/TB/language/lng_en.tb.txt");
    tbelement_->LoadSkin("Data/UI/TB/skin/skin.tb.txt", "Data/UI/TB/skin/skin.sk.txt");
    tbelement_->LoadFont("Data/UI/TB/font/kenvector_future.ttf", 16);
    tbelement_->LoadWidgets("Data/UI/TB/layout/settings.txt");

    tbelement_->SetPosition(0, 0);
    tbelement_->SetVisible(true);
    GetSubsystem<UI>()->GetRoot()->AddChild(tbelement_);

    SubscribeToEvent(tbelement_->GetRoot(), E_TBUI_WIDGET_CHANGED, URHO3D_HANDLER(HelloTBUI, HandleTBUIChanged));

    TBLayout* layout = dynamic_cast<TBLayout*>(tbelement_->GetRoot()->GetWidgetByID(TBID("layout_ppal")));
    if (layout)
    {
        SizeConstraints sc;
        TBImageWidget* img1 = new TBImageWidget(layout->core_);
        img1->SetImage("Data/Textures/DeferredDecalDebug.jpeg");
        // img1->SetAlignment(TB_VA_CENTER, TB_HA_CENTER);
        PreferredSize imgPs = img1->OnCalculatePreferredSize(sc);
        img1->SetSize(imgPs.pref_w, imgPs.pref_h);
        // img1->data.SetInt(i);

        TBImageWidget* img22 = new TBImageWidget(layout->core_);
        img22->SetImage("Data/Textures/Logo.png");
        imgPs = img22->OnCalculatePreferredSize(sc);
        // img22->SetPosition(TBPoint(50, 0));
        img22->SetVerticalAlignment(TB_VA_CENTER);
        img22->SetSize(imgPs.pref_w, imgPs.pref_h);
        img22->SetTransform(1);
        // img22->data.SetInt(i);
        
        TBMultipleImage* img2 = new TBMultipleImage(layout->core_);
        img2->SetImage("Data/Textures/UrhoIcon.png");
        img2->SetAlignment(TB_VA_CENTER, TB_HA_CENTER);
        // imgPs = img2->OnCalculatePreferredSize(sc);
        img2->SetTransform(0);
        // img2->data.SetInt(i);

        TBImageWidget* img3 = new TBImageWidget(layout->core_);
        img3->SetImage("Data/Textures/Editor/HSV20.png");
        img3->SetAlignment(TB_VA_CENTER, TB_HA_CENTER);
        imgPs = img3->OnCalculatePreferredSize(sc);
        img3->SetSize(imgPs.pref_w, imgPs.pref_h);

        // img3->data.SetInt(i);
        // img3->SetGravity(WIDGET_GRAVITY_BOTTOM);

        img2->SetSize(imgPs.pref_w, imgPs.pref_h);
                    
        char colorName[512];
        sprintf(colorName, "card%d-model%d", 0, 0);

        TBWidget* widget = new TBWidget(layout->core_);
        // layout->SetGravity(WIDGET_GRAVITY_ALL);
        widget->SetID(TBID(colorName));
        widget->AddChild(img1);
        widget->AddChild(img3);
        widget->AddChild(img2);
        widget->AddChild(img22);

        layout->AddChild(widget);

        if (TBAnimationObject *anim = new TBWidgetAnimationCustom(img2))
        {
            TBAnimationManager::StartAnimation(anim, ANIMATION_DEFAULT_CURVE, ANIMATION_DEFAULT_DURATION, ANIMATION_TIME_FIRST_UPDATE, true);
        }
    }

    // TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement_->GetRoot()->GetWidgetByID(TBID("progress_bar")));
    // if (pb) 
    // {
    //     pb->AddColorValue(0.5f, TBColor(0, 255, 0));
    //     pb->AddColorValue(0.25f, TBColor(255, 255, 0));
    //     pb->AddColorValue(0.15f, TBColor(255, 0, 0));
    //     pb->AddColorValue(0.05f, TBColor(0, 0, 255));
    // }

    // TBColorPicker* cp = dynamic_cast<TBColorPicker*>(tbelement_->GetRoot()->GetWidgetByID(TBID("color_picker")));
    // if (cp)
    // {
    //     TBColor c(1.0 * 255.0, 0.0 * 255.0, 0.0 * 255.0, 1.0 * 255.0);
    //     cp->SetColor(c);
    //     cp->UpdateFromColor();
    // }

    // TBMultipleImage* img2 = new TBMultipleImage(cp->core_);
    // img2->SetImage("Data/Textures/UrhoIcon.png");
    // SizeConstraints sc;
    // PreferredSize imgPs = img2->OnCalculatePreferredSize(sc);
    // img2->SetSize(imgPs.pref_w * 2, imgPs.pref_h * 2);
    // img2->SetTransform(0);
    // cp->AddChild(img2);

    // if (TBAnimationObject *anim = new TBWidgetAnimationCustom(img2))
    // {
    //     TBAnimationManager::StartAnimation(anim, ANIMATION_DEFAULT_CURVE, ANIMATION_DEFAULT_DURATION, ANIMATION_TIME_FIRST_UPDATE, true);
    // }
}

void HelloTBUI::CreateModalElement()
{
    if (modalElement_)
    {
        modalElement_->Remove();
        modalElement_ = nullptr;
    }

    modalElement_ = new TBUIElement(context_);
    modalElement_->SetName("ModalElement");
    // modalElement_->LoadResources();
    modalElement_->LoadLanguage("Data/UI/TB/language/lng_en.tb.txt");
    modalElement_->LoadSkin("Data/UI/TB/skin/skin.tb.txt", "Data/UI/TB/skin/skin.sk.txt");
    modalElement_->LoadFont("Data/UI/TB/font/kenvector_future.ttf", 16);

    modalElement_->LoadWidgets("Data/UI/TB/layout/settings.txt");
    
    modalElement_->SetPosition(300, 200);
    modalElement_->SetSize(600, 300);

    GetSubsystem<UI>()->GetRootModalElement()->AddChild(modalElement_);
}

void HelloTBUI::CreateModalElement2()
{
    if (modalElement2_)
    {
        modalElement2_->Remove();
        modalElement2_ = nullptr;
    }

    modalElement2_ = new TBUIElement(context_);
    modalElement2_->SetName("ModalElement2");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // Material* material = cache->GetResource<Material>("Materials/BasicUI.xml");
    // modalElement2_->SetCustomMaterial(material);

    modalElement2_->LoadResources();
    modalElement2_->LoadWidgets("Data/UI/TB/layout/drawer.txt");
    modalElement2_->LoadLanguage("Data/UI/TB/language/lng_en.tb.txt");
    Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();
    modalElement2_->SetPosition((g->GetWidth() / 2) - 200, (g->GetHeight() / 2) - 200);
    modalElement2_->SetSize(400, 400);
    GetSubsystem<UI>()->GetRootModalElement()->AddChild(modalElement2_);
}

void HelloTBUI::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

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
        // ShowDebugInfoSettingsWindow(tbelement_->GetRoot());
#else
        URHO3D_LOGERRORF("gamestate.handlekeydown: TB_RUNTIME_DEBUG_INFO not defined");
#endif
    }

    if (key == KEY_A)
    {
        // animTest_->core_->widgets_animation_manager::AbortAnimations();
        // TBWidgetsAnimationManager::AbortAnimations(animTest_->root_);
        // ANIMATION_CURVE curve = ANIMATION_CURVE_BEZIER;
        // double duration = 1500;

        // TBContainer* cont = animTest_->GetRoot()->GetWidgetByIDAndType<TBContainer>(TBIDC("load_container"));
        // TBRect rect = cont->GetRect();

        // if (TBAnimationObject *anim = new TBWidgetAnimationRect(cont, rect.Offset(0, -rect.y - rect.h), rect))
        // {
        //         TBAnimationManager::StartAnimation(anim, curve, duration);
        // }
        // CreateTBElement();

        CreateModalElement();
    }

    if (key == KEY_B)
    {
        modalElement_->SetVisible(false);
        modalElement_->Clear();
    }

    if (key == KEY_C)
    {
        CreateModalElement2();
    }

    if (key == KEY_LEFT)
    {
        // TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        // float val = (float)pb->GetValueDouble();
        // val -= 0.01f;
        // pb->SetValueDouble(val);        

        TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement_->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        if (pb) 
        {
            pb->Clear();
            pb->AddColorValue(0.21f, TBColor(0, 255, 255));
            pb->AddColorValue(0.61f, TBColor(255, 0, 255));
        }
    }

    if (key == KEY_RIGHT)
    {
        // TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        // float val = (float)pb->GetValueDouble();
        // val += 0.01f;
        // pb->SetValueDouble(val);        

        TBProgressBar* pb = dynamic_cast<TBProgressBar*>(tbelement_->GetRoot()->GetWidgetByID(TBID("progress_bar")));
        if (pb) 
        {
            pb->ClearProgressBar(1);
        }
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
    tbelement_->Remove();
    if (GetPlatform() != "Web")
        engine_->Exit();
}

void HelloTBUI::HandleTBUIReleased(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("HelloTBUI::HandleTBUIReleased");
    tbelement_->Clear();
}

void HelloTBUI::HandleTBUIChanged(StringHash eventType, VariantMap& eventData)
{
    uint32 widgetId = eventData[P_WIDGET_ID].GetUInt();
    TBColorPicker* cp = dynamic_cast<TBColorPicker*>(tbelement_->GetRoot()->GetWidgetByID(widgetId));
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
