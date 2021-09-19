#ifndef TB_COLOR_PICKER_H
#define TB_COLOR_PICKER_H

#include "tb_widgets.h"
#include "tb_layout.h"
#include "tb_msg.h"
#include "tb_widgets_common.h"
#include "image/tb_image_widget.h"

namespace tb {

class TBColorPicker : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBColorPicker, TBWidget);

    TBColorPicker(TBCore * core);
	~TBColorPicker();

	/** Same as SetValue, but with double precision. */
    virtual void SetValueDouble(double value) override;
    virtual double GetValueDouble() override { return m_value; }

    virtual void SetValue(int value) override { SetValueDouble(value); }
    virtual int GetValue() override { return (int) GetValueDouble(); }

	virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override { return m_layout.GetPreferredSize(); }

    virtual void OnInflate(const INFLATE_INFO &info) override;
    virtual bool OnEvent(const TBWidgetEvent &ev) override;
    virtual void OnResized(int old_w, int old_h) override;
	virtual void OnPaint(const PaintProps &paint_props) override;
	virtual void OnSkinChanged() override;

	const TBColor& GetColor() const { return m_color; }

	void SetColor(const TBColor& color) { m_color = color; m_setColor = true; }

	float m_valueColor;

	uint32* m_data;

protected:
	TBLayout m_layout;
	TBLayout m_layout0;
	TBLayout m_layout1;
	TBWidget m_handle;
	// TBSkinImage m_wheelColor;
	// TBImageWidget m_imgColor;
	TBWidget m_imgColor;
	TBTextField m_colorWheel;
	TBSlider m_sliderY;
	TBSlider m_sliderX;
	TBSkinElement *m_skinElement;
	
	TBColor m_color;

	bool m_setColor;

	double m_value;
	
	double m_to_pixel_factor;
	
	void UpdateHandle(int x, int y);
	void UpdateFromColor();
	void UpdateColor();
	void UpdateValueColor();
	void CreatePalette();
	void ToPolar(int x, int y, float& theta, float& r);
	void FromPolar(float theta, float r, int& x, int& y);

	void Bounds(float* min, float* max, bool clipped = false) const;
	float SaturationHSV() const;
	float SaturationHSV(float min, float max) const;
	float Hue() const;
	float Hue(float min, float max) const;
};

} // namespace tb

#endif