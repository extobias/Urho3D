#pragma once

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;

class InputManager : public Component
{
    URHO3D_OBJECT(InputManager, Component);

public:
    explicit InputManager(Context* context);

    ~InputManager() override;

    static void RegisterObject(Context* context);

    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);

    void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData);

    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    void HandleTouchBegin3(StringHash eventType, VariantMap& eventData);

    void HandleTouchMove3(StringHash eventType, VariantMap& eventData);

    void HandleTouchEnd3(StringHash eventType, VariantMap& eventData);

    void HandleMove(const Vector3& pos);

    void HandleButtonDown(const IntVector2& screenPos, const Vector3& worldPos);

    void HandleButtonUp();

    Vector3 GetMousePositionXY();
};
