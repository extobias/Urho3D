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
      pumpTimerMax_(0.5f),
      pumpCount_(0),
      pumpCountMax_(30),
      nodeBalls_(nullptr)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

Pumper::~Pumper() = default;

void Pumper::RegisterObject(Context* context)
{
    context->RegisterFactory<Pumper>(URHO2D_CATEGORY);

    URHO3D_ATTRIBUTE("Pump Timer", float, pumpTimerMax_, 0.5f, AM_FILE);

    URHO3D_ATTRIBUTE("Max Init Force", Vector2, maxInitForce_, Vector2(-5.0f, 5.0f), AM_FILE);

    URHO3D_ATTRIBUTE("Min Init Force", Vector2, minInitForce_, Vector2(-5.0f, 5.0f), AM_FILE);
}

void Pumper::FixedUpdate(float timeStep)
{
    ballTimer_ += timeStep;
    if (ballTimer_ > pumpTimerMax_ && GetScene()->IsUpdateEnabled() && pumpCount_ < pumpCountMax_)
    {
        ballTimer_ = 0.0f;
        pumpCount_++;

        CreateBall();
    }
}

void Pumper::CreateBall()
{
    Node* node  = nodeBalls_->CreateChild("Ball");
    node->SetWorldPosition(node_->GetWorldPosition());
    node->SetTemporary(true);
    node->SetVar("Type", "Ball");

    // unsigned type = (unsigned)Random(0, 3);
    unsigned type = 2;
    node->SetVar("Color", type);

    Ball2D* ball = node->CreateComponent<Ball2D>();
    ball->SetTemporary(true);
    ball->SetType(type);

    Vector2 force(Random(minInitForce_.x_, minInitForce_.y_), Random(maxInitForce_.x_, maxInitForce_.y_));
    ball->SetForce(force);

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

void Pumper::OnNodeSet(Node* node)
{
    if (!node)
        return;

    ResetToDefault();

    nodeBalls_ = GetScene()->CreateChild("Balls");
    nodeBalls_->SetTemporary(true);
//    nodeBalls_->SetPosition(node->GetPosition() * gScreenRatio);
}
