//
// Copyright (c) 2008-2017 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "Sample.h"

namespace Urho3D
{
class Node;
class Scene;
class RigidBody;
}

class ConvexCast;
struct WheelInfo;
/// Physics example.
/// This sample demonstrates:
///     - Creating both static and moving physics objects to a scene
///     - Displaying physics debug geometry
///     - Using the Skybox component for setting up an unmoving sky
///     - Saving a scene to a file and loading it to restore a previous state
class Physics : public Sample
{
    URHO3D_OBJECT(Physics, Sample);

public:
    /// Construct.
    Physics(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual String GetScreenJoystickPatchString() const { return
        "<patch>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Spawn</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"MouseButtonBinding\" />"
        "            <attribute name=\"Text\" value=\"LEFT\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Debug</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();

    void CreateDebugText();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void SubscribeToEvents();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Spawn a physics object from the camera position.
    void SpawnObject(bool camera = true);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void UpdateWheelTransformsWS(WheelInfo& wheel);

    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);

    void HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData);

    void UpdateLineal(float timeStep);

    void UpdateLinealCast(float timeStep, ConvexCast* caster);

    void UpdateRotation(float timeStep);

    float GetVelocity(const Vector3& relPos);
    /// Flag for drawing debug geometry.
    bool drawDebug_;

    SharedPtr<Node> node_;
    Vector<Node*> boxNodes_;
    Vector<ConvexCast*> convexCastTest_;
    Vector<WheelInfo> wheelsInfo_;
    Vector<SharedPtr<Node>> wheelsNode_;

    RigidBody* body_;
    float mass_;
    float length_, width_;
    float floatHeight_;
    Vector3 impulse_;
    float k_, c_;
    int kFactor_;
    float maxHeight_;
    float maxImpulse_;
    bool updateLinSuspension_;
    bool updateRotSuspension_;
    float timeElapsed_;
    Text* debugText_;

    float suspensionStiffness_;
    float suspensionLength_;
    float suspensionRest_;
    float suspensionMinRest_;
    float suspensionDelta_;
    float suspensionDamping_;
};
