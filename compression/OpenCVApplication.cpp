#include "stdafx.h"
#include "common.h"
#include "JPEGCompress.h"

void testJPEGCompress()
{
	char fname[MAX_PATH];
	if (!openFileDlg(fname))
		return;
	Smecherie::JPEGCompress(fname, "test.bin");
}

void testJPEGDecompress()
{
	char fname[MAX_PATH];
	if (!openFileDlg(fname))
		return;
	Smecherie::JPEGDecompress(fname, "test.jpg");
}

int main(int argc, char **argv)
{
	if (argc < 4 || (argv[1][0] != 'c' && argv[1][0] != 'd'))
	{
		std::cout << "Usage: " << argv[0] << " c/d src_file dst_file" << std::endl;
		return 0;
	}

	if (argv[1][0] == 'c')
	{
		Smecherie::JPEGCompress(argv[2], argv[3]);
	}
	else
	{
		Smecherie::JPEGDecompress(argv[2], argv[3]);
	}

	return 0;
}