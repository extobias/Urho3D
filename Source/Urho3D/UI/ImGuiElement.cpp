#include "../UI/ImGuiElement.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Texture2D.h"
#include "../IO/Log.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Core/CoreEvents.h"
#include "../Core/ProcessUtils.h"
#include "../Scene/Component.h"
#include "../Graphics/Camera.h"
#include "../Input/Input.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"

#include <SDL/SDL.h>

#include "imgui.h"

#define VERTEX_SIZE 6

static const int KEY_QUAL_OFFSET = 0x8000;
/*
 Cosas a resolver
 - el Context global no es una buena idea.
 - crear tantas texturas tampoco es buena idea.
 - el teclado tiene que andar en android
        - la solucion es medio chota
 */


static Context *g_context = 0;
static String g_programDir;

namespace Urho3D
{

extern const char* UI_CATEGORY;

/*------------ TBElement ------------*/
ImGuiElement::ImGuiElement(Context* context)
    : UIElement(context),
    imguiContext_(nullptr),
    scene_(nullptr)
{
    SetName("ImGuiElement");

    SetEnabled(true);
    SetFocusMode(FM_FOCUSABLE);

    IMGUI_CHECKVERSION();
    imguiContext_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // ImGui::StyleColorsClassic();
    ImGui::StyleColorsDark();
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10.0f);
    ImGuiStyle& style = ImGui::GetStyle();
    // style.WindowBorderSize = 1.0f;
    // style.Alpha = 1.0f;
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;

    // Create texture
    texture_ = new Texture2D(context_);
    texture_->SetMipsToSkip(QUALITY_LOW, 0);
    texture_->SetNumLevels(1);

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    URHO3D_LOGINFOF("get data <%u, %u>", width, height);

    if (!texture_->SetSize(width, height, Graphics::GetRGBAFormat()))
        URHO3D_LOGINFOF("tex <%p> error setsize!", texture_);

    if (!texture_->SetData(0, 0, 0, width, height, pixels))
        URHO3D_LOGINFOF("tex <%p> error setdata!", texture_);

    texture_->SetAddressMode(COORD_U, ADDRESS_WRAP);
    texture_->SetAddressMode(COORD_V, ADDRESS_WRAP);

    CreateKeyMap();

    SubscribeToEvents();

    //Graphics* g = GetSubsystem<Graphics>();
    //SetPosition(0, 0);
    //SetSize(g->GetWidth(), g->GetHeight());
}

ImGuiElement::~ImGuiElement()
{
}

void ImGuiElement::SubscribeToEvents()
{
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(ImGuiElement, HandleScreenMode));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(ImGuiElement, HandlePostUpdate));
    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(ImGuiElement, HandleBeginFrame));

    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(ImGuiElement, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(ImGuiElement, HandleKeyUp));

    SubscribeToEvent(E_SDLRAWINPUT, URHO3D_HANDLER(ImGuiElement, HandleRawEvent));

    //    SubscribeToEvent(this, E_FOCUSED, URHO3D_HANDLER(ImGuiElement, HandleFocused));
    //    SubscribeToEvent(this, E_DEFOCUSED, URHO3D_HANDLER(LineEdit, HandleDefocused));
}

void ImGuiElement::RegisterObject(Context* context)
{
    g_context = context;

    context->RegisterFactory<ImGuiElement>(UI_CATEGORY);
}

void ImGuiElement::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    ImGui::SetCurrentContext(imguiContext_);
    ImDrawData* draw_data = ImGui::GetDrawData();
    //if (draw_data)
    //{
    //    URHO3D_LOGERRORF("draw pos <%f, %f> size <%f, %f>", draw_data->DisplayPos.x, draw_data->DisplayPos.y,
    //        draw_data->DisplaySize.x, draw_data->DisplaySize.y);
    //}

    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            ImVec4 clip_rect;
            clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
            clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
            clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
            clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

            if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            {
                unsigned offset1 = 0;
                IntRect scissor(clip_rect.x - offset1, clip_rect.y - offset1, clip_rect.z + offset1, clip_rect.w + offset1);
                // IntRect scissor((int)clip_rect.x, (int)clip_rect.y, (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
                // IntRect scissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

                UIBatch batch(this, BLEND_ALPHA, scissor, texture_, &vertexData);

                unsigned vertexSize = VERTEX_SIZE * pcmd->ElemCount;
                if (!vertexSize)
                    continue;
                batch.vertexStart_ = vertexData.Size();
                batch.vertexEnd_ = batch.vertexStart_ + vertexSize;
                vertexData.Resize(batch.vertexStart_ + vertexSize);
                float* dest = &(vertexData.At(batch.vertexStart_));

                unsigned offset = 0;
                for (int i = 0; i < pcmd->ElemCount; i++, offset += VERTEX_SIZE)
                {
                    const ImDrawVert* v = &cmd_list->VtxBuffer[cmd_list->IdxBuffer[pcmd->IdxOffset + i]];
                    dest[offset + 0] = v->pos.x;
                    dest[offset + 1] = v->pos.y;
                    dest[offset + 2] = 0.0f;
                    ((unsigned&)dest[offset + 3]) = (unsigned)v->col;
                    dest[offset + 4] = v->uv.x;
                    dest[offset + 5] = v->uv.y;
                }

                UIBatch::AddOrMerge(batch, batches);
            }
         }
    }
}

void ImGuiElement::CreateKeyMap()
{
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = KEY_C;
    io.KeyMap[ImGuiKey_V] = KEY_V;
    io.KeyMap[ImGuiKey_X] = KEY_X;
    io.KeyMap[ImGuiKey_Y] = KEY_Y;
    io.KeyMap[ImGuiKey_Z] = KEY_Z;
}

int ImGuiElement::FindKeyMap(int key)
{
    return -1;
}

bool ImGuiElement::IsWithinScissor(const IntRect& currentScissor)
{
    return true;
}


void ImGuiElement::OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor)
{
    dragBeginCursor_ = screenPosition;
    dragBeginPosition_ = position;
}

void ImGuiElement::OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, int dragButtons, int buttons, Cursor* cursor)
{

}

void ImGuiElement::OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos,
    int buttons, int qualifiers, Cursor* cursor)
{
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(screenPosition.x_, screenPosition.y_);

    IntVector2 delta = screenPosition - dragBeginCursor_;
    SetPosition(dragBeginPosition_ + delta);
}

void ImGuiElement::OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers)
{

}

void ImGuiElement::OnTextInput(const String& text)
{
    inputText_ = text;
}

void ImGuiElement::OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor)
{
    UIElement::OnHover(position, screenPosition, buttons, qualifiers, cursor);

    ImGui::SetCurrentContext(imguiContext_);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(screenPosition.x_, screenPosition.y_);

    // URHO3D_LOGERRORF("hover <%s> position <%i, %i> screen <%i, %i>", name_.CString(), position.x_, position.y_, screenPosition.x_, screenPosition.y_);
}

void ImGuiElement::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition,
                                int button, int buttons, int qualifiers, Cursor* cursor)
{
    ImGui::SetCurrentContext(imguiContext_);
    // 0=left, 1=right, 2=middle
    ImGuiIO& io = ImGui::GetIO();
    if(button & MOUSEB_LEFT)
        io.MouseDown[0] = true;
    else if (button & MOUSEB_RIGHT)
        io.MouseDown[1] = true;
    else if (button & MOUSEB_MIDDLE)
        io.MouseDown[2] = true;
}

void ImGuiElement::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition,
                              int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement)
{
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    // 0=left, 1=right, 2=middle
    if (button & MOUSEB_LEFT)
        io.MouseDown[0] = false;
    else if (button & MOUSEB_RIGHT)
        io.MouseDown[1] = false;
    else if (button & MOUSEB_MIDDLE)
        io.MouseDown[2] = false;
}

void ImGuiElement::OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers)
{
    ImGui::SetCurrentContext(imguiContext_);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = delta > 0 ? 1 : (delta < 0 ? -1 : 0);
}

void ImGuiElement::OnPositionSet(const IntVector2& newPosition)
{
    // URHO3D_LOGERRORF("position <%i, %i>", newPosition.x_, newPosition.y_);
}

void ImGuiElement::Update(float timeStep)
{
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    int w, h;
    int display_w, display_h;
    Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();
    w = g->GetWidth();
    h = g->GetHeight();
    //glfwGetWindowSize(g_Window, &w, &h);
    //glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    // if (w > 0 && h > 0)
    //    io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    ImGui::NewFrame();

    // static bool show_demo_window = true;
    // ImGui::ShowDemoWindow(&show_demo_window);

    if (!inputText_.Empty())
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharactersUTF8(inputText_.CString());

        inputText_ = String::EMPTY;
    }

    Render(timeStep);

    ImGui::EndFrame();
}

static void ElementCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    URHO3D_LOGERRORF("element callback");
}

void ImGuiElement::Render(float timeStep)
{

}

void ImGuiElement::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // Rendering
    ImGui::SetCurrentContext(imguiContext_);

    ImGui::Render();
}

void ImGuiElement::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
//    int w, h;
//    int display_w, display_h;
//    Urho3D::Graphics* g = GetSubsystem<Urho3D::Graphics>();
//    w = g->GetWidth();
//    h = g->GetHeight();
//    //glfwGetWindowSize(g_Window, &w, &h);
//    //glfwGetFramebufferSize(g_Window, &display_w, &display_h);
//    io.DisplaySize = ImVec2((float)w, (float)h);
    // if (w > 0 && h > 0)
    //    io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
    using namespace BeginFrame;
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    io.DeltaTime = timeStep < M_EPSILON ? 1.0f / 60.0f : timeStep;
}

void ImGuiElement::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    //using namespace ScreenMode;
    //int w = eventData[P_WIDTH].GetInt();
    //int h = eventData[P_HEIGHT].GetInt();

    //SetSize(w, h);
}

void ImGuiElement::HandleFocused(StringHash /*eventType*/, VariantMap& eventData)
{
}

void ImGuiElement::HandleKeyDown(StringHash eventType, VariantMap &eventData)
{
    using namespace KeyDown;
    Key key = (Key)eventData[P_KEY].GetUInt();
    unsigned scancode = eventData[P_SCANCODE].GetUInt();
    QualifierFlags qualifiers = QualifierFlags(eventData[P_QUALIFIERS].GetUInt());

    // URHO3D_LOGERRORF("onkey key <%i> scancode <%i> qualifiers <%i>", key, scancode, qualifiers);

    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();

    io.KeyAlt = qualifiers & QUAL_ALT;
    io.KeyCtrl = qualifiers & QUAL_CTRL;
    io.KeyShift = qualifiers & QUAL_SHIFT;

    io.KeysDown[scancode] = true;
}

void ImGuiElement::HandleKeyUp(StringHash eventType, VariantMap &eventData)
{
    using namespace KeyDown;
    Key key = (Key)eventData[P_KEY].GetUInt();
    unsigned scancode = eventData[P_SCANCODE].GetUInt();
    QualifierFlags qualifiers = QualifierFlags(eventData[P_QUALIFIERS].GetUInt());

    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();

    io.KeyAlt = qualifiers & QUAL_ALT;
    io.KeyCtrl = qualifiers & QUAL_CTRL;
    io.KeyShift = qualifiers & QUAL_SHIFT;

    io.KeysDown[scancode] = false;
}

void ImGuiElement::HandleRawEvent(StringHash eventType, VariantMap& args)
{
    auto event = static_cast<SDL_Event*>(args[SDLRawInput::P_SDLEVENT].Get<void*>());

    //if (event->type == SDL_MOUSEMOTION)
    //{
    //    ImGuiIO& io = ImGui::GetIO();
    //    io.MousePos = ImVec2(event->motion.x, event->motion.y);

    //    if (ImGuizmo::IsOver())
    //    {
    //        URHO3D_LOGERRORF("guizmo hover!!!!!!!");
    //    }
    //}
    //else if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP /*|| */)
    //{
    //
    //}
    //else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP)
    //{
    //    if (HasFocus())
    //    {
    //        args[SDLRawInput::P_CONSUMED] = true;
    //    }
    //}
}

} // end namespace Urho3D

