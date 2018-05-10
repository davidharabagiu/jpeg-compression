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

	static std::vector<std::vector<Mat_<Vec3i>>> Partition(const Mat_<Vec3i>& img)
	{
		int pr = img.rows / 8;
		int pc = img.cols / 8;
		if (img.rows % 8 != 0)
			++pr;
		if (img.cols % 8 != 0)
			++pc;

		std::vector<std::vector<Mat_<Vec3i>>> partitions{};
		for (int pi = 0; pi < pr; ++pi)
		{
			partitions.push_back(std::vector<Mat_<Vec3i>>{});
			for (int pj = 0; pj < pc; ++pj)
			{
				partitions[pi].push_back(Mat_<Vec3i>(8, 8, Vec3i{}));
				for (int i = 0; i < 8; ++i)
				{
					for (int j = 0; j < 8; ++j)
					{
						if (i + pi * 8 >= img.rows || j + pj * 8 >= img.cols)
						{
							continue;
						}
						partitions[pi][pj](i, j) = img(i + pi * 8,j + pj * 8);
					}
				}
			}
		}

		return partitions;
	}

	static Mat_<Vec3i> dct(const Mat_<Vec3i>& src)
	{
		Mat_<Vec3i> dst(src.rows, src.cols);
		double ny, ncb, ncr, ci, cj;

		for (int i = 0; i < src.rows; ++i)
		{
			for (int j = 0; j < src.cols; ++j)
			{
				ny = ncb = ncr = 0.0;
				for (int x = 0; x < 8; ++x)
				{
					for (int y = 0; y < 8; ++y)
					{
						ny += (src(y, x)[0] - 128) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
						ncb += (src(y, x)[1]) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
						ncr += (src(y, x)[2]) * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
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

				dst(i, j) = Vec3i{ (int)ny, (int)ncb, (int)ncr };
			}
		}

		return dst;
	}

	static Mat_<Vec3i> idct(const Mat_<Vec3i>& src)
	{
		const double alpha0 = 1 / sqrt(2.0);

		Mat_<Vec3i> dst(src.rows, src.cols);
		double ny, ncb, ncr, ci, cj, cy, ccb, ccr;

		for (int x = 0; x < src.rows; ++x)
		{
			for (int y = 0; y < src.cols; ++y)
			{
				ny = ncb = ncr = 0.0;
				for (int i = 0; i < 8; ++i)
				{
					for (int j = 0; j < 8; ++j)
					{
						ci = std::sqrt(((i == 0) ? 1.0 : 2.0) / 8);
						cj = std::sqrt(((j == 0) ? 1.0 : 2.0) / 8);

						cy = src(i, j)[0] * ql[i][j];
						ccb = src(i, j)[1] * qc[i][j];
						ccr = src(i, j)[2] * qc[i][j];

						ny += ci * cj * cy * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
						ncb += ci * cj * ccb * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
						ncr += ci * cj * ccr * std::cos(((2 * y + 1) * i * PI) / 16) * std::cos(((2 * x + 1) * j * PI) / 16);
					}
				}

				ny = (ny - floor(ny) <= 0.5) ? floor(ny) : ceil(ny);
				ncb = (ncb - floor(ncb) <= 0.5) ? floor(ncb) : ceil(ncb);
				ncr = (ncr - floor(ncr) <= 0.5) ? floor(ncr) : ceil(ncr);

				dst(x, y) = Vec3i{ (int)ny + 128, (int)ncb, (int)ncr };
			}
		}

		return dst;
	}

	static std::vector<std::vector<Mat_<Vec3i>>> Quantize(const std::vector<std::vector<Mat_<Vec3i>>>& src)
	{
		std::vector<std::vector<Mat_<Vec3i>>> dst{};

		for (int i = 0; i < src.size(); ++i)
		{
			dst.push_back(std::vector<Mat_<Vec3i>>{});
			for (int j = 0; j < src[i].size(); ++j)
			{
				dst[i].push_back(dct(src[i][j]));
			}
		}

		return dst;
	}

	static std::vector<Mat_<Vec3i>> Dequantize(const std::vector<Mat_<Vec3i>>& src)
	{
		std::vector<Mat_<Vec3i>> dst{};

		for (int i = 0; i < src.size(); ++i)
		{
			dst.push_back(idct(src[i]));
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
		for (int i = 0; i < 16; ++i)
		{
			res.push_back(src(zz[i][0], zz[i][1]));
		}

		return res;
	}

	static Mat_<Vec3i> Zigzag2(const std::vector<Vec3i>& src)
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

		Mat_<Vec3i> res(8, 8, { 0, 0, 0 });
		for (int i = 0; i < 16; ++i)
		{
			res(zz[i][0], zz[i][1]) = src[i];
		}

		return res;
	}

	void WriteJPEG(std::vector<std::vector<Vec3i>> data, int width, int height, char *path)
	{
		std::ofstream fout(path, std::ios::binary);

		fout.write(reinterpret_cast<char *>(&width), sizeof(width));
		fout.write(reinterpret_cast<char *>(&height), sizeof(height));

		for (int i = 0; i < data.size(); ++i)
		{
			auto& p = data[i];
			for (int k = 0; k < p.size(); ++k)
			{
				Vec3i px = p[k];
				Vec3b px_packed{ (uchar)px[0], (uchar)px[1], (uchar)px[2] };
				fout.write(reinterpret_cast<char *>(&px_packed), sizeof(px_packed));
			}
		}

		fout.close();
	}

	std::vector<std::vector<Vec3i>> ReadJPEG(char *path, int& width, int& height)
	{
		std::ifstream fin(path, std::ios::binary);

		fin.read(reinterpret_cast<char *>(&width), sizeof(width));
		fin.read(reinterpret_cast<char *>(&height), sizeof(height));

		std::vector<std::vector<Vec3i>> data{};

		int partitions = (width / 8) * (height / 8);
		for (int i = 0; i < partitions; ++i)
		{
			data.push_back(std::vector<Vec3i>{});
			for (int j = 0; j < 16; ++j)
			{
				Vec3b px;
				fin.read(reinterpret_cast<char *>(&px), sizeof(px));
				data[i].push_back({ (char)px[0], (char)px[1], (char)px[2] });
			}
		}

		return data;
	}

	template<typename T>
	void print(Mat_<T>& img)
	{
		for (int i = 0; i < img.rows; ++i)
		{
			for (int j = 0; j < img.cols; ++j)
			{
				std::cout << img(i, j) << " ";
			}
			std::cout << "\n";
		}
		std::cout.flush();

		std::cin.get();
		std::cin.get();
	}

	void JPEGCompress(Mat_<Vec3b>& img, char *path)
	{
		auto img_ycbcr = RGB_to_YCbCr(img);

		auto partitions = Partition(img_ycbcr);

		auto partitions_quantized = Quantize(partitions);

		std::vector<std::vector<Vec3i>> data{};
		for (int i = 0; i < partitions_quantized.size(); ++i)
		{
			auto& row = partitions_quantized[i];
			for (int j = 0; j < row.size(); ++j)
			{
				data.push_back(Zigzag(row[j]));
			}
		}

		WriteJPEG(data, partitions_quantized.size() * 8, partitions_quantized[0].size() * 8, "test.bin");

		JPEGDecompress("test.bin");
	}

	void JPEGDecompress(char *path)
	{
		int width, height;
		auto jpegData = ReadJPEG(path, width, height);

		std::vector<Mat_<Vec3i>> partitions_quantized{};
		for (auto part : jpegData)
		{
			partitions_quantized.push_back(Zigzag2(part));
		}

		auto partitions = Dequantize(partitions_quantized);
		
		Mat_<Vec3i> img_ycbcr(width, height);
		for (int i = 0, k = 0; i < width / 8; ++i)
		{
			for (int j = 0; j < height / 8; ++j, ++k)
			{
				for (int _i = 0; _i < 8; ++_i)
				{
					for (int _j = 0; _j < 8; ++_j)
					{
						img_ycbcr(i * 8 + _i, j * 8 + _j) = partitions[k](_i, _j);
					}
				}
			}
		}

		auto img = YCbCr_to_RGB(img_ycbcr);

		imshow("doamne apara si pazeste", img);
		waitKey(0);
	}
}

