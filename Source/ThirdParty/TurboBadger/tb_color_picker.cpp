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
    , m_layout1(core)
    , m_handle(core)
    , m_imgColor(core)
    , m_colorWheel(core)
    , m_sliderY(core)
    , m_sliderX(core)
    , m_value(100.0)
    , m_to_pixel_factor(0)
    , m_valueColor(1.0)
    , m_setColor(false)
    , m_data(nullptr)
{
    SetIsFocusable(true);

    AddChild(&m_layout);
    
    m_layout.SetRect(GetPaddingRect());
    m_layout.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout.SetAxis(AXIS_Y);
    m_layout.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);

    // FIXME replace this fixed values
    m_imgColor.SetGravity(WIDGET_GRAVITY_ALL);
    m_imgColor.SetRect(TBRect(0, 0, 100, 100));
    LayoutParams lp;
    lp.min_h = 20;
    lp.max_h = 20;
    m_imgColor.SetLayoutParams(lp);
    m_layout.AddChild(&m_imgColor);
 
    m_layout0.SetAxis(AXIS_X);
    m_layout0.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout0.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);
    m_layout0.SetLayoutPosition(LAYOUT_POSITION_CENTER);
    m_layout.AddChild(&m_layout0);

    m_layout1.SetAxis(AXIS_Y);
    m_layout1.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout1.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);
    m_layout1.SetLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
    m_layout1.SetLayoutPosition(LAYOUT_POSITION_LEFT_TOP);
    m_layout0.AddChild(&m_layout1);

    m_colorWheel.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout1.AddChild(&m_colorWheel);

    m_sliderX.SetAxis(AXIS_X);
    m_sliderX.SetGravity(WIDGET_GRAVITY_ALL);
    m_sliderX.SetLimits(0, 100);
    m_sliderX.SetValue(100);
    m_layout1.AddChild(&m_sliderX);

    m_sliderY.SetAxis(AXIS_Y);
    m_sliderY.SetGravity(WIDGET_GRAVITY_ALL);
    m_sliderY.SetLimits(0, 100);
    m_sliderY.SetValue(100);
    m_layout0.AddChild(&m_sliderY);
    
    m_handle.SetSkinBg(TBIDC("TBColorFg"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
    AddChild(&m_handle);

    TBWidgetValue *value = g_value_group.CreateValueIfNeeded("value", TBValue::TYPE::TYPE_INT);
    value->SetInt(100);
    m_sliderY.Connect(value);
    Connect(value);

    TBWidgetValue *alpha = g_value_group.CreateValueIfNeeded("alpha", TBValue::TYPE::TYPE_INT);
    alpha->SetInt(100);
    m_sliderX.Connect(alpha);
    Connect(alpha);

    CreatePalette();

    Invalidate();
}

TBColorPicker::~TBColorPicker()
{
    if (m_data)
    {
        delete m_skinElement;
        free(m_data);
    }

    RemoveChild(&m_handle);
    m_layout1.RemoveChild(&m_colorWheel);
    m_layout1.RemoveChild(&m_sliderX);

    m_layout0.RemoveChild(&m_layout1);
    m_layout0.RemoveChild(&m_sliderY);
    
    m_layout.RemoveChild(&m_imgColor);
    m_layout.RemoveChild(&m_layout0);

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

    m_skinElement = core_->tb_skin_->GetSkinElement(TBIDC("TBColorWheel"));
    if (m_skinElement)
        return;

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
    m_data = (uint32*) malloc(w * h * size);
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

                m_data[index] = (color);
            }
            else
            {
                m_data[index] = c;
            }
        }
    }

    m_skinElement->bitmap = core_->tb_skin_->GetFragmentManager()->CreateNewFragment(TBIDC("TBColorWheel"), false, w, h, w, m_data);
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
    if (ev.type == EVENT_TYPE_POINTER_MOVE && core_->captured_widget == &m_handle)
    {
        int dx = ev.target_x - core_->pointer_down_widget_x;
        int dy = ev.target_y - core_->pointer_down_widget_y;

        TBRect rectLay = m_layout0.GetRect();
        TBRect p = m_colorWheel.GetRect();
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

        int centerx = (p.w / 2) - (handlePS.pref_w / 2);
        int centery = (p.h / 2) + rectLay.y - (handlePS.pref_h / 2);

        int a = x - centerx;
        int b = centery - y;
        float t, r;
        ToPolar(a, b, t, r);
        float angle = t * 360.0f / (2 * M_PI);

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
            TBWidgetValue *value = g_value_group.CreateValueIfNeeded("value", TBValue::TYPE::TYPE_INT);
            m_valueColor = value->GetInt() / 100.0;
            
            UpdateColor();

            return true;
        }
        else if (ev.target == &m_sliderX)
        {
            TBWidgetValue *alpha = g_value_group.CreateValueIfNeeded("alpha", TBValue::TYPE::TYPE_INT);
            m_color.a = (unsigned)((alpha->GetInt() / 100.0f) * 255);
           
            UpdateColor();

            return true;
        }
    }
    return false;
}

/// update color from handle
void TBColorPicker::UpdateColor()
{
    TBRect rectLay = m_layout0.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();
    TBRect p = m_colorWheel.GetRect();
    TBRect handleRect = m_handle.GetRect();
    
    int x = handleRect.x;
    int y = handleRect.y;

    int centerx = (p.w / 2) - (handlePS.pref_w / 2);
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
    m_color.r = color.r;
    m_color.g = color.g;
    m_color.b = color.b;
}

/// update color with m_valueColor
void TBColorPicker::UpdateValueColor()
{
    float min, max;
    Bounds(&min, &max, true);

    float h = Hue(min, max);
    float s = SaturationHSV(min, max);
    float v = m_valueColor;
    double rr, gg, bb;

    hsv2rgb(h, s, v, rr, gg, bb);
    TBColor color(rr * 255, gg * 255, bb * 255);
    // m_color = color;
    m_color.r = color.r;
    m_color.g = color.g;
    m_color.b = color.b;

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
    TBRect p = m_colorWheel.GetRect();

    // FIXME pq suma 30?
    // float t = h * (2 * M_PI) / 360.0f;
    float t = (h * 360.0f + 30.0f);
    float r = s * (p.w / 2);

    if (t > 360.0f)
        t = t - 360.0f;

    int a, b;
    FromPolar(t * M_PI / 180.0f, r, a, b);

    TBRect rectLay = m_layout0.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();
    int centerx = (p.w / 2) - (handlePS.pref_w / 2);
    int centery = (p.h / 2) + rectLay.y - (handlePS.pref_h / 2);

    int x = a + centerx;
    int y = centery - b;

    UpdateHandle(x, y);

    m_sliderY.SetValue(static_cast<int>(round(m_valueColor * 100.0f)));

    m_sliderX.SetValue(static_cast<int>(round((m_color.a / 255.0f) * 100.0f)));
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

    TBRect rectLay = m_layout0.GetRect();
    TBRect rectWheel = m_colorWheel.GetRect();
    TBRect handleRect = m_handle.GetRect();
    TBRect sliderRect = m_sliderY.GetRect();
    PreferredSize handlePS = m_handle.GetPreferredSize();
    PreferredSize imgPS = m_imgColor.GetPreferredSize();

    int size = rectWheel.w > rectWheel.h ? rectWheel.h : rectWheel.w;
    rectWheel.w = rectWheel.h = size;
    m_colorWheel.SetRect(rectWheel);

    sliderRect.h = size;
    m_sliderY.SetRect(sliderRect);

    TBRect sliderXRect = m_sliderX.GetRect();
    sliderXRect.y = rectWheel.y + rectWheel.h;
    sliderXRect.w = size;
    m_sliderX.SetRect(sliderXRect);
    
    UpdateFromColor();
}

void TBColorPicker::OnPaint(const PaintProps &paint_props) 
{
    TBWidget::OnPaint(paint_props);

    TBRect rectLay = m_layout0.GetRect();
    TBRect rect = m_colorWheel.GetRect();
    TBRect p = m_colorWheel.GetRect();
    
    TBWidgetSkinConditionContext context(this);
    WIDGET_STATE state = GetAutoState();

    int trns_x = rectLay.x, trns_y = rectLay.y;
    core_->renderer_->Translate(trns_x, trns_y);
    core_->tb_skin_->PaintSkin(rect, m_skinElement, static_cast<SKIN_STATE>(state), context);
    core_->renderer_->Translate(-trns_x, -trns_y);

    TBRect colorRect(0, 0, rectLay.w, rectLay.y);
    core_->tb_skin_->PaintRectFill(colorRect, m_color);

    int size = 10;
    PreferredSize handlePS = m_handle.GetPreferredSize();
    int centerx = (p.w / 2);
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