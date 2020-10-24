#pragma once

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
    class LogicComponent;
    class RigidBody2D;
    class StaticSprite2D;
    class CollisionShape2D;
    class Sprite2D;
}

using namespace Urho3D;

class Ball2D : public LogicComponent
{
    URHO3D_OBJECT(Ball2D, LogicComponent)

public:

    explicit Ball2D(Context* context);

    ~Ball2D() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void SetType(unsigned type);

    void SetForce(const Vector2& force);

    float GetArea() const { return area_; }

    void SetChained(bool chain) { chained_ = chain; }

    bool IsChained() const { return chained_; }

    void SetMagnetized(bool magnetized) { magnetized_ = magnetized; }

    bool IsMagnetized() const { return magnetized_; }

    Node* next_;

protected:

    void OnNodeSet(Node* node) override;

private:

    void HandleBeginContact2D(StringHash eventType, VariantMap& eventData);

    void UpdateArea();

    WeakPtr<RigidBody2D> body_;

    WeakPtr<StaticSprite2D> sprite_;

    WeakPtr<CollisionShape2D> shape_;

    float area_;

    bool chained_;

    bool magnetized_;
};