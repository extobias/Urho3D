//
// Copyright (c) 2008-2016 the Urho3D project.
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

class Window;

}

/// A simple 'HelloWorld' GUI created purely from code.
/// This sample demonstrates:
///     - Creation of controls and building a UI hierarchy
///     - Loading UI style from XML and applying it to controls
///     - Handling of global and per-control events
/// For more advanced users (beginners can skip this section):
///     - Dragging UIElements
///     - Displaying tooltips
///     - Accessing available Events data (eventData)
class HelloTBUI : public Sample
{
    URHO3D_OBJECT(HelloTBUI, Sample);

public:
    /// Construct.
    HelloTBUI(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;
    void Stop() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual String GetScreenJoystickPatchString() const { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    void CreateScene();
    void CreateTBElement();
    void CreateModalElement();
    void CreateModalElement2();

    void SetupViewport();
    /// Handle drag begin for the fish button.
    void HandleDragBegin(StringHash eventType, VariantMap& eventData);
    /// Handle drag move for the fish button.
    void HandleDragMove(StringHash eventType, VariantMap& eventData);
    /// Handle drag end for the fish button.
    void HandleDragEnd(StringHash eventType, VariantMap& eventData);
    /// Handle any UI control being clicked.
    void HandleControlClicked(StringHash eventType, VariantMap& eventData);
    /// Handle close button pressed and released.
    void HandleClosePressed(StringHash eventType, VariantMap& eventData);
    
    void HandleTBUIReleased(StringHash eventType, VariantMap& eventData);

    void HandleTBUIChanged(StringHash eventType, VariantMap& eventData);

	void HandleKeyDown(StringHash eventType, VariantMap& eventData);

    /// The Window.
    SharedPtr<Window> window_;
    /// The UI's root UIElement.
    SharedPtr<UIElement> uiRoot_;
    /// Remembered drag begin position.
    IntVector2 dragBeginPosition_;
    
    TBUIElement* tbelement_;
    TBUIElement* modalElement_;
    TBUIElement* modalElement2_;
    TBUIElement* animTest_;
};


