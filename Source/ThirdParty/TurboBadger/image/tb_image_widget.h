// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#ifndef TB_IMAGE_WIDGET_H
#define TB_IMAGE_WIDGET_H

#include "tb_widgets.h"

#ifdef TB_IMAGE

#include "image/tb_image_manager.h"

namespace tb {

/** TBImageWidget is a widget showing a image loaded by TBImageManager,
	constrained in size to its skin.
	If you need to show a image from the skin, you can use TBSkinImage. */

class TBImageWidget : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBImageWidget, TBWidget);

    TBImageWidget(TBCore* core) : TBWidget(core), m_transform(0) {}

	void SetImage(const TBImage &image) { m_image = image; }
    void SetImage(const char *filename) { m_image = core_->image_manager_->GetImage(filename); }

	void SetTransform(int transform) { m_transform = transform; }

    virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override;

    virtual void OnInflate(const INFLATE_INFO &info) override;
    virtual void OnPaint(const PaintProps &paint_props) override;
private:
	TBImage m_image;
	int m_transform;
};

} // namespace tb

#endif // TB_IMAGE
#endif // TB_IMAGE_WIDGET_H
