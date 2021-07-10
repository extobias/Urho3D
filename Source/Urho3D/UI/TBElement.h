#pragma once

#include "../UI/UIElement.h"
#include "../Graphics/Texture2D.h"
#include "../Core/Context.h"

#include "renderers/tb_renderer_batcher.h"
#include "tb_widgets_common.h"
#include "tb_message_window.h"

using namespace tb;
using namespace Urho3D;

namespace Urho3D
{

class TBRendererUrho3D;
class TBUIElement;

static const StringHash E_TBUI_BUTTON_RELEASED("TB_UI_BUTTON_RELEASED");
static const StringHash E_TBUI_WIDGET_CHANGED("TB_UI_WIDGET_CHANGED");

static const StringHash E_TBUI_JOYSTICK_ADDED("TB_UI_JOYSTICK_ADDED");
static const StringHash E_TBUI_JOYSTICK_REMOVE("TB_UI_JOYSTICK_REMOVE");

static const StringHash P_BUTTON_ID("button_id");
static const StringHash P_BUTTON_TEXT("button_text");
static const StringHash P_CONTROLLER_ID("controller_id");
static const StringHash P_WIDGET_ID("widget_id");
static const StringHash P_WIDGET_VALUE("widget_value");

class NavMapping
{
public:
    void Insert(int first, int second) { mapKey_.Insert(Pair<int, int>(first, second)); }

    HashMap<int, int> mapKey_;
};

class TBRootWidget : public TBWidget, public TBWidgetListener, public Object
{
    URHO3D_OBJECT(TBRootWidget, Object)

public:

    TBRootWidget(Context* context, TBCore* core);

    virtual ~TBRootWidget();

    virtual bool OnEvent(const TBWidgetEvent &ev) override;

    virtual void OnWidgetFocusChanged(TBWidget *widget, bool focused) override;

//    virtual void OnResized(int old_w, int old_h);
};

// TB bitmap for Urho3D engine
class TBBitmapUrho3D : public TBBitmap
{
public:

    TBBitmapUrho3D(TBRendererUrho3D *renderer);

    ~TBBitmapUrho3D();

    bool Init(int width, int height, uint32 *data);

    virtual int Width() { return width_; }

    virtual int Height() { return height_; }

    virtual void SetData(uint32 *data);

    SharedPtr<Texture2D> GetTexture() const { return texture_; }

private:

    TBRendererUrho3D *renderer_;

    int width_, height_;

    SharedPtr<Texture2D> texture_;
};

// TB renderer for Urho3D engine
class TBRendererUrho3D : public TBRendererBatcher
{

public:

    TBRendererUrho3D();

    virtual ~TBRendererUrho3D();

    // == TBRenderer ==
    virtual void BeginPaint(int render_target_w, int render_target_h);

    virtual void EndPaint();

    virtual TBBitmap *CreateBitmap(int width, int height, uint32 *data);

    // TBRendererBatcher override functions
    virtual void RenderBatch(TBRendererBatcher::Batch *batch);

    virtual void SetClipRect(const TBRect &rect);

    unsigned GetBatchSize() const { return batches_.Size(); }

    UIBatch& GetBatch(unsigned i) { return batches_.At(i); }

    void Clear();

private:
    // batch vertex data
    PODVector<float> vertexData_;
    // batch size - used to access m_vertexData
    PODVector<UIBatch> batches_;
    // current clip rect
    TBRect clipRect_;
};

// TB UI Element hook to the Urho3D UI System
// tiene crear el root widget tb y tomar los datos para renderizar 
class URHO3D_API TBUIElement : public UIElement
{
   URHO3D_OBJECT(TBUIElement, UIElement)

public:

    TBUIElement(Context* context);

    virtual ~TBUIElement() override;

    static void RegisterObject(Context* context);
    
    void AddStateWidget(TBWidget* stateWidget, bool bottom = false, bool fullscreen = true);

    void LoadWidgets(TBWidget* stateWidget, const String& filename);

    void LoadWidgets(const String& filename);

    void LoadResources();

    void SetBoxSize(int width, int height);

    void Clear();

    void GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor) override;

    void OnClickBegin (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    void OnClickEnd (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, UIElement* beginElement) override;
   
    // other events
    // virtual void OnWheel(int delta, int buttons, int qualifiers);
    virtual void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    // widget callback
    void OnPositionSet(const IntVector2& newPosition) override;

    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    void Update(float timeStep) override;

    bool IsWithinScissor(const IntRect& currentScissor) override;

    const IntVector2& GetScreenPosition() const override;
    
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

    void HandleEndFrame(StringHash eventType, VariantMap& eventData);

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void HandleKeyUp(StringHash eventType, VariantMap& eventData);

    void HandleRawEvent(StringHash eventType, VariantMap& args);

    void HandleScreenMode(StringHash eventType, VariantMap& args);

    void SetNavMapping(const NavMapping& keyMap, const NavMapping& qualMap);

    bool InvokeKey(int key, unsigned special, unsigned modifier, bool down, int userdata = -1);

    TBRootWidget* GetRoot() { return root_; }

private:

    MODIFIER_KEYS GetModifierKeys();

    bool ShouldEmulateTouchEvent();

    void HandleFocused(StringHash /*eventType*/, VariantMap& eventData);

    void CreateKeyMap();

    int FindKeyMap(int key);

    int FindQualMap(int key);

    TBCore* core_;

    TBRendererUrho3D* renderer_;

    HashMap<int, int> mapKey_;

    HashMap<int, int> mapQual_;

    // SharedPtr<TBRootWidget> root_;
    TBRootWidget* root_;

    int mouse_x = 0;
    int mouse_y = 0;
    bool key_alt = false;
    bool key_ctrl = false;
    bool key_shift = false;
    bool key_super = false;
};


}
