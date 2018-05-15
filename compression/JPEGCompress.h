//
// File: JPEGCompress.h
// Author: David Harabagiu
//

#ifndef _JPEGCompress_H
#define _JPEGCompress_H

#include "common.h"
#include <vector>
#include <cmath>

namespace Smecherie
{
	void JPEGCompress(char *src, char *dst, int quality = 32);
	void JPEGDecompress(char *src, char *dst);
}

#endif