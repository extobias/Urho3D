#include "tb_progress_bar.h"

#include "tb_msg.h"
#include "tb_widget_skin_condition_context.h"
#include "tb_system.h"
#include "tb_types.h"

namespace tb {

const uint32 msg_delay = 1000 / 30;

TBColorBar::TBColorBar(TBCore* core)
    : TBWidget (core)
    , m_value(0.0)
{

}

TBColorBar::~TBColorBar()
{

}

void TBColorBar::SetValueDouble(double value)
{
    m_value = value;
}

//============= TBProgressBar =============
TBProgressBar::TBProgressBar(TBCore *core)
    : TBWidget (core)
    , m_layout(core)
    // , m_imgColor(core)
    , m_layoutColor(core)
    , m_value(0.0)
    , m_lastValue(0.0)
    , m_step(0.02)
    , m_clearIndex(0)
    , m_clearBarOffset(2)
    , m_clearTotalTime(5)
{
    LayoutParams lp1;
    lp1.min_h = 25;
    lp1.max_h = 65;
    lp1.min_w = 25;
    lp1.max_w = 200;
    lp1.pref_h = 45;
    lp1.pref_w = 200;
    // SetLayoutParams(lp1);
    
    m_layoutColor.SetID(TBID("layout-color"));
    // m_layoutColor.SetLayoutParams(lp1);
    m_layoutColor.SetZ(WIDGET_Z_TOP);
    AddChild(&m_layoutColor);
    
    m_layout.SetGravity(WIDGET_GRAVITY_ALL);
    m_layout.SetAxis(AXIS_X);
    m_layout.SetLayoutDistribution(LAYOUT_DISTRIBUTION_GRAVITY);
    m_layout.SetLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
    m_layout.SetZ(WIDGET_Z_TOP);
    m_layout.SetSkinBg(TBIDC("TBProgressBar_bg"), WIDGET_INVOKE_INFO_NO_CALLBACKS);
    AddChild(&m_layout);
}

TBProgressBar::~TBProgressBar()
{
    Clear();

    RemoveChild(&m_layoutColor);
    
    RemoveChild(&m_layout);
}

bool TBProgressBar::OnEvent(const TBWidgetEvent &ev)
{
    return false;
}

void TBProgressBar::OnResized(int old_w, int old_h)
{
    TBWidget::OnResized(old_w, old_h);
    TBRect rect = GetRect();
    TBRect layoutRect = m_layoutColor.GetRect();
    layoutRect.w = rect.w;
    layoutRect.h = rect.h;
    m_layoutColor.SetRect(layoutRect);

    UpdateBar();
}

void TBProgressBar::SetValueDouble(double value)
{
    if (value == m_value)
        return;

    if (value < 0.0 || value >= 1.0)
        return; 

    m_lastValue = m_value;
    m_value = value;

    if (m_value < 1.0)
	{
		// Start animation
		if (!GetMessageByID(TBID("pg_update")))
		{
			PostMessageDelayed(TBID("pg_update"), nullptr, msg_delay);
		}
	}
	else
	{
		// Stop animation
		if (TBMessage *msg = GetMessageByID(TBID("pg_update")))
			DeleteMessage(msg);
	}
}

void TBProgressBar::SetLastValue(double value)
{
    m_lastValue = value;
    m_value = value;

    UpdateBar();
    Invalidate();
}

void TBProgressBar::OnMessageReceived(TBMessage *msg)
{
    if (msg->message == TBID("pg_clear"))
    {
        if (!msg->data)
            return;

        unsigned index = msg->data->v1.GetInt();
        bool finish = ClearColorBar(index);
        if (!finish)
        {
            Invalidate();
            // Keep animation running
            TBMessageData *data = new TBMessageData();
            data->v1.SetInt(index);

            PostMessageDelayed(TBID("pg_clear"), data, msg_delay);
        }
        else
        {
            // if (TBMessage *msg = GetMessageByID(TBID("pg_clear")))
			    // DeleteMessage(msg);

            TBWidgetEvent ev(EVENT_TYPE_CHANGED);
            InvokeEvent(ev);
        }
    }
}

void TBProgressBar::UpdateBar()
{
    int offset = 0;
    unsigned childNumber = 0;
    for (TBWidget *child = m_layoutColor.GetFirstChild(); child; child = child->GetNext())
    {
        UpdateColorBar(offset, (TBColorBar*)child);
        
        TBRect rect = child->GetRect();
        offset += childNumber == 0 ? rect.x + rect.w : rect.w;
        childNumber++;
    }
}

bool TBProgressBar::NextOffset(int& offset, unsigned& childNumber)
{
    for (TBWidget *child = m_layoutColor.GetFirstChild(); child; child = child->GetNext())
    {
        TBRect rect = child->GetRect();
        offset += childNumber == 0 ? rect.x + rect.w : rect.w;

        childNumber++;
    }

    return true;
}

void TBProgressBar::AddColorBar(int offset, unsigned childNumber, float percent, const TBColor& color)
{
    TBStr childName;
    childName.SetFormatted("color-%d", childNumber);
    TBColorBar* imgColor = new TBColorBar(core_);
    imgColor->SetID(TBID(childName.CStr()));
    imgColor->SetGravity(WIDGET_GRAVITY_ALL);
    imgColor->SetZ(WIDGET_Z_TOP);
    imgColor->SetValueDouble(percent);

    TBSkinElement* skin = new TBSkinElement;
    if (skin)
    {
        skin->bg_color = color;
        skin->bg_color_blend = color;
        // if (offset == 0)
        // {
        //     skin->bg_color = color;
        //     skin->bg_color_blend = color;
        // }
        // else
        // {
        //     TBStr lastName;
        //     lastName.SetFormatted("color-%d", childNumber - 1);
        //     TBSkinElement* lastSkin = core_->tb_skin_->GetSkinElement(TBID(lastName.CStr()));    
        //     if (lastSkin)
        //     {
        //         TBColor blendColor = color.Blend(lastSkin->bg_color);
        //         skin->bg_color = blendColor;
        //         skin->bg_color_blend = color;

        //         lastSkin->bg_color_blend = blendColor;
        //     }
        //     else
        //     {
        //         skin->bg_color = color;
        //         skin->bg_color_blend = color;
        //     }
        // }

        core_->tb_skin_->AddSkinElement(TBID(childName.CStr()), skin);
    }

    imgColor->SetSkinBg(TBIDC(childName.CStr()), WIDGET_INVOKE_INFO_NO_CALLBACKS);
    m_layoutColor.AddChild(imgColor);

    UpdateColorBar(offset, imgColor);
}

void TBProgressBar::UpdateColorBar(unsigned offset, TBColorBar* imgColor)
{
    TBRect imgRect = imgColor->GetRect();
    TBRect layoutRect = m_layoutColor.GetRect();

    bool horizontal = true;
    float available_pixels = horizontal ? (float) GetRect().w : (float) GetRect().h;
        
    float percent = static_cast<float>(imgColor->GetValueDouble());
    float pad = 5.0f;
    imgRect.x = offset != 0 ? offset : static_cast<int>(pad);
    imgRect.w = static_cast<int>(available_pixels * percent);
    if (imgRect.x + imgRect.w > available_pixels)
    {
        imgRect.w = static_cast<int>(available_pixels - pad) - imgRect.x;
    }

    TBRect padRect = m_layout.GetPaddingRect();
    TBRect parentRect = GetRect();
    imgRect.h = layoutRect.h;

    imgColor->SetRect(imgRect);

    LayoutParams lp1;
    lp1.pref_w = imgRect.w;
    lp1.pref_h = imgRect.h;
    imgColor->SetLayoutParams(lp1);
}

void TBProgressBar::AddColorValue(float percent, const TBColor& color)
{
    int offset = 0;
    unsigned childNumber = 0;
    NextOffset(offset, childNumber);
    
    AddColorBar(offset, childNumber, percent, color);
}

void TBProgressBar::RemoveColorValue(unsigned index)
{
    TBWidget* child = m_layoutColor.GetChildFromIndex(index);
    if (child)
    {
        m_layoutColor.RemoveChild(child);
        delete child;
    }
}

bool TBProgressBar::ClearColorBar(unsigned index)
{
    TBWidget* child = m_layoutColor.GetChildFromIndex(index);
    if (!child)
        return true;

    TBRect rect = child->GetRect();
    TBRect layRect = m_layoutColor.GetRect();
    
    // 30 (de 1/30s) * 10s (tiempo total)
    int fact = 30 * m_clearTotalTime;
    unsigned pad = 5;
    int step = ((layRect.w - pad * 2) + fact - 1) / fact;
    m_clearBarOffset = MAX(step, 1);

    rect.w -= m_clearBarOffset;
    child->SetRect(rect);

    return (rect.w <= 0);
}

void TBProgressBar::Clear()
{
    for (TBWidget *child = m_layoutColor.GetFirstChild(); child; child = child->GetNext())
    {
        core_->tb_skin_->RemoveSkinElement(child->GetID());
    }
    m_layoutColor.DeleteAllChildren();
}

void TBProgressBar::ClearProgressBar(unsigned index)
{
    if (!GetMessageByID(TBID("pg_clear")))
    {
        int offset = 0;
        unsigned childNumber = 0;
        NextOffset(offset, childNumber);
        m_clearIndex = childNumber;

        if ((index + 1) > childNumber)
            return;
        
        TBMessageData *data = new TBMessageData();
        data->v1.SetInt(index);
        PostMessageDelayed(TBID("pg_clear"), data, msg_delay);
    }
}

}