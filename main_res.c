#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lodepng.h"

typedef struct Node {
    unsigned char r, g, b, w;
    struct Node *up, *down, *lt, *rt, *p;
    int rank;
} Node;

typedef struct point {
    int x, y;
} point;

char* load_png_file(const char *filename, int *width, int *height) {
    unsigned char *image = NULL;
    int err = lodepng_decode32_file(&image, width, height, filename);
    if (err) {
        printf("PNG error %u: %s\n", err, lodepng_error_text(err));
        return NULL;
    }
    return (char*)image;
}

Node* create_graph(unsigned char *img, int w, int h) {
    Node *grid = (Node*)malloc(w * h * sizeof(Node));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Node *curr = &grid[y * w + x];
            unsigned char *px = &img[(y * w + x) * 4];
            curr->r = px[0];
            curr->g = px[1];
            curr->b = px[2];
            curr->w = px[3];
            curr->up = (y > 0) ? &grid[(y-1)*w + x] : NULL;
            curr->down = (y < h-1) ? &grid[(y+1)*w + x] : NULL;
            curr->lt = (x > 0) ? &grid[y*w + (x-1)] : NULL;
            curr->rt = (x < w-1) ? &grid[y*w + (x+1)] : NULL;
            curr->p = curr;
            curr->rank = 0;
        }
    }
    return grid;
}

Node* find_set(Node *x) {
    while (x->p != x) {
        x->p = x->p->p;
        x = x->p;
    }
    return x;
}

void union_set(Node *a, Node *b, double eps) {
    if (a->r < 40 && b->r < 40) return;
    
    Node *root_a = find_set(a);
    Node *root_b = find_set(b);
    
    double diff = sqrt((a->r-b->r)*(a->r-b->r) + 
                 (a->g-b->g)*(a->g-b->g) + 
                 (a->b-b->b)*(a->b-b->b));
    
    if (root_a != root_b && diff < eps) {
        if (root_a->rank > root_b->rank) {
            root_b->p = root_a;
        } else {
            root_a->p = root_b;
            if (root_a->rank == root_b->rank)
                root_b->rank++;
        }
    }
}

void apply_filter(unsigned char *img, int w, int h) {
    int sobel_x[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    int sobel_y[3][3] = {{1,2,1},{0,0,0},{-1,-2,-1}};
    unsigned char *buf = (unsigned char*)malloc(w*h*4);
    
    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {
            int gx = 0, gy = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int idx = ((y+dy)*w + (x+dx)) * 4;
                    int gray = (img[idx]+img[idx+1]+img[idx+2])/3;
                    gx += sobel_x[dy+1][dx+1] * gray;
                    gy += sobel_y[dy+1][dx+1] * gray;
                }
            }
            int mag = sqrt(gx*gx + gy*gy);
            mag = (mag > 255) ? 255 : mag;
            int pos = (y*w + x) * 4;
            buf[pos] = buf[pos+1] = buf[pos+2] = mag;
            buf[pos+3] = img[pos+3];
        }
    }
    
    for (int i = 0; i < w*h*4; i++) img[i] = buf[i];
    free(buf);
}

void find_components(Node *grid, int w, int h, double eps) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Node *curr = &grid[y*w + x];
            if (curr->up) union_set(curr, curr->up, eps);
            if (curr->down) union_set(curr, curr->down, eps);
            if (curr->lt) union_set(curr, curr->lt, eps);
            if (curr->rt) union_set(curr, curr->rt, eps);
        }
    }
}

void flood(unsigned char *img, int start_x, int start_y, 
           int new_r, int new_g, int new_b, 
           int old_val, int w, int h) {
    int dirs[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};
    point *stack = (point*)malloc(w*h * sizeof(point));
    int top = 0;
    stack[top++] = (point){start_x, start_y};
    
    while (top > 0) {
        point p = stack[--top];
        if (p.x < 0 || p.x >= w || p.y < 0 || p.y >= h) continue;
        
        int idx = (p.y*w + p.x) * 4;
        if (img[idx] > old_val) continue;
        
        img[idx] = new_r;
        img[idx+1] = new_g;
        img[idx+2] = new_b;
        
        for (int i = 0; i < 4; i++) {
            int nx = p.x + dirs[i][0];
            int ny = p.y + dirs[i][1];
            if (nx > 0 && nx < w && ny > 0 && ny < h) {
                int nidx = (ny*w + nx) * 4;
                if (img[nidx] <= old_val) {
                    stack[top++] = (point){nx, ny};
                }
            }
        }
    }
    free(stack);
}

void color_components(unsigned char *img, int w, int h, int eps) {
    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {
            int idx = (y*w + x) * 4;
            if (img[idx] < eps) {
                int r = rand() % 256;
                int g = rand() % 256;
                int b = rand() % 256;
                flood(img, x, y, r, g, b, eps, w, h);
            }
        }
    }
}

int main() {
    int w = 0, h = 0;
    char *input = "skull_input.png";
    unsigned char *img = (unsigned char*)load_png_file(input, &w, &h);
    double eps = 28.0;
    
    apply_filter(img, w, h);
    Node *graph = create_graph(img, w, h);
    find_components(graph, w, h, eps);
    color_components(img, w, h, eps);
    
    char *output = "output.png";
    lodepng_encode32_file(output, img, w, h);
    
    free(img);
    free(graph);
    return 0;
}