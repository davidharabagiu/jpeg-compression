//
// File: JPEGCompress.h
// Author: David Harabagiu
//

#ifndef _JPEGCompress_H_
#define _JPEGCompress_H_

#include "common.h"
#include <vector>
#include <cmath>

namespace Smecherie
{
	void JPEGCompress(char *src, char *dst);
	void JPEGDecompress(char *src, char *dst);
}

#endif