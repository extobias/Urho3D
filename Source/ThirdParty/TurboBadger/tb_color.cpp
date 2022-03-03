// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_color.h"
#include <stdio.h>

namespace tb {

// == TBColor ===============================================================================

void TBColor::SetFromString(const char *str, int len)
{
	int r, g, b, a;
	if (len == 9 && sscanf(str, "#%2x%2x%2x%2x", &r, &g, &b, &a) == 4)			// rrggbbaa
		Set(TBColor(r, g, b, a));
	else if (len == 7 && sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3)			// rrggbb
		Set(TBColor(r, g, b));
	else if (len == 5 && sscanf(str, "#%1x%1x%1x%1x", &r, &g, &b, &a) == 4)		// rgba
		Set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4), a + (a << 4)));
	else if (len == 4 && sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3)			// rgb
		Set(TBColor(r + (r << 4), g + (g << 4), b + (b << 4)));
	else
		Set(TBColor());
}

TBColor TBColor::Blend(const TBColor& rhs) const
{
    // over operator
	const float max = 255.0f;
	float rr = r / max;
	float gg = g / max;
	float bb = b / max;
	float aa = a / max;

	// over operator
    float inva = (1 - aa);
    float a1 = aa + (rhs.a / max) * inva;
    float r1 = (rr * aa + (rhs.r / max) * (rhs.a / max) * inva) / aa;
    float g1 = (gg * aa + (rhs.g / max) * (rhs.a / max) * inva) / aa;
    float b1 = (bb * aa + (rhs.b / max) * (rhs.a / max) * inva) / aa;

	return TBColor(r1 * 255.0, g1 * 255.0, b1 * 255.0, a1 * 255.0);
    // return TBColor(r, g, b, a);
}

} // namespace tb
