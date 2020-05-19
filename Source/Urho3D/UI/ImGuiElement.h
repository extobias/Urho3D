#pragma once

#include "../UI/UIElement.h"
#include "../Graphics/Texture2D.h"
#include "../Scene/Scene.h"
#include "../Core/Context.h"

using namespace Urho3D;
struct ImDrawList;
struct ImGuiContext;

namespace Urho3D
{
// TB UI Element hook to the Urho3D UI System
// tiene crear el root widget tb y tomar los datos para renderizar 
class ImGuiElement : public UIElement
{
   URHO3D_OBJECT(ImGuiElement, UIElement)

public:

    ImGuiElement(Context* context);

    ~ImGuiElement();

    static void RegisterObject(Context* context);
    
    virtual void GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor);

    virtual void OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor);

    virtual void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor);

    virtual void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement);
   
    virtual void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    virtual void OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor) override;

    virtual void OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, int dragButtons, int buttons, Cursor* cursor) override;

    virtual void OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos,
            int buttons, int qualifiers, Cursor* cursor)  override;

    virtual void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers);

    virtual void OnTextInput(const String& text);
    // uielement callback
    virtual void OnPositionSet(const IntVector2& newPosition);

    virtual void Update(float timeStep);

    virtual void Render(float timeStep);

    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleScreenMode(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void HandleKeyUp(StringHash eventType, VariantMap& eventData);

    void HandleRawEvent(StringHash eventType, VariantMap& args);

    bool IsWithinScissor(const IntRect& currentScissor);

protected:

    WeakPtr<Scene> scene_;

private:

    // void ElementCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);

    void HandleFocused(StringHash /*eventType*/, VariantMap& eventData);

    void CreateKeyMap();

    void SubscribeToEvents();

    int FindKeyMap(int key);

    HashMap<int, int> mapKey_;

    SharedPtr<Texture2D> texture_;

    IntVector2 dragBeginCursor_;

    IntVector2 dragBeginPosition_;

    ImGuiContext* imguiContext_;

    String inputText_;
};


}
