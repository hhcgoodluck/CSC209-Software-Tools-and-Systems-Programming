#include <stdio.h>   // 标准输入输出库，用于 printf
#include <stdlib.h>  // 标准库，用于 strtod()（将字符串转换为 double）

/*
 * Write a void function invest that takes your money and multiplies it by the given rate.
 */


/*
 * NOTE: don't change the main function!
 * Sample usage:
 * $ gcc -Wall -g -o invest invest.c
 * $ ./invest 10000 1.05
 * 10500.00
 */
void invest(double *principal, double rate) {    // double *principal 本金的指针，表示一个可修改的数值
    *principal *= rate;     // *principal = *principal * rate
}     // 直接修改 main 中的 principal 变量，而 不需要返回值，避免了传值调用带来的副本拷贝问题

int main(int argc, char **argv) {
    // Read in the command-line arguments and convert the strings to doubles
    // argc：命令行参数的 个数（包括程序名）
    // argv：参数数组（argv[0] 是程序名，argv[1] 和 argv[2] 是用户输入的值）

    double principal = strtod(argv[1], NULL);    // 转换 argv[1]（字符串）为 double，并存储到 principal（本金）
    double rate = strtod(argv[2], NULL);         // 转换 argv[2]（字符串）为 double，并存储到 rate（利率）

    // Call invest to make you more money
    // 传入 principal 的地址 &principal，以便 invest 直接修改本金值
    invest(&principal, rate);

    // %.2f 保留两位小数，确保输出格式符合货币计算
    printf("%.2f\n", principal);
    return 0;
}

/*
总结 核心概念
  指针传递：允许 invest 修改 principal 的值，而不需要返回值。
  命令行参数处理：使用 strtod 解析用户输入的 double 值。
  数值计算：本金 * 利率，实现投资增长。
  这段代码展示了 指针参数 的实际应用，是 C 语言中修改变量的一种 高效方法！
*/