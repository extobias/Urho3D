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
#include <Urho3D/Physics/CollisionShape.h>

namespace Urho3D
{

class Node;
class Scene;

}

/// Static 3D scene example.
/// This sample demonstrates:
///     - Creating a 3D scene with static content
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard and mouse input to move a freelook camera
class StaticScene : public Sample
{
    URHO3D_OBJECT(StaticScene, Sample);

public:
    /// Construct.
    StaticScene(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);

    void MoveCameraMobile(float timeStep);

    void MoveCameraDesktop(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();

    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);

    void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    void HandleTouchMove3(StringHash eventType, VariantMap& eventData);

    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    void HandleGestureInput(StringHash eventType, VariantMap& eventData);

    void HandleMultiGesture(StringHash eventType, VariantMap& eventData);

    void PaintTexture(const IntVector2& screenPosition);

    bool SelectVertex(const IntVector2& position, PODVector<IntVector2>& faces, PODVector<Vector2>& texUV);

    void UpdateRenderPath(float timeStep);

    int commandIndexSaoCopy_;
    int commandIndexSaoMain_;
    bool aoOnly_;

    Material* effectMaterial_;

    ParticleEffect* effect_;

    Light* light_;

    CollisionShape* meshShape;

    EditorModelDebug* editorModel_;

    SharedPtr<Node> rearCameraNode_;

    float yaw_, pitch_;

    float cameraDistance_;

    Node* mushroomNode_;
};
