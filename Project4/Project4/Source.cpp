#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <iostream>
#include <numbers>
#include <math.h>


using namespace cv;
using namespace std;
using std::pow;
using std::begin;
using std::end;


// FUNCI?N PARA EL KERNEL
float** kernel_filled(int kernel_size, float sigma)
{

	// VARIABLES AUXILIARES PARA EL C?LCULO DE LOS VALORES DEL KERNEL
	float** kernel = new float* [kernel_size];
	for (int i = 0; i < kernel_size; i++)
		kernel[i] = new float[kernel_size];
	int i, j, x, y;
	int border_kernel = -(kernel_size - 1) / 2;
	double pi = 3.141592653589793;
	double euler = 2.718281828459045235360;
	float g_x_y;


	// ASIGNAR LOS VALORES AL KERNEL
	for (i = 0; i < kernel_size; i++) {
		for (j = 0; j < kernel_size; j++) {

			x = border_kernel + i;
			y = border_kernel + j;

			g_x_y = (1 / (2 * pi * sigma * sigma)) * pow(euler, -((x * x) + (y * y)) / (2 * sigma * sigma));

			kernel[i][j] = g_x_y;
			cout << x << ", " << y << ": " << g_x_y << "\n";
		}
	}
	return kernel;
}


// FUNCI?N PARA APLICAR BORDES
Mat bordered_image(Mat image_gray, int kernel_size) {

	// DECLARAMOS LA MATRIZ CON BORDES
	int rows = image_gray.rows;
	int columns = image_gray.cols;
	int new_img_rows = rows + kernel_size - 1;
	int new_img_columns = columns + kernel_size - 1;
	Mat bord_image(new_img_rows, new_img_columns, CV_8UC1);


	// ASIGNAR VALORES DE 0 A LA NUEVA MATRIZ
	for (int i = 0; i < new_img_rows; i++) {
		for (int j = 0; j < new_img_columns; j++) {
			bord_image.at<uchar>(Point(j, i)) = uchar(0);
		}
	}


	// LLENAR LA NUEVA IMAGEN CON LA IMAGEN ORIGINAL
	int aux = (kernel_size - 1) / 2;
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			bord_image.at<uchar>(Point(j + aux, i + aux)) = image_gray.at<uchar>(Point(j, i));
		}
	}

	return bord_image;
}


// FUNCI?N PARA APLICAR EL FILTRO
Mat convolution(float** kernel, int kernel_size, Mat bord_image)
{
	int i, j, k, l;
	int rows = bord_image.rows - kernel_size + 1;
	int columns = bord_image.cols - kernel_size + 1;
	Mat gauss_image(rows, columns, CV_8UC1);

	// APLICAMOS EL FILTRO
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++) {

			float convolution_value = 0;

			for (k = 0; k < kernel_size; k++) {
				for (l = 0; l < kernel_size; l++) {
					convolution_value += kernel[k][l] * bord_image.at<uchar>(Point(j + l, i + k));
				}
			}
			gauss_image.at<uchar>(Point(j, i)) = convolution_value;
		}
	}
	return gauss_image;
}


// FUNCI?N PARA CALCULAR HISTOGRAMA
unsigned long int* histogram(Mat gauss_image) {
	
	unsigned long int h[256];
	int rows = gauss_image.rows;
	int columns = gauss_image.cols;
	int aux;
	
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			aux = gauss_image.at<uchar>(Point(i, j));
			h[aux] = h[aux] + 1;
		}
	}

	return h;
}


// FUNCI?N PARA ECUALIZAR IMAGEN
Mat equalize_image(Mat gauss_image, unsigned long int* histogram) {

	// VARIABLES AUXILIARES
	int sum = 0;
	double c = 0;
	int accumulated_sum[256];
	int original_pixel_val = 0;
	int new_pixel_val = 0;
	

	// MATRIZ PARA LA IMAGEN ECUALIZADA
	int rows = gauss_image.rows;
	int columns = gauss_image.cols;
	Mat equalized_image(rows, columns, CV_8UC1);


	// CALCULAMOS LOS VALORES DE LA SUMA ACUMULADA
	for (int i = 0; i < 256; i++) {
		sum += (histogram[i] / (rows * columns));
		// cout << i << ": " << sum << "\n";
		accumulated_sum[i] = sum;
		//cout << accumulated_sum << "\n";
	}


	// CALCULAMOS EL VALOR DE LA CONSTANTE: LA SUMA ACUMULADA ENTRE LA CANTIDAD DE PIXELES QUE TIENE LA IMAGEN
	c = (double)(sum) / (double)(rows * columns);
	//cout << "c: " << c << "\n";
	//cout << "sum: " << sum << "\n";
	//cout << "rows * columns: " << rows * columns << "\n";

	int max = accumulated_sum[255] * c;

	// OBTENEMOS LOS VALORES DE LA IMAGEN ECUALIZADA
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			original_pixel_val = gauss_image.at<uchar>(Point(i, j));
			new_pixel_val = (((float)(accumulated_sum[original_pixel_val] * c)) / max) * 254;
			//cout << "original_pixel_val: " << original_pixel_val << "\n";
			if (new_pixel_val > 255) {
				new_pixel_val = 255;
			}
			equalized_image.at<uchar>(Point(i, j)) = new_pixel_val;
		}
	}
	
	return equalized_image;
}


// FUNCI?N PARA CALCULAR G_X
Mat G_X_kernel(Mat equalized_image) {
	// M?SCARA G_X
	float** g_x = new float* [3];
	for (int i = 0; i < 3; i++)
		g_x[i] = new float[3];
	g_x[0][0] = -1;
	g_x[0][1] = 0;
	g_x[0][2] = 1;
	g_x[1][0] = -2;
	g_x[1][1] = 0;
	g_x[1][2] = 2;
	g_x[2][0] = -1;
	g_x[2][1] = 0;
	g_x[2][2] = 1;

	Mat G_X = convolution(g_x, 3, equalized_image);

	return G_X;

}


// FUNCI?N PARA CALCULAR G_Y
Mat G_Y_kernel(Mat equalized_image) {

	// M?SCARA G_Y
	float** g_y = new float* [3];
	for (int i = 0; i < 3; i++)
		g_y[i] = new float[3];
	g_y[0][0] = -1;
	g_y[0][1] = -2;
	g_y[0][2] = -1;
	g_y[1][0] = 0;
	g_y[1][1] = 0;
	g_y[1][2] = 0;
	g_y[2][0] = 1;
	g_y[2][1] = 2;
	g_y[2][2] = 1;


	Mat G_Y = convolution(g_y, 3, equalized_image);

	return G_Y;

}



// FUNCI?N PARA ACPLICAR EL FILTRO DE SOBEL A LA IMAGEN ECUALIZADA
Mat sobel(Mat equalized_image) {

	int rows = equalized_image.rows - 2;
	int columns = equalized_image.cols - 2;

	// DECLARAMOS LA MATRIZ PARA G_XY
	Mat G_XY(rows, columns, CV_8UC1);	
	Mat G_X = G_X_kernel(equalized_image);
	Mat G_Y = G_X_kernel(equalized_image);


	// VARIABLES AUXILIARES
	int val_pix_x = 0;
	int val_pix_y = 0;

	// CALCULAMOS LOS VALORES DE G_XY
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			val_pix_x = G_X.at<uchar>(Point(i, j));
			val_pix_y = G_Y.at<uchar>(Point(i, j));
			G_XY.at<uchar>(Point(i, j)) = sqrt(pow(val_pix_x, 2) + pow(val_pix_y, 2));
		}
	}

	return G_XY;
}


// FUNCI?N PARA DETECCI?N DE BORDE DE CANNY
Mat canny(Mat sobel_image, Mat equalized_image) {
	int rows = sobel_image.rows;
	int columns = sobel_image.cols;

	// DECLARAMOS LA MATRIZ CANY
	Mat canny_image(rows, columns, CV_8UC1);

	// DECLARAMOS LA MATRIZ PARA EL ?NGULO
	Mat angle(rows, columns, CV_8UC1);

	Mat G_X = G_X_kernel(equalized_image);
	Mat G_Y = G_X_kernel(equalized_image);

	// CALCULAMOS LOS ?NGULOS
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			
			angle.at<uchar>(Point(i, j)) = atan((G_Y.at<uchar>(Point(i, j))) / (G_X.at<uchar>(Point(i, j))+1));
		}
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {

			canny_image.at<uchar>(Point(i, j)) = angle.at<uchar>(Point(i, j)) * sobel_image.at<uchar>(Point(i, j));
		}
	}


	// FUNCI?N CANNY DE LA LIBRER?A
	int min_threshold = 150;
	int max_threshold = 200;
	Canny(sobel_image, canny_image, min_threshold, max_threshold);
	return canny_image;
}


// FUNCI?N PARA MOSTRAR IM?GENES Y GUARDARLAS
void show_images(Mat image, Mat image_gray, Mat bordered_image, Mat gauss_image, Mat equalized_image, Mat sobel_image, Mat canny_image) {

	// IMAGEN ORIGINAL
	namedWindow("Imagen Original", WINDOW_AUTOSIZE);
	imshow("Imagen Original", image);

	// IMAGEN EN ESCALA DE GRISES
	namedWindow("Imagen en escala de grises", WINDOW_AUTOSIZE);
	imshow("Imagen en escala de grises", image_gray);

	// IMAGEN CON BORDES
	namedWindow("Imagen con bordes", WINDOW_AUTOSIZE);
	imshow("Imagen con bordes", bordered_image);

	// FILTRO GAUSS
	namedWindow("Imagen filtrada", WINDOW_AUTOSIZE);
	imshow("Imagen filtrada", gauss_image);

	// IMAGEN ECUALIZADA
	namedWindow("Imagen ecualizada", WINDOW_AUTOSIZE);
	imshow("Imagen ecualizada", equalized_image);

	// IMAGEN CON EL FILTRO DE SOBEL
	namedWindow("Imagen con el filtro de Sobel", WINDOW_AUTOSIZE);
	imshow("Imagen con el filtro de Sobel", sobel_image);

	// IMAGEN CON DETECCI?N DE BORDES CANNY
	namedWindow("Imagen con detecci?n de bordes", WINDOW_AUTOSIZE);
	imshow("Imagen con detecci?n de bordes", canny_image);


	// GUARDAMOS LAS IM?GENES
	imwrite("img_original.png", image);
	imwrite("img_gray.png", image_gray);
	imwrite("img_borders.png", bordered_image);
	imwrite("img_gauss.png", gauss_image);
	imwrite("img_equalized.png", equalized_image);
	imwrite("img_sobel.png", sobel_image);
	imwrite("img_canny.png", canny_image);


	// MOSTRAR EL TAMA?O DE LAS IM?GENES
	cout << "\n\nImagen original\nrows: " << image.rows << "\ncolumns: " << image.cols << "\n\nImagen con bordes: \nrows: " << bordered_image.rows << "\ncolumns:" << bordered_image.cols << "\n\nImagen filtrada\nrows: " << gauss_image.rows << "\ncolumns: " << gauss_image.cols << "\n\n";

}


int main() {
	char imageName[] = "lena.jpg";
	Mat image;


	// ABRIR LA IMAGEN
	image = imread(imageName);
	if (!image.data) {
		cout << "Error al cargar la imagen: " << imageName << endl;
		exit(1);
	}


	// GUARDAR LOS VALORES DE LARGO Y ANCHO DE LA IMAGEN (FILAS Y COLUMNAS)
	int rows = image.rows;
	int columns = image.cols;


	// CONVERTIMOS LA IMAGEN A ESCALA DE GRISES
	Mat image_gray(rows, columns, CV_8UC1);
	cvtColor(image, image_gray, COLOR_BGR2GRAY);


	// Ingresar el tama?o del kernel
	int kernel_size;
	cout << "Ingrese el tama?o del kernel: \n";
	cin >> kernel_size;


	// Ingresar el valor de sigma
	int sigma;
	cout << "Ingrese el valor de sigma: \n";
	cin >> sigma;


	// KERNEL
	float** kernel;
	kernel = kernel_filled(kernel_size, sigma);


	// MATRIZ CON BORDES
	Mat bord_image = bordered_image(image_gray, kernel_size);


	// APLICAR FILTRO GAUSS
	Mat gauss_image = convolution(kernel, kernel_size, bord_image);


	// HISTOGRAMA
	unsigned long int * histograma = histogram(gauss_image);


	// IMAGEN ECUALIZADA
	Mat equalized_image = equalize_image(gauss_image, histograma);


	// IMAGEN CON EL FILTRO DE SOBEL
	Mat sobel_image = sobel(equalized_image);


	// IMAGEN CON DETECCI?N DE BORDES CANNY
	Mat canny_image = canny(sobel_image, equalized_image);


	// MOSTRAR LAS IM?GENES EN NUEVAS VENTANAS
	show_images(image, image_gray, bord_image, gauss_image, equalized_image, sobel_image, canny_image);


	waitKey(0);
	return 1;
}