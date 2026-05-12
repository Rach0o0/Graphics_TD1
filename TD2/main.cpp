#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
notes from the lecture notes 

We want to get a good color palette in an image 
Method to transfer the color palette from one image to another

Model image : source
Input image : image we want to transform

Sliced optimal transport matching
Consider optimal transport problem
Same nb of pixels
--> linear assignment problem

Match 2 point clouds of pixels (RGB)
--> move pixels of the input image towards their asigned pixel in the model image

Iteratively : 
1- find uniformly random direction on the sphere
r1, r2 ~ U(0,1)
x = cos(2pir1)sqrt(r2(1-r2))
y = sin(2pi r1)sqrt(r2(1-r2))
z = 1 - 2r2

2- project input and model point clouds onto this direction 
compute dor product btwn random dir and pixel coord

3- optimal matching 
matching first projected point of the input point cloud
to the first projected point of the model point cloud
the second with the second ...

--> sort 2 projected point clouds according to dot product computed, keeping pixel index
std::pair<>, std::sort 

4- advect points of the input point cloud in the randomly chosen direction  by the 
projected distance to its matched point in the model image

Iterate with newly chosen random dir

*/

/*
What we need 
- pi
- vec3
- dot product
- get a random direction
*/
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323856
#endif

struct Vec3 {
    double r, g, b;
};

double dot(const Vec3& a, const Vec3& b) {
    return a.r * b.r + a.g * b.g + a.b * b.b;
}

Vec3 random_direction() {
    static std::default_random_engine engine;
    static std::uniform_real_distribution<double> uniform(0.0, 1.0);

    double r1 = uniform(engine);
    double r2 = uniform(engine);

    double x = std::cos(2.0 * M_PI * r1) * 2.0 * std::sqrt(r2 * (1.0 - r2));
    double y = std::sin(2.0 * M_PI * r1) * 2.0 * std::sqrt(r2 * (1.0 - r2));
    double z = 1.0 - 2.0 * r2;

    return {x, y, z};
}

double clamp(double x) {
    if (x < 0.0) return 0.0;
    if (x > 255.0) return 255.0;
    return x;
}


int main() {

	int W1, H1, C1;
	int W2, H2, C2;
	
	/*
	//stbi_set_flip_vertically_on_load(true);
	unsigned char *image = stbi_load("8733654151_b9422bb2ec_k.jpg",
                                 &W,
                                 &H,
                                 &C,
                                 STBI_rgb);
	std::vector<double> image_double(W*H*3);
	for (int i=0; i<W*H*3; i++)
		image_double[i] = image[i];
	
	std::vector<unsigned char> image_result(W*H * 3, 0);
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {

			image_result[(i*W + j) * 3 + 0] = image_double[(i*W+j)*3+0]*0.5;
			image_result[(i*W + j) * 3 + 1] = image_double[(i*W+j)*3+1]*0.3;
			image_result[(i*W + j) * 3 + 2] = image_double[(i*W+j)*3+2]*0.2;
		}
	}
	*/

	unsigned char* input = stbi_load("redim.jpg", &W1, &H1, &C1, STBI_rgb);
    unsigned char* model = stbi_load("imgA.jpg", &W2, &H2, &C2, STBI_rgb);

	//different dimensions
	if (W1 != W2 || H1 != H2) {
        std::cout << "error dimensions" ;
        return 1;
    }

	int W = W1;
	int H = H1;
	int n = W * H;

	//input points
	std::vector<Vec3> I(n);

	//model points
	std::vector<Vec3> M(n);

	//store points in lists 
	for (int i = 0; i < n; i++) {
        I[i] = {(double)input[3 * i + 0],(double)input[3 * i + 1],(double)input[3 * i + 2]};

        M[i] = {(double)model[3 * i + 0],(double)model[3 * i + 1],(double)model[3 * i + 2]};
    }

	int nbiter = 100;
	for (int iter = 0; iter < nbiter; iter++){
		Vec3 v = random_direction();

		//project. we store the dot product and pixel index as a pair of values
		std::vector<std::pair<double, int>> projI(n);
		std::vector<std::pair<double, int>> projM(n);

		for (int i = 0; i < n; i++){ // for each pixel
			projI[i] = {dot(I[i], v), i};
			projM[i] = {dot(M[i], v), i};
		}

		//Sort according to the dot product
		std::sort(projI.begin(), projI.end());
		std::sort(projM.begin(), projM.end());

		//Advect initial point cloud
		for (int i = 0; i< n; i++){
			I[projI[i].second].r = (projM[i].first - projI[i].first) * v.r;
			I[projI[i].second].g = (projM[i].first - projI[i].first) * v.g;
			I[projI[i].second].b = (projM[i].first - projI[i].first) * v.b;
		}

	}

	//result
	std::vector<unsigned char> final_result(n * 3, 0);

    for (int i = 0; i < n; i++) {
        final_result[3 * i + 0] = (unsigned char)clamp(I[i].r);
        final_result[3 * i + 1] = (unsigned char)clamp(I[i].g);
        final_result[3 * i + 2] = (unsigned char)clamp(I[i].b);
    }

    stbi_write_png("image.png", W, H, 3, final_result.data(), 0);

	return 0;
}

/*
#define _CRT_SECURE_NO_WARNINGS 1

#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323856
#endif

struct Vec3 {
    double r, g, b;
};

double dot(const Vec3& a, const Vec3& b) {
    return a.r * b.r + a.g * b.g + a.b * b.b;
}

double clamp(double x) {
    if (x < 0.0) return 0.0;
    if (x > 255.0) return 255.0;
    return x;
}

Vec3 random_direction() {
    static std::default_random_engine engine;
    static std::uniform_real_distribution<double> uniform(0.0, 1.0);

    double r1 = uniform(engine);
    double r2 = uniform(engine);

    double x = std::cos(2.0 * M_PI * r1) * 2.0 * std::sqrt(r2 * (1.0 - r2));
    double y = std::sin(2.0 * M_PI * r1) * 2.0 * std::sqrt(r2 * (1.0 - r2));
    double z = 1.0 - 2.0 * r2;

    return { x, y, z };
}

int main() {

    int W1, H1, C1;
    int W2, H2, C2;

    unsigned char* input = stbi_load("redim.jpg", &W1, &H1, &C1, STBI_rgb);
    unsigned char* model = stbi_load("imgA.jpg", &W2, &H2, &C2, STBI_rgb);

    if (!input || !model) {
        std::cout << "error loading images" << std::endl;
        return 1;
    }

    if (W1 != W2 || H1 != H2) {
        std::cout << "error dimensions" << std::endl;
        return 1;
    }

    int W = W1;
    int H = H1;
    int n = W * H;

    std::vector<Vec3> I(n);
    std::vector<Vec3> M(n);

    for (int i = 0; i < n; i++) {
        I[i] = {
            (double)input[3 * i + 0],
            (double)input[3 * i + 1],
            (double)input[3 * i + 2]
        };

        M[i] = {
            (double)model[3 * i + 0],
            (double)model[3 * i + 1],
            (double)model[3 * i + 2]
        };
    }

    int nbiter = 100;

    for (int iter = 0; iter < nbiter; iter++) {
        Vec3 v = random_direction();

        std::vector<std::pair<double, int>> projI(n);
        std::vector<std::pair<double, int>> projM(n);

        for (int i = 0; i < n; i++) {
            projI[i] = { dot(I[i], v), i };
            projM[i] = { dot(M[i], v), i };
        }

        std::sort(projI.begin(), projI.end());
        std::sort(projM.begin(), projM.end());

        for (int i = 0; i < n; i++) {
            int indexI = projI[i].second;

            double diff = projM[i].first - projI[i].first;

            I[indexI].r += diff * v.r;
            I[indexI].g += diff * v.g;
            I[indexI].b += diff * v.b;
        }

        std::cout << "iteration " << iter + 1 << " / " << nbiter << std::endl;
    }

    std::vector<unsigned char> final_result(n * 3, 0);

    for (int i = 0; i < n; i++) {
        final_result[3 * i + 0] = (unsigned char)clamp(I[i].r);
        final_result[3 * i + 1] = (unsigned char)clamp(I[i].g);
        final_result[3 * i + 2] = (unsigned char)clamp(I[i].b);
    }

    stbi_write_png("image.png", W, H, 3, final_result.data(), 0);

    stbi_image_free(input);
    stbi_image_free(model);

    return 0;
}
*/