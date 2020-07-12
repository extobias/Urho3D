#pragma once

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
    class LogicComponent;
    class RigidBody2D;
    class StaticSprite2D;
    class CollisionCircle2D;
    class Sprite2D;
}

using namespace Urho3D;

class Ball2D : public LogicComponent
{
    URHO3D_OBJECT(Ball2D, LogicComponent);

public:

    explicit Ball2D(Context* context);

    ~Ball2D() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void SetColor(const Color& color);

    const Color& GetColor() const { return color_; }

    void SetScaleFactor(float scaleFactor);

    void SetSprite(Sprite2D* sprite);

    float GetArea() const { return area_; }

    void SetChained(bool chain) { chained_ = chain; }

    bool IsChained() const { return chained_; }

    void SetMagnetized(bool magnetized) { magnetized_ = magnetized; }

    bool IsMagnetized() const { return magnetized_; }

    Node* next_;

protected:

    void OnNodeSet(Node* node) override;

private:

    void UpdateArea();

    WeakPtr<RigidBody2D> body_;

    WeakPtr<StaticSprite2D> sprite_;

    WeakPtr<CollisionCircle2D> shape_;

    Color color_;

    float scaleFactor_;

    float area_;

    bool chained_;

    bool magnetized_;
};
