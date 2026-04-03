#include <stdio.h>
#include <stdlib.h>

/* Return a pointer to an array of two dynamically allocated arrays of ints.
   The first array contains the elements of the input array s that are at even indices.
   The second array contains the elements of the input array s that are at odd indices.

   Do not allocate any more memory than necessary. You are not permitted
   to include math.h.  You can do the math with modulo arithmetic and integer
   division.
*/
int **split_array(const int *s, int length) {
    int even_count = (length + 1) / 2; // 偶数索引的数量（向上取整）
    int odd_count = length / 2;        // 奇数索引的数量

    int **result = malloc(2 * sizeof(int *)); // 分配二级指针数组
    if (result == NULL) {
        printf("memory dynamically-allocated fail！\n");
        exit(1);
    }

    result[0] = malloc(even_count * sizeof(int)); // 分配偶数索引数组
    result[1] = malloc(odd_count * sizeof(int));  // 分配奇数索引数组
    if (result[0] == NULL || result[1] == NULL) {
        printf("memory dynamically-allocated fail！\n");
        exit(1);
    }

    // 填充两个子数组
    for (int i = 0, even_index = 0, odd_index = 0; i < length; i++) {
        if (i % 2 == 0) { // 偶数索引
            result[0][even_index++] = s[i];
        } else {          // 奇数索引
            result[1][odd_index++] = s[i];
        }
    }

    return result;
}

/* Return a pointer to an array of ints with size elements.
   - strs is an array of strings where each element is the string
     representation of an integer.
   - size is the size of the array
 */

int *build_array(char **strs, int size) {
    int *arr = malloc(size * sizeof(int)); // 分配足够的内存
    if (arr == NULL) {
        printf("memory dynamically-allocated fail！\n");
        exit(1);
    }
    for (int i = 0; i < size; i++) {
        arr[i] = strtol(strs[i], NULL, 10); // 将字符串转换为整数
    }
    return arr;
}


int main(int argc, char **argv) {
    /* Replace the comments in the next two lines with the appropriate
       arguments.  Do not add any additional lines of code to the main
       function or make other changes.
     */
    int *full_array = build_array(argv + 1, argc - 1);
    int **result = split_array(full_array, argc - 1);

    printf("Original array:\n");
    for (int i = 0; i < argc - 1; i++) {
        printf("%d ", full_array[i]);
    }
    printf("\n");

    printf("result[0]:\n");
    for (int i = 0; i < argc / 2; i++) {
        printf("%d ", result[0][i]);
    }
    printf("\n");

    printf("result[1]:\n");
    for (int i = 0; i < (argc - 1) / 2; i++) {
        printf("%d ", result[1][i]);
    }
    printf("\n");
    free(full_array);
    free(result[0]);
    free(result[1]);
    free(result);
    return 0;
}
