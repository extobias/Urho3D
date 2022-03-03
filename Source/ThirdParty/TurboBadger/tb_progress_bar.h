#ifndef TB_PROGRESS_BAR_H
#define TB_PROGRESS_BAR_H

#include "tb_widgets.h"
#include "tb_layout.h"
#include "tb_msg.h"
#include "tb_widgets_common.h"
#include "image/tb_image_widget.h"

namespace tb {

class TBColorBar : public TBWidget
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBColorBar, TBWidget);

    TBColorBar(TBCore* core);
	~TBColorBar();

	virtual void SetValueDouble(double value) override;
    virtual double GetValueDouble() override { return m_value; }

protected:
	double m_value;
};

class TBProgressBar : public TBWidget, protected TBMessageHandler
{
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBProgressBar, TBWidget);

    TBProgressBar(TBCore * core);
	~TBProgressBar();

	void SetStep(double step) { m_step = step; }

	void SetLastValue(double value);

	virtual void SetValueDouble(double value) override;
    virtual double GetValueDouble() override { return m_value; }

    virtual void SetValue(int value) override { SetValueDouble(value); }
    virtual int GetValue() override { return (int) GetValueDouble(); }

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
    virtual void OnResized(int old_w, int old_h) override;
	virtual void OnPaint(const PaintProps &paint_props) override;

	// virtual PreferredSize OnCalculatePreferredContentSize(const SizeConstraints &constraints) override { return m_layout.GetPreferredSize(); }
	virtual void OnMessageReceived(TBMessage *msg) override;

	void AddColorValue(float percent, const TBColor& color);

	void RemoveColorValue(unsigned index);

	void Clear();
	
protected:

	void UpdateBar();

	bool NextOffset(int& offset, unsigned& childNumber);

	void AddColorBar(int offset, unsigned childNumber, float percent, const TBColor& color);

	void UpdateColorBar(unsigned offset, TBColorBar* imgColor);

	TBLayout m_layout;
	TBContainer m_layoutColor;
	TBListAutoDeleteOf<TBSkinElement> m_skinChildren;

	double m_value;
	double m_lastValue;
	double m_step;
};

} // namespace tb

#endif