// 测试各种合法的标识符

int main() {
    // 简单标识符
    int a;
    int abc;
    int ABC;
    int aBc;
    
    // 下划线开头
    int _var;
    int __var;
    int ___var;
    
    // 包含下划线
    int my_variable;
    int MY_CONSTANT;
    int _private_var_;
    
    // 字母数字组合
    int var1;
    int var123;
    int a1b2c3;
    
    // 长标识符
    int this_is_a_very_long_variable_name_123;
    
    // 单字母
    int i, j, k, x, y, z;
    
    // 大小写混合
    int myVariable;
    int MyVariable;
    int myVARIABLE;
    
    return 0;
}
