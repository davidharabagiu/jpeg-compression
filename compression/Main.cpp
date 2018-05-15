#include "stdafx.h"
#include "common.h"
#include "JPEGCompress.h"

int main(int argc, char **argv)
{
	if (argc < 4 || (argv[1][0] != 'c' && argv[1][0] != 'd'))
	{
		std::cout << "Usage: " << argv[0] << " c/d src_file dst_file" << std::endl;
		return 0;
	}

	if (argv[1][0] == 'c')
	{
		if (argc >= 5)
		{
			try
			{
				std::cout << "ok " << argv[4] << std::endl;
				Smecherie::JPEGCompress(argv[2], argv[3], std::stoi(std::string{ argv[4] }));
			}
			catch (std::invalid_argument)
			{
				std::cout << "Invalid argument " << argv[4] << std::endl;
				Smecherie::JPEGCompress(argv[2], argv[3]);
			}
		}
		else
		{
			Smecherie::JPEGCompress(argv[2], argv[3]);
		}
	}
	else
	{
		Smecherie::JPEGDecompress(argv[2], argv[3]);
	}

	return 0;
}