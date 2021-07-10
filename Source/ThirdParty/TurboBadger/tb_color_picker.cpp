#include "tb_color_picker.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_system.h"
#include <stdlib.h> 
#define _USE_MATH_DEFINES
#include <math.h>
#include <limits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // Conversion from 'double' to 'float'
#pragma warning(disable:4702) // unreachable code
#endif

namespace tb {

TBColorPicker::TBColorPicker(TBCore *core)
    : TBWidget (core)
    , m_layout(core)
    , m_layout0(core)
    , m_handle(core)
    , m_imgColor(core)
    , m_textWheel(core)
    , m_sliderY(core)
    , m_value(100.0)
    , m_to_pixel_factor(0)
    , m_valueColor(1.0)
    , m_setColor(false)
{
    SetIsFocusable(true);
    // SetAxis(AXIS_X);
    // SetSkinBg(TBIDC("TBButton"), WIDGET_INVOKE_INFO_NO_CALLBACKS);

    AddChild(&m_layout);
    
    m_layout.SetRect(GetPaddingRect());
    m_layout.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout.SetAxis(AXIS_Y);
    m_layout.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);

    m_imgColor.SetGravity(WIDGET_GRAVITY_ALL);
    m_imgColor.SetImage("Data/UI/TB/images/tb_logo.png");
    m_layout.AddChild(&m_imgColor);
 
    m_layout0.SetAxis(AXIS_X);
    m_layout0.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout0.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);
    m_layout0.SetLayoutPosition(LAYOUT_POSITION_CENTER);
    m_layout.AddChild(&m_layout0);

    m_textWheel.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout0.AddChild(&m_textWheel);

    m_sliderY.SetAxis(AXIS_Y);
    m_sliderY.SetGravity(WIDGET_GRAVITY_ALL);
    m_sliderY.SetLimits(0, 100);
    m_layout0.AddChild(&m_sliderY);
    
    m_handle.SetSkinBg(TBIDC("TBColorFg"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
    AddChild(&m_handle);

    TBWidgetValue *value = g_value_group.CreateValueIfNeeded("val", TBValue::TYPE::TYPE_INT);
    m_sliderY.Connect(value);
    Connect(value);

    CreatePalette();

    Invalidate();
}

TBColorPicker::~TBColorPicker()
{
    RemoveChild(&m_handle);
    m_layout0.RemoveChild(&m_textWheel);
    m_layout0.RemoveChild(&m_sliderY);

    m_layout.RemoveChild(&m_imgColor);
    m_layout.RemoveChild(&m_layout0);

    delete m_skinElement;

    RemoveChild(&m_layout);
}

void TBColorPicker::ToPolar(int x, int y, float& theta, float& r)
{
    theta = atan2(y, x);

    if (theta < 0)
    {
        theta += (2.0f * M_PI);
    }
    r = sqrt((x*x) + (y*y));
}

void TBColorPicker::FromPolar(float theta, float r, int& x, int& y)
{
    x = r * cos(theta);
    y = r * sin(theta);
}

void hsv2rgb(double h, double s, double v, double& r, double& g, double& b)
{
    double hh, p, q, t, ff;
    long i;

    // < is bogus, just shuts up warnings
    if(s <= 0.0) 
    {
        r = v;
        g = v;
        b = v;
        return;
    }
    hh = h;
    if(hh >= 360.0) 
        hh = 0.0;

    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
    default:
        r = v;
        g = p;
        b = q;
        break;
    }
}

void TBColorPicker::CreatePalette()
{
    unsigned long long w = 1ULL << 12;
    unsigned long long h = 1ULL << 12;

    m_skinElement = new TBSkinElement;
    m_skinElement->width = w;
    m_skinElement->height = h;
    TBBitmapFragment *bitmap = core_->tb_skin_->GetFragmentManager()->GetFragment(TBIDC("TBColorWheel"));
    if (bitmap)
    {
        m_skinElement->bitmap = bitmap;
        return;
    }

    unsigned c = (0x00 << 24) | (0x00 << 16) | (0x00 << 8) | 0x00;
    unsigned size =  sizeof(uint32);
    uint32* data = (uint32*) malloc(w * h * size);
    for (unsigned i = 0; i < w; i++)
    {
        for (unsigned j = 0; j < h; j++)
        {
            unsigned index = i * w + j;
            int a = i - w/2;
            int b = h/2 - j;
            float t, r;
            ToPolar(a, b, t, r);
            
            if (r <= w/2)
            {
                float hue = t * 360.0f / (2 * M_PI);
                float sat = r / (w / 2);

                double rr, gg, bb;
                hsv2rgb(hue, sat, 1.0, rr, gg, bb);
                TBColor color(rr * 255, gg * 255, bb * 255);

                data[index] = (color);
            }
            else
            {
                data[index] = c;
            }

            // TBColor color((index) & 0xFF, (index >> 8) & 0xFF, (index >> 16) & 0xFF);
            // data[index] = (color);
        }
    }

    m_skinElement->bitmap = core_->tb_skin_->GetFragmentManager()->CreateNewFragment(TBIDC("TBColorWheel"), false, w, h, w, data);

    free(data);
}

void TBColorPicker::SetValueDouble(double value)
{
    if (value == m_value)
        return;
    m_value = value;

    // UpdateHandle(0, 0);
    // UpdateColor();
    // FIXME this event fire an update no wanted
    // TBWidgetEvent ev(EVENT_TYPE_CHANGED);
    // InvokeEvent(ev);
}

bool TBColorPicker::OnEvent(const TBWidgetEvent &ev)
{
    // int gx = core_->pointer_move_widget_x;
    // int gy = core_->pointer_move_widget_y;
    // ev.target->ConvertToRoot(gx, gy);
    // TBDebugPrint("TBColorPicker::OnEvent: ev <%i, %i>\n", gx, gy);

    if (ev.type == EVENT_TYPE_POINTER_MOVE && core_->captured_widget == &m_handle)
    {
        int dx = ev.target_x - core_->pointer_down_widget_x;
        int dy = ev.target_y - core_->pointer_down_widget_y;

        TBRect rectLay = m_layout0.GetRect();
        TBRect p = m_textWheel.GetRect();
        TBRect handleRect = m_handle.GetRect();
        PreferredSize handlePS = m_handle.GetPreferredSize();

        int x = handleRect.x + dx;
        int y = handleRect.y + dy;

        int hw = handlePS.pref_w;
        int hh = handlePS.pref_h;
        int minx = p.x;
        int maxx = p.x + p.w - hw;
        int miny = p.y + rectLay.y;
        int maxy = p.y + rectLay.y + p.h - hh;

        // if (x > maxx)
        //     x = maxx;
        // if (x < minx)
        //     x = minx;
        // if (y > maxy)
        //     y = maxy;
        // if (y < miny)
        //     y = miny;

        // UpdateHandle(x, y);

        // UpdateColor();

        int centerx = (rectLay.w / 2) - (handlePS.pref_w / 2);
        int centery = (p.h / 2) + rectLay.y - (handlePS.pref_h / 2);

        int a = x - centerx;
        int b = centery - y;
        float t, r;
        ToPolar(a, b, t, r);
        float angle = t * 360.0f / (2 * M_PI);

        // TBDebugPrint("TBColorPicker::OnEvent: target pos <%i, %i> angle-radious <%f, %f>\n", a, b, angle, r);
        float rmax = (p.w / 2) - (handlePS.pref_w / 2); 
        if (r < rmax)
        {
            UpdateHandle(x, y);
        }
        else
        {
            UpdateHandle(centerx + cos(t) * rmax, centery - sin(t) * rmax);
        }

        UpdateColor();

        return true;
    }
    else if (ev.type == EVENT_TYPE_CHANGED)
    {
        if (ev.target == &m_sliderY)
        {
            TBWidgetValue *value = g_value_group.CreateValueIfNeeded("val", TBValue::TYPE::TYPE_INT);
            m_valueColor = value->GetInt() / 100.0;

            TBDebugPrint("TBColorPicker::OnEvent: changed <%i> <%f>\n", value->GetInt(), m_valueColor);
            
            // UpdateValueColor();
            UpdateColor();

            return true;
        }
    }
    return false;
}

void TBColorPicker::UpdateColor()
{
    TBRect rectLay = m_layout0.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();
    TBRect p = m_textWheel.GetRect();
    TBRect handleRect = m_handle.GetRect();
    
    int x = handleRect.x;// + (handlePS.pref_w / 2);
    int y = handleRect.y;// + (handlePS.pref_h / 2);

    int centerx = (rectLay.w / 2) - (handlePS.pref_w / 2);
    int centery = (p.h / 2) + rectLay.y - (handlePS.pref_h / 2);

    int a = x - centerx;
    int b = centery - y;
    float t, r;

    ToPolar(a, b, t, r);

    float hue = t * 360.0f / (2 * M_PI);
    float sat = r / ((p.w) / 2);

    double rr, gg, bb;
    // FIXME ver pq hay que restar 30
    hsv2rgb(hue - 30.0, sat, m_valueColor, rr, gg, bb);
    TBColor color(rr * 255, gg * 255, bb * 255);
    m_color = color;

    TBDebugPrint("TBColorPicker::UpdateColor: target pos <%i, %i> pos2 <%i, %i> angle-radious <%f, %f> color <%i, %i, %i, %i> center <%i, %i>\n", 
                a, b, x, y, hue, r, color.r, color.g, color.b, color.a, centerx, centery);

    TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	InvokeEvent(ev);
}

void TBColorPicker::UpdateValueColor()
{
    float min, max;
    Bounds(&min, &max, true);

    float h = Hue(min, max);
    float s = SaturationHSV(min, max);
    float v = m_valueColor;

    TBDebugPrint("TBColorPicker::UpdateValueColor: color 0 <%i, %i, %i, %i> value <%f> max <%f>\n", m_color.r, m_color.g, m_color.b, m_color.a, m_valueColor, max);

    double rr, gg, bb;

    hsv2rgb(h, s, v, rr, gg, bb);
    TBColor color(rr * 255, gg * 255, bb * 255);
    m_color = color;

    TBDebugPrint("TBColorPicker::UpdateValueColor: color 1 <%i, %i, %i, %i> value <%f> max <%f>\n", m_color.r, m_color.g, m_color.b, m_color.a, m_valueColor, max);

    TBWidgetEvent ev(EVENT_TYPE_CHANGED);
	InvokeEvent(ev);
}

void TBColorPicker::UpdateFromColor()
{   
    float min, max;
    Bounds(&min, &max, true);

    float h = Hue(min, max);
    float s = SaturationHSV(min, max);
    float v = max;

    m_valueColor = v;
    TBRect p = m_textWheel.GetRect();

    // float t = h * (2 * M_PI) / 360.0f;
    float t = (h * 360.0f + 30.0f);
    float r = s * (p.w / 2);

    if (t > 360.0f)
        t = t - 360.0f;

    int a, b;
    FromPolar(t * M_PI / 180.0f, r, a, b);

    TBRect rectLay = m_layout0.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();
    int centerx = (rectLay.w / 2) - (handlePS.pref_w / 2);
    int centery = (p.h / 2) + rectLay.y - (handlePS.pref_h / 2);

    int x = a + centerx;
    int y = centery - b;

    TBDebugPrint("TBColorPicker::UpdateFromColor: target pos <%i, %i> pos2 <%i, %i> angle-radious <%f, %f> center <%i,%i>\n", a, b, x, y, t, r, centerx, centery);

    UpdateHandle(x, y);
}

void TBColorPicker::UpdateHandle(int x, int y)
{
    PreferredSize ps = m_handle.GetPreferredSize();
    int w = ps.pref_w;
    int h = ps.pref_h;

    TBRect rect(x, y, w, h);
    m_handle.SetRect(rect);
}

void TBColorPicker::OnResized(int old_w, int old_h)
{
    TBWidget::OnResized(old_w, old_h);
    TBDebugPrint("TBColorPicker::OnResized: onresize old <%i, %i> new <%i, %i>\n", old_w, old_h, GetRect().w, GetRect().h);

    TBRect rectLay = m_layout0.GetRect();
    TBRect rectWheel = m_textWheel.GetRect();
    TBRect handleRect = m_handle.GetRect();
    TBRect sliderRect = m_sliderY.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();

    TBDebugPrint("TBColorPicker::OnResized: onresize layout <%i, %i, %i, %i>\n", rectLay.x, rectLay.y, rectLay.w, rectLay.h);
    TBDebugPrint("TBColorPicker::OnResized: onresize wheel <%i, %i, %i, %i>\n", rectWheel.x, rectWheel.y, rectWheel.w, rectWheel.h);
    TBDebugPrint("TBColorPicker::OnResized: onresize slider <%i, %i, %i, %i>\n", sliderRect.x, sliderRect.y, sliderRect.w, sliderRect.h);

    int size = rectWheel.w > rectWheel.h ? rectWheel.h : rectWheel.w;
    rectWheel.w = rectWheel.h = size;
    rectWheel.x = (rectLay.w / 2) - (size / 2);
    m_textWheel.SetRect(rectWheel);

    sliderRect.h = size;
    m_sliderY.SetRect(sliderRect);
    
    // int centerx = (rectLay.w / 2) - (handlePS.pref_w / 2);
    // int centery = (rectWheel.h / 2) + rectLay.y - (handlePS.pref_h / 2);
 
    // UpdateHandle(centerx, centery);
    UpdateFromColor();
}

void TBColorPicker::OnPaint(const PaintProps &paint_props) 
{
    TBWidget::OnPaint(paint_props);

    TBRect rectLay = m_layout0.GetRect();
    TBRect rect = m_textWheel.GetRect();
    TBRect p = m_textWheel.GetRect();
    
    TBWidgetSkinConditionContext context(this);
    WIDGET_STATE state = GetAutoState();

    int trns_x = rectLay.x, trns_y = rectLay.y;
    core_->renderer_->Translate(trns_x, trns_y);
    core_->tb_skin_->PaintSkin(rect, m_skinElement, static_cast<SKIN_STATE>(state), context);
    core_->renderer_->Translate(-trns_x, -trns_y);

    // core_->renderer_->DrawBitmapColored(rectCol, rectCol, color, m_skinElement->bitmap);
    // core_->renderer_->DrawBitmap(rectCol, rectCol, m_skinElement->bitmap);
    TBRect colorRect(0, 0, rectLay.w, rectLay.y);
    core_->tb_skin_->PaintRectFill(colorRect, m_color);

    int size = 10;
    int centerx = (rectLay.w / 2);
    int centery = (p.h / 2) + rectLay.y;
    TBRect colorRect2(centerx - size / 2, centery - size / 2, size, size);
    core_->tb_skin_->PaintRectFill(colorRect2, m_color);
}

void TBColorPicker::OnSkinChanged()
{
    m_layout.SetRect(GetPaddingRect());
}

void TBColorPicker::Bounds(float* min, float* max, bool clipped) const
{
    assert(min && max);

    float r = m_color.r / 255.0;
    float g = m_color.g / 255.0;
    float b = m_color.b / 255.0;

    if (r > g)
    {
        if (g > b) // r > g > b
        {
            *max = r;
            *min = b;
        }
        else // r > g && g <= b
        {
            *max = r > b ? r : b;
            *min = g;
        }
    }
    else
    {
        if (b > g) // r <= g < b
        {
            *max = b;
            *min = r;
        }
        else // r <= g && b <= g
        {
            *max = g;
            *min = r < b ? r : b;
        }
    }

    if (clipped)
    {
        *max = *max > 1.0f ? 1.0f : (*max < 0.0f ? 0.0f : *max);
        *min = *min > 1.0f ? 1.0f : (*min < 0.0f ? 0.0f : *min);
    }
}

static const float M_EPSILON = 0.000001f;

template <class T>
bool Equals(T lhs, T rhs) { return lhs + std::numeric_limits<T>::epsilon() >= rhs && lhs - std::numeric_limits<T>::epsilon() <= rhs; }

float TBColorPicker::SaturationHSV() const
{
    float min, max;
    Bounds(&min, &max, true);

    return SaturationHSV(min, max);
}

float TBColorPicker::SaturationHSV(float min, float max) const
{
    // Avoid div-by-zero: result undefined
    if (max <= M_EPSILON)
        return 0.0f;

    // Saturation equals chroma:value ratio
    return 1.0f - (min / max);
}

float TBColorPicker::Hue() const
{
    float min, max;
    Bounds(&min, &max, true);

    return Hue(min, max);
}

float TBColorPicker::Hue(float min, float max) const
{
    float chroma = max - min;

    float r = m_color.r / 255.0;
    float g = m_color.g / 255.0;
    float b = m_color.b / 255.0;

    // If chroma equals zero, hue is undefined
    if (chroma <= M_EPSILON)
        return 0.0f;

    // Calculate and return hue
    if (Equals<float>(g, max))
        return (b + 2.0f * chroma - r) / (6.0f * chroma);
    else if (Equals<float>(b, max))
        return (4.0f * chroma - g + r) / (6.0f * chroma);
    else
    {
        float r = (g - b) / (6.0f * chroma);
        return (r < 0.0f) ? 1.0f + r : ((r >= 1.0f) ? r - 1.0f : r);
    }
}



} // namespace tb