#pragma once

#include <algorithm>

namespace mediasoup {

inline int scaledDimension(int sourceDim, double scaleDownBy)
{
	double safeScale = scaleDownBy >= 1.0 ? scaleDownBy : 1.0;
	int scaled = static_cast<int>(sourceDim / safeScale);
	if (scaled < 2) scaled = 2;
	if (scaled % 2 != 0) scaled -= 1;
	return std::max(2, scaled);
}

} // namespace mediasoup
