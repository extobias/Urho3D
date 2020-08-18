#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Scene.h>

#include "Pumper.h"
#include "Ball2D.h"
#include "BallDefs.h"

namespace Urho3D
{
extern const char* URHO2D_CATEGORY;
}

Pumper::Pumper(Context* context)
    : LogicComponent (context),
      ballTimer_(0.0f),
      pumpTimerMax_(5.0f)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

Pumper::~Pumper() = default;

void Pumper::RegisterObject(Context* context)
{
    context->RegisterFactory<Pumper>(URHO2D_CATEGORY);

    URHO3D_ATTRIBUTE("Pump Timer", float, pumpTimerMax_, 5.0f, AM_FILE);
}

void Pumper::FixedUpdate(float timeStep)
{
    ballTimer_ += timeStep;
    if (ballTimer_ > pumpTimerMax_ && GetScene()->IsUpdateEnabled())
    {
        ballTimer_ = 0.0f;
        CreateBall();
    }
}

void Pumper::CreateBall()
{
    Node* ballsNode = GetScene()->GetChild("Balls");

    Node* node  = ballsNode->CreateChild("Ball");
    node->SetTemporary(true);
    node->SetVar("Type", "Ball");

    unsigned type = (unsigned)Random(0, 3);
    node->SetVar("Color", type);

    Ball2D* ball = node->CreateComponent<Ball2D>();
    ball->SetTemporary(true);
    ball->SetType(type);


    //    float areaTotal = 0.0f;
    //    for (int i = 0; i < balls_.Size(); i++)
    //    {
    //        Ball2D* b = balls_.At(i);
    //        if (b)
    //        {
    //            areaTotal += b->GetArea();
    //        }
    //        else
    //        {
    //            URHO3D_LOGERRORF("node not found <%p>", b);
    //        }
    //    }


    //    if (areaTotal > 400.0f)
    //        scene_->SetUpdateEnabled(false);
}
