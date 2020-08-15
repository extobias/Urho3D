#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
    class LogicComponent;
    class RigidBody2D;
    class StaticSprite2D;
    class CollisionShape2D;
    class Sprite2D;
    class Text3D;
}

using namespace Urho3D;

class Sinkhole : public LogicComponent
{
    URHO3D_OBJECT(Sinkhole, LogicComponent);

public:

    explicit Sinkhole(Context* context);

    ~Sinkhole() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void PostUpdate(float timeStep) override;

protected:

    void OnNodeSet(Node* node) override;

private:

    void HandlePhysicsBegin2D(StringHash eventType, VariantMap& eventData);

    void UpdateSprite(unsigned type);

    WeakPtr<RigidBody2D> body_;

    WeakPtr<StaticSprite2D> sprite_;

    Vector<SharedPtr<CollisionShape2D> > shapes_;

    Text3D* text3d_;

    Vector<Color> colors_;

    float sinkTimer_;

    float sinkMaxTimer_;
};
