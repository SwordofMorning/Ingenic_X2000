#include <stdio.h>
#include <iostream>

class class_a {
    int v;
public:
    class_a(int value);
    void set_v(int value);
    void print(void);
};

class_a::class_a(int value)
{
    // 构造函数中最好不要 使用std::count 打印
    // 因为 std::count的构造函数可能还没有执行,因此出错
    printf("class_a constructor is called\n");
    v = value;
}

void class_a::set_v(int value)
{
    v = value;
}

void class_a::print(void)
{
    printf("%s: v=%d\n", __FUNCTION__, v);
}

// 导出给 c 语言调用需要 extern "C"
extern "C" int test_cpp(void);

class_a m_global(20);

int test_cpp(void)
{
    printf("%s: is called\n", __FUNCTION__);
    std::cout << "std::count " << __FUNCTION__ << " is called\n" << std::endl;

    class_a a(15);

    m_global.print();

    a.print();

    return 0;
}
