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
class URHO3D_API ImGuiElement : public UIElement
{
   URHO3D_OBJECT(ImGuiElement, UIElement)

public:

    ImGuiElement(Context* context);

    virtual ~ImGuiElement();

    static void RegisterObject(Context* context);
    
    virtual void GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor) override;

    virtual void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    virtual void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    virtual void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor, UIElement* beginElement) override;
   
    virtual void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    virtual void OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;

    virtual void OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags buttons, Cursor* cursor) override;

    virtual void OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
                            Cursor* cursor)  override;
    /// not used
    virtual void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    virtual void OnTextInput(const String& text) override;

    virtual void OnPositionSet(const IntVector2& newPosition) override;

    virtual void Update(float timeStep) override;

    virtual void Render(float timeStep);

    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleScreenMode(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void HandleKeyUp(StringHash eventType, VariantMap& eventData);

    bool IsWithinScissor(const IntRect& currentScissor) override;

protected:

    WeakPtr<Scene> scene_;

    ImGuiContext* imguiContext_;

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

    String inputText_;
};


}
