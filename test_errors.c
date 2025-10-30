// 测试词法错误检测

int main() {
    // 未终止的字符串
    char *str = "This is unterminated
    
    // 未终止的字符常量
    char c = 'ab
    
    // 未终止的块注释
    /* This comment is not closed
    
    // 非法字符
    int x = 100;
    int@ y = 200;
    int$ z = 300;
    int` w = 400;
    
    return 0;
}
