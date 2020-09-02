#pragma once

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
    class LogicComponent;
    class RigidBody2D;
    class StaticSprite2D;
    class CollisionShape2D;
}

using namespace Urho3D;

class BallRacket : public LogicComponent
{
    URHO3D_OBJECT(BallRacket, LogicComponent);

public:

    explicit BallRacket(Context* context);

    ~BallRacket() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void DelayedStart() override;

    void DebugDraw();

    void SetScaleFactor(float scale);

protected:

    void OnNodeSet(Node* node) override;

private:

    WeakPtr<RigidBody2D> body_;

    WeakPtr<StaticSprite2D> sprite_;

    WeakPtr<CollisionShape2D> shape_;

    float rotationVelocity_;

    bool clockwise_;
};
