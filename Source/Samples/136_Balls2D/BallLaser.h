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

class BallLaser : public LogicComponent
{
    URHO3D_OBJECT(BallLaser, LogicComponent);

public:

    explicit BallLaser(Context* context);

    ~BallLaser() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void DebugDraw();

    void SetScaleFactor(float scale);

    void SetBalls(Vector<WeakPtr<class Ball2D> >& balls);

    float GetMagnetRadius() const { return magnetRadius_; }

protected:

    void OnNodeSet(Node* node) override;

private:

    WeakPtr<RigidBody2D> body_;

    WeakPtr<StaticSprite2D> sprite_;

    WeakPtr<CollisionShape2D> shape_;

    Vector<WeakPtr<Ball2D> >* balls_;

    float scaleFactor_;

    float magnetRadius_;

    float magnetForce_;


};
