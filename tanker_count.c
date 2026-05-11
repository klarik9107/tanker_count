#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height)
{
    unsigned char* image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error != 0) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    return image;
}

void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
    unsigned char* png;
    size_t pngsize;
    int error = lodepng_encode32(&png, &pngsize, image, width, height);
    if (error == 0) {
        lodepng_save_file(png, pngsize, filename);
    } else {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    free(png);
}

void to_gray(const unsigned char* picture, unsigned char* bw_pic, int size)
{
    for (int i = 0; i < size; i++) {
        int r = picture[i * 4];
        int g = picture[i * 4 + 1];
        int b = picture[i * 4 + 2];
        int max_ch = r > g ? r : g; if (b > max_ch) max_ch = b;
        int min_ch = r < g ? r : g; if (b < min_ch) min_ch = b;
        if (max_ch - min_ch > 8) {
            bw_pic[i] = 0;
        } else {
            bw_pic[i] = (r + g + b) / 3;
        }
    }
}

void threshold(unsigned char* bw_pic, int size, int level)
{
    for (int i = 0; i < size; i++) {
        if (bw_pic[i] < level) bw_pic[i] = 0;
        else                   bw_pic[i] = 255;
    }
}

void make_territory(const char* zone_file, unsigned char* territory, unsigned width, unsigned height)
{
    unsigned zw, zh;
    unsigned char* z = load_png(zone_file, &zw, &zh);
    int size = width * height;
    unsigned char* red = (unsigned char*)calloc(size, 1);
    for (int i = 0; i < size; i++) {
        int r = z[i * 4], g = z[i * 4 + 1], b = z[i * 4 + 2];
        if (r > 120 && r - g > 60 && r - b > 60) red[i] = 1;
    }
    free(z);

    unsigned char* wall = (unsigned char*)calloc(size, 1);
    int D = 40;
    for (int y = 0; y < (int)height; y++) {
        for (int x = 0; x < (int)width; x++) {
            for (int dy = -D; dy <= D; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= (int)height) continue;
                for (int dx = -D; dx <= D; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= (int)width) continue;
                    if (red[yy * width + xx]) { wall[y * width + x] = 1; goto next; }
                }
            }
            next:;
        }
    }
    free(red);

    unsigned char* outside = (unsigned char*)calloc(size, 1);
    int* queue = (int*)malloc(size * sizeof(int));
    int head = 0, tail = 0;
    for (int x = 0; x < (int)width; x++) {
        if (!wall[x]) { outside[x] = 1; queue[tail++] = x; }
        int p = (height - 1) * width + x;
        if (!wall[p]) { outside[p] = 1; queue[tail++] = p; }
    }
    for (int y = 0; y < (int)height; y++) {
        int p = y * width;
        if (!wall[p]) { outside[p] = 1; queue[tail++] = p; }
        p = y * width + width - 1;
        if (!wall[p]) { outside[p] = 1; queue[tail++] = p; }
    }
    while (head < tail) {
        int p = queue[head++];
        int x = p % width, y = p / width;
        int n[4] = { p - 1, p + 1, p - (int)width, p + (int)width };
        int ok[4] = { x > 0, x < (int)width - 1, y > 0, y < (int)height - 1 };
        for (int k = 0; k < 4; k++) {
            if (ok[k] && !outside[n[k]] && !wall[n[k]]) {
                outside[n[k]] = 1;
                queue[tail++] = n[k];
            }
        }
    }
    free(queue); free(wall);

    for (int i = 0; i < size; i++) territory[i] = outside[i] ? 0 : 255;
    free(outside);
}

int count_blob(unsigned char* bw_pic, unsigned char* visited, int start, int width, int size)
{
    int* stack = (int*)malloc(size * sizeof(int));
    int top = 0;
    stack[top++] = start;
    visited[start] = 1;
    int sz = 0, sx = 0, sy = 0;
    while (top > 0) {
        int p = stack[--top];
        int x = p % width, y = p / width;
        sz++; sx += x; sy += y;
        int n[4] = { p - 1, p + 1, p - width, p + width };
        int ok[4] = { x > 0, x < width - 1, y > 0, y < size / width - 1 };
        for (int k = 0; k < 4; k++) {
            if (ok[k] && !visited[n[k]] && bw_pic[n[k]] == 255) {
                visited[n[k]] = 1;
                stack[top++] = n[k];
            }
        }
    }
    free(stack);
    if (sz < 2 || sz > 12) return -1;
    return (sy / sz) * width + (sx / sz);
}

void mark(unsigned char* picture, int cx, int cy, int width, int height)
{
    for (int d = -3; d <= 3; d++) {
        int xs[2] = { cx - 3, cx + 3 }, ys[2] = { cy - 3, cy + 3 };
        for (int k = 0; k < 2; k++) {
            int x = cx + d, y = ys[k];
            if (x >= 0 && x < width && y >= 0 && y < height) { picture[4*(y*width+x)] = 0; picture[4*(y*width+x)+1] = 255; picture[4*(y*width+x)+2] = 0; }
            x = xs[k]; y = cy + d;
            if (x >= 0 && x < width && y >= 0 && y < height) { picture[4*(y*width+x)] = 0; picture[4*(y*width+x)+1] = 255; picture[4*(y*width+x)+2] = 0; }
        }
    }
}

int main(void)
{
    const char* filename = "strait.png";
    const char* zone_file = "zones.png";
    const char* out_file = "tankers_detected.png";

    unsigned int width, height;
    unsigned char* picture = load_png(filename, &width, &height);
    if (picture == NULL) {
        printf("Problem reading picture from the file %s. Error.\n", filename);
        return -1;
    }
    int size = width * height;

    unsigned char* bw_pic    = (unsigned char*)calloc(size, sizeof(unsigned char));
    unsigned char* visited   = (unsigned char*)calloc(size, sizeof(unsigned char));
    unsigned char* territory = (unsigned char*)calloc(size, sizeof(unsigned char));

    to_gray(picture, bw_pic, size);
    threshold(bw_pic, size, 92);
    make_territory(zone_file, territory, width, height);
    for (int i = 0; i < size; i++) {
        if (territory[i] == 0) bw_pic[i] = 0;
    }

    int counter = 0;
    for (int i = 0; i < size; i++) {
        if (!visited[i] && bw_pic[i] == 255) {
            int c = count_blob(bw_pic, visited, i, width, size);
            if (c >= 0) {
                int cx = c % width, cy = c / width;
                mark(picture, cx, cy, width, height);
                counter++;
            }
        }
    }
    printf("%d\n", counter);

    write_png(out_file, picture, width, height);
    free(bw_pic);
    free(visited);
    free(territory);
    free(picture);
    return 0;
}
