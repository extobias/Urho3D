#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/ConstraintMouse2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
//#include <Urho3D/Urho2D/StaticSprite2D.h>

#include "InputManager.h"
#include "BallDefs.h"
#include "Ball2D.h"

InputManager::InputManager(Context* context)
    : Component (context)
{
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InputManager, HandleMouseButtonDown));

    if (GetPlatform() == "Android" || GetPlatform() == "iOS")
    {
        SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(InputManager, HandleTouchBegin3));
    }
}

InputManager::~InputManager() = default;

void InputManager::RegisterObject(Context* context)
{
    context->RegisterFactory<InputManager>();
}

Vector3 InputManager::GetMousePositionXY()
{
    Input* input = GetSubsystem<Input>();
    Graphics* graphics = GetSubsystem<Graphics>();

    Camera* camera = gCameraNode->GetComponent<Camera>();

    Vector3 screenPoint = Vector3((float)input->GetMousePosition().x_ / graphics->GetWidth(), (float)input->GetMousePosition().y_ / graphics->GetHeight(), 0.0f);
    Vector3 worldPoint = camera->ScreenToWorldPoint(screenPoint);
//     URHO3D_LOGERRORF("worldpoint <%f, %f, %f>", worldPoint.x_, worldPoint.y_, worldPoint.z_);
    return worldPoint;
}

void InputManager::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    HandleButtonDown(input->GetMousePosition(), GetMousePositionXY());

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(InputManager, HandleMouseMove));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(InputManager, HandleMouseButtonUp));
}

void InputManager::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    HandleButtonUp();

    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONUP);
}

void InputManager::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    HandleMove(GetMousePositionXY());
}

void InputManager::HandleTouchBegin3(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGERRORF("Balls2DPhysics::HandleTouchBegin3");

    Graphics* graphics = GetSubsystem<Graphics>();
    using namespace TouchBegin;
    Camera* camera = gCameraNode->GetComponent<Camera>();

    Vector3 worldPos = camera->ScreenToWorldPoint(Vector3((float)eventData[P_X].GetInt() / graphics->GetWidth(), (float)eventData[P_Y].GetInt() / graphics->GetHeight(), 0.0f));
    IntVector2 screenPos(eventData[P_X].GetInt(), eventData[P_Y].GetInt());

    HandleButtonDown(screenPos, worldPos);

    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(InputManager, HandleTouchMove3));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(InputManager, HandleTouchEnd3));
}

void InputManager::HandleTouchMove3(StringHash eventType, VariantMap& eventData)
{
    Graphics* graphics = GetSubsystem<Graphics>();
    using namespace TouchMove;
    Camera* camera = gCameraNode->GetComponent<Camera>();
    Vector3 pos = camera->ScreenToWorldPoint(Vector3(float(eventData[P_X].GetInt()) / graphics->GetWidth(), float(eventData[P_Y].GetInt()) / graphics->GetHeight(), 0.0f));

    HandleMove(pos);
}

void InputManager::HandleTouchEnd3(StringHash eventType, VariantMap& eventData)
{
    HandleButtonUp();

    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
}

void InputManager::HandleMove(const Vector3& pos)
{
    if (gPickedNode)
    {
        ConstraintMouse2D* constraintMouse = gPickedNode->GetComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(Vector2(pos.x_, pos.y_));
    }
}

void InputManager::HandleButtonDown(const IntVector2& screenPos, const Vector3& worldPos)
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    PhysicsWorld2D* physicsWorld = scene->GetComponent<PhysicsWorld2D>();
    RigidBody2D* rigidBody = physicsWorld->GetRigidBody(screenPos.x_, screenPos.y_); // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        // evita seleccionar las paredes
        String name = rigidBody->GetNode()->GetName();
        if (name != "Ball")
            return;

        gPickedNode = rigidBody->GetNode();
        Ball2D *ball = gPickedNode->GetComponent<Ball2D>();
        ball->SetMagnetized(true);
        // staticSprite->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f)); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        ConstraintMouse2D* constraintMouse = gPickedNode->CreateComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(Vector2(worldPos.x_, worldPos.y_));
        constraintMouse->SetMaxForce(100000 * rigidBody->GetMass());
//        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(gDummyBody);
    }
}

void InputManager::HandleButtonUp()
{
    if (gPickedNode)
    {
        gPickedNode->RemoveComponent<ConstraintMouse2D>();
        gPickedNode->RemoveComponent<ConstraintDistance2D>();

        Ball2D* ball = gPickedNode->GetComponent<Ball2D>();
        ball->SetChained(false);
        while (ball->next_)
        {
            Node* node = ball->next_;
            node->RemoveComponent<ConstraintDistance2D>();

            ball->next_ = nullptr;
            ball = node->GetComponent<Ball2D>();
            ball->SetChained(false);
        }
        gPickedNode = nullptr;
        gTailNode = nullptr;
    }
}

