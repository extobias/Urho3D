// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#ifndef TB_MUTLTIPLE_IMAGE_H
#define TB_MUTLTIPLE_IMAGE_H

#include "tb_widgets.h"

#ifdef TB_IMAGE

#include "image/tb_image_manager.h"

namespace tb {

class TBMultipleImage : public TBWidget
{
public:
	struct ImageInfo {
		void Init(int id, bool expand);
		void UpdatePosition(double timeMS, const TBRect& rect, int expandTime, int shrinkTime, float ratioPercent);

		TBPoint m_position;
		bool m_expand;
		double m_lastTime;
		float m_scale;
		int m_pauseTime;
		bool m_paused;
		int m_id;
	};
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBMultipleImage, TBWidget);

    TBMultipleImage(TBCore* core);

	void OnAnimationStart() override;
	void OnAnimationUpdate(float progress) override;

	void SetImage(const TBImage &image) { m_image = image; }
    void SetImage(const char *filename) { m_image = core_->image_manager_->GetImage(filename); }

	void SetTransform(int transform) { m_transform = transform; }

    virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override;

    virtual void OnInflate(const INFLATE_INFO &info) override;
    virtual void OnPaint(const PaintProps &paint_props) override;

	void InitImagesInfo();
private:
	static const int MAX_IMAGES = 5;
	TBImage m_image;
	int m_transform;
	unsigned m_size;
	int m_expandTime;
	int m_shrinkTime;
	ImageInfo m_imagesInfo[MAX_IMAGES];
	float m_ratioPercent;
};

} // namespace tb

#endif // TB_IMAGE
#endif // TB_MULTIPLE_IMAGE_H
