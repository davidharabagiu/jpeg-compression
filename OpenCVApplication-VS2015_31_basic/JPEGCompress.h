#ifndef _JPEGCompress_H_
#define _JPEGCompress_H_

#include "common.h"
#include <vector>
#include <cmath>

namespace Smecherie
{
	void JPEGCompress(Mat_<Vec3b>& img, char *path);
}

#endif