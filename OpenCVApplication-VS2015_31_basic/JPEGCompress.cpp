#include "stdafx.h"
#include "JPEGCompress.h"

#include <iostream>
#include <fstream>

namespace Smecherie
{
	static const int ql[8][8] {
		{ 16, 11, 10, 16, 24, 40, 51, 61 },
		{ 12, 12, 14, 19, 26, 58, 60, 55 },
		{ 14, 13, 16, 24, 40, 57, 69, 56 },
		{ 14, 17, 22, 29, 51, 87, 80, 62 },
		{ 18, 22, 37, 56, 68, 109, 103, 77 },
		{ 24, 35, 55, 64, 81, 104, 113, 92 },
		{ 49, 64, 78, 87, 103, 121, 120, 101 },
		{ 72, 92, 95, 98, 112, 100, 103, 99 }
	};

	static const int qc[8][8]{
		{ 17, 18, 24, 47, 99, 99, 99, 99 },
		{ 18, 21, 26, 66, 99, 99, 99, 99 },
		{ 24, 26, 56, 99, 99, 99, 99, 99 },
		{ 47, 66, 99, 99, 99, 99, 99, 99 },
		{ 99, 99, 99, 99, 99, 99, 99, 99 },
		{ 99, 99, 99, 99, 99, 99, 99, 99 },
		{ 99, 99, 99, 99, 99, 99, 99, 99 },
		{ 99, 99, 99, 99, 99, 99, 99, 99 }
	};

	static const int QUALITY = 32;

	static Mat_<Vec3i> RGB_to_YCbCr(const Mat_<Vec3b>& img_rgb)
	{
		Mat_<Vec3i> img_ycbcr(img_rgb.rows, img_rgb.cols);

		Vec3b old_val;
		Vec3i new_val;
		for (int i = 0; i < img_rgb.rows; ++i)
		{
			for (int j = 0; j < img_rgb.cols; ++j)
			{
				old_val = img_rgb(i, j);

				new_val[0] = MIN(255, MAX(0, (0.299 * (int)old_val[2]) + (0.587 * (int)old_val[1]) + (0.114 * (int)old_val[0])));
				new_val[1] = MIN(255, MAX(0, 128 - (0.168736 * (int)old_val[2]) - (0.331264 * (int)old_val[1]) + (0.5 * (int)old_val[0])));
				new_val[2] = MIN(255, MAX(0, 128 + (0.5 * (int)old_val[2]) - (0.418688 * (int)old_val[1]) - (0.081312 * (int)old_val[0])));

				// downsample chrominance
				new_val[1] = 2 * (new_val[1] / 2);
				new_val[2] = 2 * (new_val[2] / 2);

				img_ycbcr(i, j) = new_val;
			}
		}

		return img_ycbcr;
	}

	static Mat_<Vec3b> YCbCr_to_RGB(const Mat_<Vec3i>& img_ycbcr)
	{
		Mat_<Vec3b> img_rgb(img_ycbcr.rows, img_ycbcr.cols);

		Vec3i old_val;
		Vec3b new_val;
		for (int i = 0; i < img_ycbcr.rows; ++i)
		{
			for (int j = 0; j < img_ycbcr.cols; ++j)
			{
				old_val = img_ycbcr(i, j);

				new_val[2] = MIN(255, MAX(0, old_val[0] + 1.402 * (old_val[2] - 128)));
				new_val[1] = MIN(255, MAX(0, old_val[0] - 0.344136 * (old_val[1] - 128) - 0.714136 * (old_val[2] - 128)));
				new_val[0] = MIN(255, MAX(0, old_val[0] + 1.772 * (old_val[1] - 128)));

				img_rgb(i, j) = new_val;
			}
		}

		return img_rgb;
	}

	static Mat_<Vec3i> ProcessImg(const Mat_<Vec3i>& src)
	{
		Mat_<Vec3i> dst(src.rows, src.cols);

		double ny, ncb, ncr, ci, cj;
		for (int br = 0; br < src.rows; br += 8)
		{
			for (int bc = 0; bc < src.cols; bc += 8)
			{
				for (int i = 0; i < 8; ++i)
				{
					for (int j = 0; j < 8; ++j)
					{
						ny = ncb = ncr = 0.0;
						for (int x = 0; x < 8; ++x)
						{
							for (int y = 0; y < 8; ++y)
							{
								ny += (src(br + y, bc + x)[0] - 128) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
								ncb += (src(br + y, bc + x)[1]) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
								ncr += (src(br + y, bc + x)[2]) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
							}
						}

						ci = std::sqrt(((i == 0) ? 1.0 : 2.0) / 8);
						cj = std::sqrt(((j == 0) ? 1.0 : 2.0) / 8);

						ny *= ci * cj;
						ncb *= ci * cj;
						ncr *= ci * cj;

						ny /= ql[i][j];
						ncb /= qc[i][j];
						ncr /= qc[i][j];

						ny = (ny - floor(ny) <= 0.5) ? floor(ny) : ceil(ny);
						ncb = (ncb - floor(ncb) <= 0.5) ? floor(ncb) : ceil(ncb);
						ncr = (ncr - floor(ncr) <= 0.5) ? floor(ncr) : ceil(ncr);

						dst(br + i, bc + j) = Vec3i{ (int)ny, (int)ncb, (int)ncr };
					}
				}
			}
		}

		return dst;
	}

	static Mat_<Vec3i> ProcessImg2(const Mat_<Vec3i>& src)
	{
		const double alpha0 = 1 / sqrt(2.0);

		Mat_<Vec3i> dst(src.rows, src.cols);
		double ny, ncb, ncr, ci, cj, cy, ccb, ccr;

		for (int br = 0; br < src.rows; br += 8)
		{
			for (int bc = 0; bc < src.cols; bc += 8)
			{
				for (int x = 0; x < 8; ++x)
				{
					for (int y = 0; y < 8; ++y)
					{
						ny = ncb = ncr = 0.0;
						for (int i = 0; i < 8; ++i)
						{
							for (int j = 0; j < 8; ++j)
							{
								ci = std::sqrt(((i == 0) ? 1.0 : 2.0) / 8);
								cj = std::sqrt(((j == 0) ? 1.0 : 2.0) / 8);

								cy = src(br + i, bc + j)[0] * ql[i][j];
								ccb = src(br + i, bc + j)[1] * qc[i][j];
								ccr = src(br + i, bc + j)[2] * qc[i][j];

								ny += ci * cj * cy * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
								ncb += ci * cj * ccb * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
								ncr += ci * cj * ccr * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
							}
						}

						ny = (ny - floor(ny) <= 0.5) ? floor(ny) : ceil(ny);
						ncb = (ncb - floor(ncb) <= 0.5) ? floor(ncb) : ceil(ncb);
						ncr = (ncr - floor(ncr) <= 0.5) ? floor(ncr) : ceil(ncr);

						dst(br + y, bc + x) = Vec3i{ (int)ny + 128, (int)ncb, (int)ncr };
					}
				}
			}
		}

		return dst;
	}

	static std::vector<Vec3i> Zigzag(const Mat_<Vec3i>& src)
	{
		std::vector<Vec2i> zz{};

		for (int i = 0; i < 8; ++i)
		{
			for (int j = 0; j <= i; ++j)
			{
				int r = (i % 2 == 1) ? j : i - j;
				int c = (i % 2 == 1) ? i - j : j;

				zz.push_back({ r, c });
			}
		}
		for (int i = 1; i < 8; ++i)
		{
			for (int j = 0; j < 8 - i; ++j)
			{
				int r = (i % 2 == 1) ? 8 - j - 1 : i + j;
				int c = (i % 2 == 1) ? i + j : 8 - j - 1;

				zz.push_back({ r, c });
			}
		}

		std::vector<Vec3i> res{};
		for (int br = 0; br < src.rows; br += 8)
		{
			for (int bc = 0; bc < src.cols; bc += 8)
			{
				for (int i = 0; i < QUALITY; ++i)
				{
					res.push_back(src(br + zz[i][0], bc + zz[i][1]));
				}
			}
		}

		return res;
	}

	static Mat_<Vec3i> Zigzag2(const std::vector<Vec3i>& src, int rows, int cols)
	{
		std::vector<Vec2i> zz{};

		for (int i = 0; i < 8; ++i)
		{
			for (int j = 0; j <= i; ++j)
			{
				int r = (i % 2 == 1) ? j : i - j;
				int c = (i % 2 == 1) ? i - j : j;

				zz.push_back({ r, c });
			}
		}
		for (int i = 1; i < 8; ++i)
		{
			for (int j = 0; j < 8 - i; ++j)
			{
				int r = (i % 2 == 1) ? 8 - j - 1 : i + j;
				int c = (i % 2 == 1) ? i + j : 8 - j - 1;

				zz.push_back({ r, c });
			}
		}

		Mat_<Vec3i> res(rows, cols, { 0, 0, 0 });
		for (int br = 0, k = 0; br < rows; br += 8)
		{
			for (int bc = 0; bc < cols; bc += 8)
			{
				for (int i = 0; i < QUALITY; ++i, ++k)
				{
					res(br + zz[i][0], bc + zz[i][1]) = src[k];
				}
			}
		}

		return res;
	}

	void WriteJPEG(std::vector<Vec3i> data, int rows, int cols, char *path)
	{
		std::ofstream fout(path, std::ios::binary);

		fout.write(reinterpret_cast<char *>(&rows), sizeof(rows));
		fout.write(reinterpret_cast<char *>(&cols), sizeof(cols));

		for (int i = 0; i < data.size(); ++i)
		{
			Vec3b px{ (uchar)data[i][0], (uchar)data[i][1], (uchar)data[i][2] };
			fout.write(reinterpret_cast<char *>(&px), sizeof(px));
		}

		fout.close();
	}

	static std::vector<Vec3i> ReadJPEG(char *path, int& rows, int& cols)
	{
		std::ifstream fin(path, std::ios::binary);

		fin.read(reinterpret_cast<char *>(&rows), sizeof(rows));
		fin.read(reinterpret_cast<char *>(&cols), sizeof(cols));

		int partitions = (rows / 8) * (cols / 8);
		std::vector<Vec3i> data(partitions * 64);

		for (int i = 0, k = 0; i < partitions; ++i)
		{
			for (int j = 0; j < QUALITY; ++j, ++k)
			{
				Vec3b px;
				fin.read(reinterpret_cast<char *>(&px), sizeof(px));
				data[k] = { (char)px[0], (char)px[1], (char)px[2] };
			}
		}

		return data;
	}

	void JPEGCompress(char *src, char *dst)
	{
		Mat_<Vec3b> img = imread(src), img_resize;
		resize(img, img_resize, { (int)(std::ceil(img.cols / 8.0) * 8), (int)(std::ceil(img.rows / 8.0) * 8) });
		auto img_ycbcr = RGB_to_YCbCr(img_resize);
		auto temp = ProcessImg(img_ycbcr);
		auto jpeg_data = Zigzag(temp);
		WriteJPEG(jpeg_data, temp.rows, temp.cols, dst);
	}

	void JPEGDecompress(char *src, char *dst)
	{
		int rows, cols;
		auto jpegData = ReadJPEG(src, rows, cols);
		auto temp = Zigzag2(jpegData, rows, cols);
		auto img_ycbcr = ProcessImg2(temp);
		auto img = YCbCr_to_RGB(img_ycbcr);

		imwrite(dst, img);
	}
}

