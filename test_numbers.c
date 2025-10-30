int main() {
    // 整数
    int zero = 0;
    int positive = 12345;
    int leading_zero = 0123;  // 注意:0开头但不是0x的是八进制(词法分析器将其识别为十进制)
    
    // 16进制
    int hex_lower = 0xabcdef;
    int hex_upper = 0XABCDEF;
    int hex_mixed = 0xABCdef123;
    int hex_short = 0x0;
    int hex_long = 0xFFFFFFFF;
    
    // 浮点数
    float f1 = 0.0;
    float f2 = 123.456;
    float f3 = .456;      // 注意:这会被识别为 . 和 456
    float f4 = 123.;
    
    // 科学计数法
    double e1 = 1e5;
    double e2 = 1E5;
    double e3 = 1.23e5;
    double e4 = 1.23E5;
    double e5 = 1.23e+5;
    double e6 = 1.23e-5;
    double e7 = 1.23E+10;
    double e8 = 1.23E-10;
    
    return 0;
}
