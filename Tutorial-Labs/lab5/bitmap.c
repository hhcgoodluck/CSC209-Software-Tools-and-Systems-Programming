#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

/*
 * Read in the location of the pixel array, the image width, and the image 
 * height in the given bitmap file.
 * Use fseek to move the file position. Don't read the whole file.
 */
void read_bitmap_metadata(FILE *image, int *pixel_array_offset, int *width, int *height) {
    // 定位到偏移量字段（字节偏移 10）
    fseek(image, 10, SEEK_SET);
    fread(pixel_array_offset, sizeof(int), 1, image);

    // 定位到宽度字段（字节偏移 18）
    fseek(image, 18, SEEK_SET);
    fread(width, sizeof(int), 1, image);

    // 定位到高度字段（字节偏移 22）
    fseek(image, 22, SEEK_SET);
    fread(height, sizeof(int), 1, image);
}

/*
 * Read in pixel array by following these instructions:
 *
 * 1. First, allocate space for m `struct pixel *` values, where m is the 
 *    height of the image.  Each pointer will eventually point to one row of 
 *    pixel data.
 * 2. For each pointer you just allocated, initialize it to point to 
 *    heap-allocated space for an entire row of pixel data.
 * 3. Use the given file and pixel_array_offset to initialize the actual 
 *    struct pixel values. Assume that `sizeof(struct pixel) == 3`, which is 
 *    consistent with the bitmap file format.
 *    NOTE: We've tested this assumption on the Teaching Lab machines, but 
 *    if you're trying to work on your own computer, we strongly recommend 
 *    checking this assumption!
 * 4. Hint: Try reading a whole row of pixels in one fread call
 * 5. Return the address of the first `struct pixel *` you initialized.
 */
struct pixel **read_pixel_array(FILE *image, int pixel_array_offset, int width, int height) {
    // 分配指针数组，每个元素指向一行像素数据
    struct pixel **pixels = malloc(height * sizeof(struct pixel *));
    if (pixels == NULL) {
        fprintf(stderr, "Memory allocation failed for pixel row pointers\n");
        return NULL;
    }

    // 为每一行分配 width 个 struct pixel 的内存
    for (int i = 0; i < height; i++) {
        pixels[i] = malloc(width * sizeof(struct pixel)); // 每行 `width` 个像素
        if (pixels[i] == NULL) {
            fprintf(stderr, "Memory allocation failed for pixel row %d\n", i);

            // 释放之前已分配的行，防止内存泄漏
            for (int j = 0; j < i; j++) {
                free(pixels[j]);
            }
            free(pixels);
            return NULL;
        }
    }

    // 跳转到像素数组的起始位置
    fseek(image, pixel_array_offset, SEEK_SET);

    // 逐行读取像素数据，每次 fread 读取整行
    for (int i = 0; i < height; i++) {
        size_t read_count = fread(pixels[i], sizeof(struct pixel), width, image);

        if (read_count != width) {
            fprintf(stderr, "Error reading pixel data at row %d\n", i);
            return NULL;
        }
    }

    // 返回二维像素数组
    return pixels;
}

/*
 * Print the blue, green, and red colour values of a pixel.
 * You should not change this function.
 */
void print_pixel(struct pixel p) {
    printf("(%u, %u, %u)\n", p.blue, p.green, p.red);
}
