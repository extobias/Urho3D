#pragma once

#include "../UI/UIElement.h"
#include "../Graphics/Texture2D.h"
#include "../Core/Context.h"

#include "renderers/tb_renderer_batcher.h"
#include "tb_widgets_common.h"
#include "tb_message_window.h"

using namespace tb;
using namespace Urho3D;

//namespace tb
//{
//class TBWidgetLister;
//class TBWidget;
//class TBBitmap;
//class TBRendererBatcher;
//}

namespace Urho3D
{
class TBRendererUrho3D;
class TBUIElement;

static const Urho3D::StringHash E_TBUI_BUTTON_RELEASED("TB_UI_BUTTON_RELEASED");
static const Urho3D::StringHash E_TBUI_WIDGET_CHANGED("TB_UI_WIDGET_CHANGED");

static const Urho3D::StringHash P_BUTTON_ID("button_id");
static const Urho3D::StringHash P_BUTTON_TEXT("button_text");
static const Urho3D::StringHash P_WIDGET_ID("widget_id");
static const Urho3D::StringHash P_WIDGET_VALUE("widget_value");

class TBRootWidget : public TBWidget, public TBWidgetListener, public Urho3D::Object
{
    URHO3D_OBJECT(TBRootWidget, Object)
public:
    TBRootWidget(Context* context);
    
    virtual bool OnEvent(const TBWidgetEvent &ev);
    virtual void OnWidgetFocusChanged(TBWidget *widget, bool focused);
//    virtual void OnResized(int old_w, int old_h);
};

// TB bitmap for Urho3D engine
class TBBitmapUrho3D : public TBBitmap
{
public:
    TBBitmapUrho3D(TBRendererUrho3D *renderer);
    ~TBBitmapUrho3D();

    bool Init(unsigned width, unsigned height, uint32 *data);
    virtual int Width() { return width_; }
    virtual int Height() { return height_; }
    virtual void SetData(uint32 *data);

    SharedPtr<Texture2D> GetTexture() const { return texture_; }
private:
    TBRendererUrho3D *renderer_;
    unsigned width_, height_;
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
class TBUIElement : public UIElement
{
   URHO3D_OBJECT(TBUIElement, UIElement)

public:
    TBUIElement(Context* context);
    ~TBUIElement();

    static void RegisterObject(Context* context);
    
    void AddStateWidget(TBWidget* stateWidget, bool bottom = false, bool fullscreen = true);

    void LoadWidgets(TBWidget* stateWidget, String filename);

    void LoadResources();

    void SetBoxSize(int width, int height);

    void Clear();

    virtual void GetBatches(PODVector<UIBatch>& batches,
                            PODVector<float>& vertexData,
                            const IntRect& currentScissor);

    virtual void OnClickBegin (const IntVector2& position,
                               const IntVector2& screenPosition,
                               int button, int buttons, int qualifiers,
                               Cursor* cursor);

    virtual void OnClickEnd (const IntVector2& position,
                             const IntVector2& screenPosition,
                             int button, int buttons, int qualifiers,
                             Cursor* cursor, UIElement* beginElement);
   
    // other events
    // virtual void OnWheel(int delta, int buttons, int qualifiers);
	virtual void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    // widget callback
    virtual void OnPositionSet(const IntVector2& newPosition);

    virtual void Update(float timeStep);

    virtual bool IsWithinScissor(const IntRect& currentScissor);
    
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleScreenMode(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void HandleKeyUp(StringHash eventType, VariantMap& eventData);

	void HandleRawEvent(StringHash eventType, VariantMap& args);

    TBRootWidget* GetRoot() const { return root_; }
private:
	
    void HandleFocused(StringHash /*eventType*/, VariantMap& eventData);
	void CreateKeyMap();
	int FindKeyMap(int key);

    TBRootWidget* root_;
    TBRendererUrho3D* renderer_;
	HashMap<int, int> mapKey_;
};


}
