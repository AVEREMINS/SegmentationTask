#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define epsilon 40

int i, j;

typedef struct pixel {
    int x, y;
} pixel;

char* load_png_file(const char *filename, int *width, int *height) {
    unsigned char *image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    return image;
}

void colouring(unsigned char* image, int nC1, int nC2, int nC3, int oldC, int w, int h, int x, int y) {
    pixel* a = malloc(sizeof(pixel)*w * h * 4);
    long long upp = 0;
    int resultInd, nx, ny, nInd;
    int dx[] = {-1, 0, 1, 0};
    int dy[] = {0, 1, 0, -1};
    a[upp] = (pixel){x, y};
    upp+=1;
    while (upp > 0) {
        upp-=1;
        pixel tmp = a[upp];
        if (tmp.x < 0  tmp.y >= h  tmp.x >= w || tmp.y < 0) continue;
        resultInd = (tmp.y * w + tmp.x) * 4;
        if (image[resultInd] > oldC) continue;
        image[resultInd] = nC1;
        image[resultInd + 1] = nC2;
        image[resultInd + 2] = nC3;
        for (i = 0; i < 4; i++) {
            nx = tmp.x + dx[i];
            ny = tmp.y + dy[i];
            nInd = (ny * w + nx) * 4;
            if (image[nInd] <= oldC && nx >= 0 && nx < w && ny >= 0 && ny < h) {
                a[upp] = (pixel){nx, ny};
                upp+=1;
            }
        }
    }
    free(a);
}

int main() {
    int w = 0, h = 0, x, y, dy, dx, xsum, ysum, ind, grey, resultInd, mg, nC1, nC2, nC3;
    char *filename = "skull.png";
    char *image = load_png_file(filename, &w, &h);
    if (image == NULL) {
        printf("I can't read the picture %s. Error.\n", filename);
        return -1;
    }
    else{
        unsigned char *result = malloc(w * h * 4 * sizeof(unsigned char));
        int gx[3][3] = {{1, 0, -1},{2, 0, -2},{1, 0, -1}};
        int gy[3][3] = {{1,  2,  1},{0,  0,  0},{-1, -2, -1}};
        for (y = 1; y < h - 1; y++) {
            for (x = 1; x < w - 1; x++) {
                xsum = 0, ysum = 0;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        ind = (w * (y + dy) + (x + dx)) * 4;
                        grey = (image[ind] + image[ind + 1] + image[ind + 2]) / 3;
                        xsum += gx[dy + 1][dx + 1] * grey;
                        ysum += gy[dy + 1][dx + 1] * grey;
                    }
                }
                mg = sqrt(xsum * xsum + ysum * ysum);
                mg = mg > 255 ? 255 : mg;
                mg = mg < 0 ? 0 : mg;
                resultInd = (y * w + x) * 4;
                for(i = 0; i < 3; i++) result[resultInd + i] = mg;
                result[resultInd + 3] = image[resultInd + 3];
            }
        }
        for (i = 0; i < w * h * 4; i++) image[i] = result[i];
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                int ind = (i * w + j) * 4;
                if (image[ind] < epsilon) {
                    nC1 = rand() % (255 - epsilon*10) + epsilon * 2;
                    nC2 = rand() % (255 - epsilon*10) + epsilon * 2;
                    nC3 = rand() % (255 - epsilon*10) + epsilon * 2;
                    colouring(image, nC1, nC2, nC3, epsilon, w, h, j, i);
                }
            }
        }
        char *output_filename = "skull_filter_applied.png";
        lodepng_encode32_file(output_filename, image, w, h);
        unsigned error = lodepng_encode32_file(output_filename, image, w, h);
        if (error) {
            printf("error %u: %s\n", error, lodepng_error_text(error));
            return -1;
        }
        free(result);
    }
    free(image);
    return 0;
}
