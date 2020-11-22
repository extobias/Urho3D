#pragma once

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{
    class LogicComponent;
}

using namespace Urho3D;

class Pumper : public LogicComponent
{
    URHO3D_OBJECT(Pumper, LogicComponent);

public:

    explicit Pumper(Context* context);

    ~Pumper() override;

    static void RegisterObject(Context* context);

    void FixedUpdate(float timeStep) override;

    void CreateBall();

protected:

    void OnNodeSet(Node* node) override;

private:

    float ballTimer_;

    float pumpTimerMax_;

    unsigned pumpCount_;

    unsigned pumpCountMax_;

    Vector2 maxInitForce_;

    Vector2 minInitForce_;

    Node* nodeBalls_;
};
