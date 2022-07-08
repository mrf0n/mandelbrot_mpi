#include <iostream>
#include "C:\Users\Ваня\Desktop\openMP\mpi лаба2\gif.h"
#include "mpi.h"
#include <vector>
using namespace std;

const int width = 768;
const int height = 512;
const int shots = 48;
//uint8_t image[width * height * 4];


void SetPixel(int xx, int yy, uint8_t red, uint8_t grn, uint8_t blu, uint8_t* image)
{
	uint8_t* pixel = &image[(yy * width + xx) * 4];
	pixel[0] = red;
	pixel[1] = blu;
	pixel[2] = grn;
	pixel[3] = 255;  // no alpha for this demo
}

void SetPixelFloat(int xx, int yy, float fred, float fgrn, float fblu, uint8_t* image)
{
	// convert float to unorm
	uint8_t red = (uint8_t)roundf(255.0f * fred);
	uint8_t grn = (uint8_t)roundf(255.0f * fgrn);
	uint8_t blu = (uint8_t)roundf(255.0f * fblu);

	SetPixel(xx, yy, red, grn, blu, image);
}

bool mandelbrot(float imC, float reC, int& i)
{
	const int maxIter = 80;
	float d = 0.0f;
	float imZ = 0.0f;
	float reZ = 0.0f;
	for (i = 0; i < maxIter && d <= 2.0f; ++i)
	{
		float im = imZ * imZ - reZ * reZ + imC;
		float re = 2.0f * imZ * reZ + reC;
		imZ = im;
		reZ = re;
		d = sqrt(imZ * imZ + reZ * reZ);
	}
	return	d < 2;
}

void shot(float x, float y, uint8_t* pixel)
{
	int	i;
	if (mandelbrot(x, y, i))
	{
		pixel[0] = pixel[1] = pixel[2] = 0x20;
		pixel[3] = 0xFF;
	}
	else
	{
		pixel[0] = i * 3 % 255;
		pixel[1] = i * 4 % 255;
		pixel[2] = i * 5 % 255;
		pixel[3] = 0xFF;
	}
}

uint8_t* seting(int i, uint8_t* image)
{
	for (int j = 0; j < height; j++)
	{
		for (int z = 0; z < width; z++)
		{
			float x = (-2 + z * (3.0 / width)) * pow(0.95, i) - 0.6;
			float y = (1 - j * (2.0 / height)) * pow(0.95, i) + 0.5;
			uint8_t* pixel = new uint8_t[4];
			shot(x, y, pixel);

			float r = pixel[0] / 255.0;
			float g = pixel[1] / 255.0;
			float b = pixel[2] / 255.0;

			SetPixelFloat(z, j, r, g, b, image);
		}
	}
	return image;
}

void main_content(int rank, int size, GifWriter writer, uint8_t* image_par)
{
	if (rank != 0)
	{
		MPI_Send(image_par, width * height * 4 * (shots / size), MPI_UINT8_T, 0, 0, MPI_COMM_WORLD);
	}
	if (rank == 0)
	{

		uint8_t* buf = new uint8_t[width * height * 4 * (shots / size)];
		for (int i = 1; i < size; i++)
		{
			MPI_Recv(buf, width * height * 4 * (shots / size), MPI_UINT8_T, i, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			for (int i = 0; i < shots / size; i++)
			{
				GifWriteFrame(&writer, buf + height * width * 4 * i, width, height, 2, 8, false);
			}
		}
		for (int i = 0; i < shots / size; i++)
		{
			GifWriteFrame(&writer, &image_par[width * height * 4 * i], width, height, 2, 8, false);
		}
		delete[] buf;
		GifEnd(&writer);
	}
}


int main(const int argc, char* argv[])
{
	setlocale(LC_ALL, "ru");

	//непараллельный вариант без MPI(чистый код)
	//GifWriter writer = {};
	//GifBegin(&writer, "gif.gif", height, width, 5, 8, true);

	int index;
	//for (int i = 0; i < shots; i++)
	//{

	//	float r, g, b;
	//	for (int j = 0; j < height; j++)
	//	{
	//		for (int z = 0; z < width; z++)
	//		{
	//			if (mandelbrot((-2+z*(3.0/width)) * pow(0.95,i) - 0.5, (1-j*(2.0/height))* pow(0.95, i) + 0.5, index))
	//			{
	//				r = g = b = 32;
	//			}
	//			else
	//			{
	//				r = index * 3 % 255;
	//				g = index * 4 % 255;
	//				b = index * 5 % 255;

	//			}
	//			SetPixel(z, j, r, g, b);
	//		}
	//	}
	//	GifWriteFrame(&write, image, width, height, 2, 8, true);
	//}
	//GifEnd(&write);
	//

	//параллельный

	MPI_Init(&argc, &argv);
	int rank, size, k; double time;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0)
	{
		time = MPI_Wtime();
	}

	int startindex = shots / size * rank;
	int endindex = shots / size * (rank + 1);
	uint8_t* image_par = new uint8_t[(width * height * 4 * (shots / size))];
	for (int i = startindex; i < endindex; i++)
	{
		cout << i + 1 << endl;
		//printf("%d", i + 1);
		seting(i, &image_par[width * height * 4 * (i - startindex)]);
	}
	GifWriter writer = {};
	GifBegin(&writer, "C:\\Users\\Ваня\\source\\repos\\mpi\\mpi\\gif.gif", height, width, 2, 8, true);

	main_content(rank, size, writer, image_par);
	delete[] image_par;
	if (rank == 0)
		printf("%f", MPI_Wtime() - time);
	MPI_Finalize();
	return 0;
}
