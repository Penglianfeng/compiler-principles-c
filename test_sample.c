// 这是一个词法分析测试文件
/* 多行注释
   测试注释识别 */

#include <stdio.h>
#define MAX_SIZE 100

int main(void) {
    // 关键字和标识符测试
    int num = 42;
    float pi = 3.14159;
    double exp_num = 1.5e-10;
    char letter = 'A';
    char escape_char = '\n';
    
    // 字符串测试
    char *str = "Hello, World!";
    char *escape_str = "Line1\nLine2\tTab";
    char *empty = "";
    
    // 下划线标识符测试
    int _private_var = 0;
    int my_variable_123 = 456;
    int __double_underscore = 789;
    
    // 16进制数测试
    int hex1 = 0x1A2B;
    int hex2 = 0xFF;
    int hex3 = 0xABCDEF;
    
    // 运算符测试
    int a = 10;
    int b = 20;
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    int mod = a % b;
    
    // 比较运算符
    if (a == b) {}
    if (a != b) {}
    if (a < b) {}
    if (a > b) {}
    if (a <= b) {}
    if (a >= b) {}
    
    // 自增自减
    a++;
    ++a;
    b--;
    --b;
    
    // 标点符号测试
    int arr[10] = {1, 2, 3, 4, 5};
    struct point { int x; int y; };
    
    // 浮点数测试
    double d1 = 123.456;
    double d2 = 0.123;
    double d3 = 123.;
    double d4 = 1.23e5;
    double d5 = 1.23e-5;
    double d6 = 1.23E+5;
    
    // 各种关键字测试
    for (int i = 0; i < 10; i++) {
        continue;
    }
    
    while (1) {
        break;
    }
    
    switch (num) {
        case 1:
            break;
        default:
            break;
    }
    
    return 0;
}
