// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "image/tb_multiple_image.h"
#include "tb_widgets_reader.h"
#include "tb_node_tree.h"
#include "tb_str.h"
#include "tb_system.h"

#include <cstdlib> 
#include <time.h>

#ifdef TB_IMAGE

namespace tb {

void TBMultipleImage::ImageInfo::Init(int id, bool expand)
{
    m_expand = expand;
    m_lastTime = -1.0; 
    m_scale = 1.0f;
    m_position = TBPoint();
    m_pauseTime = expand ? 400: 900;
    m_paused = false;
    m_id = id;
}

void TBMultipleImage::ImageInfo::UpdatePosition(double timeMS, const TBRect& rect, 
                int expandTime, int shrinkTime, float ratioPercent)
{
    int time = m_expand ? expandTime : shrinkTime;
    if (m_paused)
        time = m_pauseTime;

    if (timeMS > (m_lastTime + time))
    {
        m_lastTime = timeMS;

        if (m_paused)
            m_paused = false;
        else
        {
            if (!m_expand)
                m_paused = true;
        }
        
        if (!m_paused)
        {
            m_expand = !m_expand;
            m_scale = 1.0f;

            if (m_expand)
            {
                int centerx = rect.w / 2;
                int centery = rect.h / 2;
                int size = rect.w < rect.h ? centerx : centery;
                int minSize = int(size * ratioPercent);
                size = rand() % (size - minSize + 1) + minSize;
                int degree = (rand() +  m_id * 137) % 360;
                // int degree = (rand()) % 360;

                float M_DEGTORAD = 3.14159f / 180.0f;
                m_position.x = centerx + int(size * cos(degree * M_DEGTORAD));
                m_position.y = centery + int(size * sin(degree * M_DEGTORAD));
            }
        }
    }

    if (!m_paused)
    {
        // expand
        if (m_expand)
            m_scale = (float(timeMS - m_lastTime) / expandTime);
        else
            m_scale = 1.0f - (float(timeMS - m_lastTime) / shrinkTime);
    }
}

TBMultipleImage::TBMultipleImage(TBCore* core) 
    : TBWidget(core), 
    m_transform(0), 
    m_expandTime(400), 
    m_shrinkTime(500),
    m_size(3),
    m_ratioPercent(0.6f)
{
    InitImagesInfo();
}

void TBMultipleImage::InitImagesInfo()
{
    // Init images
    for (unsigned i = 0; i < m_size; i++)
    {
        ImageInfo& ii = m_imagesInfo[i];
        ii.Init(i, i);
    }
}

PreferredSize TBMultipleImage::OnCalculatePreferredContentSize(const SizeConstraints &constraints)
{
	return PreferredSize(m_image.Width(), m_image.Height());
}

void TBMultipleImage::OnPaint(const PaintProps &paint_props)
{
	if (TBBitmapFragment *fragment = m_image.GetBitmap())
	{
        TBRect srcRect = TBRect(0, 0, m_image.Width(), m_image.Height());
        for (unsigned i = 0; i < m_size; i++)
        {
            ImageInfo& ii = m_imagesInfo[i];
            if (ii.m_position.x == 0 || ii.m_position.y == 0)
                return;

            int w = int(m_image.Width()* ii.m_scale);
            int h = int(m_image.Height()* ii.m_scale);
            int cx = ii.m_position.x;
            int cy = ii.m_position.y;
            TBRect dstRect = TBRect(cx - (w / 2), cy - (h / 2), w, h);

            core_->renderer_->DrawBitmap(dstRect, srcRect, fragment, m_transform);
        }
	}
}

void TBMultipleImage::OnAnimationStart()
{
    double time_now = TBSystem::GetTimeMS();
    srand((unsigned)time_now);
    for (unsigned i = 0; i < m_size; i++)
    {
        ImageInfo& ii = m_imagesInfo[i];
        ii.m_lastTime = TBSystem::GetTimeMS() - (rand() % ((i + 1) * 500));
    }
}

void TBMultipleImage::OnAnimationUpdate(float progress)
{
    double time_now = TBSystem::GetTimeMS();
    TBRect rect = GetPaddingRect();

    for (unsigned i = 0; i < m_size; i++)
    {
        ImageInfo& ii = m_imagesInfo[i];
        ii.UpdatePosition(time_now, rect, m_expandTime, m_shrinkTime, m_ratioPercent);
    }
}

} // namespace tb

#endif // TB_IMAGE
