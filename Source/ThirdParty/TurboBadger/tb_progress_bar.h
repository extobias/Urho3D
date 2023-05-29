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

	void SetClearOffset(unsigned clearOffset) { m_clearBarOffset = clearOffset; }

	void SetClearTime(unsigned clearTime) { m_clearTotalTime = clearTime; }

	virtual void SetValueDouble(double value) override;
    virtual double GetValueDouble() override { return m_value; }

    virtual void SetValue(int value) override { SetValueDouble(value); }
    virtual int GetValue() override { return (int) GetValueDouble(); }

	virtual void OnInflate(const INFLATE_INFO &info) override;
	virtual bool OnEvent(const TBWidgetEvent &ev) override;
    virtual void OnResized(int old_w, int old_h) override;

	virtual void OnMessageReceived(TBMessage *msg) override;

	void AddColorValue(float percent, const TBColor& color);

	void RemoveColorValue(unsigned index);

	void Clear();
	int ClearProgressBar(unsigned index);
	
protected:

	void UpdateBar();

	bool NextOffset(int& offset, unsigned& childNumber);

	void AddColorBar(int offset, unsigned childNumber, float percent, const TBColor& color);

	void UpdateColorBar(unsigned offset, TBColorBar* imgColor);

	bool ClearColorBar(unsigned index);

	TBLayout m_layout;
	TBWidget m_layoutColor;
	TBListAutoDeleteOf<TBSkinElement> m_skinChildren;

	double m_value;
	double m_lastValue;
	double m_step;
	double m_elapsedTime;
	double m_lastTime;
	double m_totalTime;
	double m_wastedTime;
	double m_currentTime;
	double m_currentStep;

	unsigned m_clearBarOffset;
	unsigned m_clearTotalTime;
};

} // namespace tb

#endif