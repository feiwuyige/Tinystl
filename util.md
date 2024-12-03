# 完美转发

实际需求：某些函数需要将自己的参数转发给其他函数，在这个过程中，我们需要保证这些参数的所有性质，是否是 `const` 以及左值还是右值。考虑以下例子：

```cpp
template <typename F, typename T1, typename T2>
void flip1(F f, T1 t1, T2 t2){
	f(t2, t1);
}
```

这个函数在一般情况下正确，但是当我们 `f` 函数的原型中参数是引用就会出现错误：
```cpp
void f(int v1, int& v2); //v2是一个引用，我们在函数内部改变了v2的值
```

假设我们有如下调用：

```cpp
flip1(f, j, 42); //j是int类型
```

那么编译器会实例化出以下代码：

```cpp
void flip1(void (*f)(int, int&), int t1, int t2){
    f(t2, t1);
}
```

所以 `j` 的值被拷贝到 `t1` 中，调用 `f` 的时候我们将引用绑定到了 `t1`，而非 `j`，所以 `j` 的值不会发生改变，与我们预期不符合。

**这是因为我们将参数 `j` 传递给 `flip1` 的时候，丢失了参数的左值性，我们应该将 `j` 推导为 `int &` 类型**

**通过将一个函数参数定义为一个指向模板类型参数的右值引用，我们可以保持其对应实参的所有类型信息。**

传递一个左值，右值引用绑定左值，将该类型推导为对应类型的左值引用，根据引用折叠，得到一个左值引用

但是如果我们要调用的函数形参是一个右值，那么这种情况就会出错：
```cpp
void g(int &&i, int &j);
template <typename F, typename T1, typename T2>
void flip2(F f, T1&& t1, T2&& t2){
	f(t2, t1);
}

flip2(g, j, 42); 
//T1被推导为int&, T2 为int
void flip2(void(*f)(int&&, int), int& t1, int&& t2);
```

**函数参数与其他任何变量一样，都是左值表达式**，所以 `t2` 是一个左值，但是 `g` 的第一个参数需要一个右值，出错。

所以使用 `std::forward` 函数来实现完美转发，**与move不同，forward必须通过显式模板实参来调用。forward返回该显式实参类型的右值引用。即，forward\<T\>的返回类型是T&&。**

```cpp
template <typename F, typename T1, typename T2>
void flip2(F f, T1&& t1, T2&& t2){
	f(std::forward<T2>(t2), std::forward<T1>(t1));
}
```

注意 `std::forward` 不能如下实现，来使用模板参数推导（https://stackoverflow.com/questions/7779900/why-is-template-argument-deduction-disabled-with-stdforward）：
```cpp
//forward:左值实例化该模板
    template <typename T>
    T&& forward(typename std::remove_reference<T>::type& arg) noexcept{
        return static_cast<T&&>(arg);
    }
    //forward:右值实例化该模板
    template <typename T>
    T&& forward(typename std::remove_reference<T>::type&& arg) noexcept{
        static_assert(!std::is_lvalue_reference<T>::value, "bad forward");
        return static_cast<T&&>(arg);
    }
    
    /* 
    我的想法的forward，但是是不正确的
    template <typename T>
    T &&
    forward(T&& arg){
        return static_cast<T&&>(arg);
    } 
    */
```

**有名字的值是一个左值**，所以定义了一个右值引用 `z`，当我们传递给我们实现的 `forward` 的时候，他会将 `T` 推导为对应类型的引用类型，而不是对应类型。

```cpp
#include <utility>
#include <iostream>

template<typename T>
T&& forward_with_deduction(T&& obj)
{
    return static_cast<T&&>(obj);
}

void test(int&){
    std::cout << "int &" << std::endl;
}
void test(const int&){
    std::cout << "const int &" << std::endl;
}
void test(int&&){
    std::cout << "int &&" << std::endl;
}

template<typename T>
void perfect_forwarder(T&& obj)
{
    test(forward_with_deduction(obj));
}

int main()
{
    int x;
    const int& y(x);
    int&& z = std::move(x);

    test(forward_with_deduction(7));    //  7 is an int&&, correctly calls test(int&&)
    test(forward_with_deduction(z));    //  z is treated as an int&, calls test(int&)

    //  All the below call test(int&) or test(const int&) because in perfect_forwarder 'obj' is treated as
    //  an int& or const int& (because it is named) so T in forward_with_deduction is deduced as int& 
    //  or const int&. The T&& in static_cast<T&&>(obj) then collapses to int& or const int& - which is not what 
    //  we want in the bottom two cases.
    perfect_forwarder(x);           
    perfect_forwarder(y);           
    perfect_forwarder(std::move(x));
    perfect_forwarder(std::move(y));
}
```

# 数组引用

标准库在实现swap函数的时候，使用了如下两种方式：

```cpp
 	template <typename Tp>
    void swap(Tp &lhs, Tp &rhs){
        auto tmp(mystl::move(lhs));
        lhs = mystl::move(rhs);
        rhs = mystl::move(tmp);
    }

    template <class Tp, size_t N>
    void swap(Tp(&a)[N], Tp(&b)[N]){
        mystl::swap_range(a, a + N, b);
    }

```

这里要注意数组这种模板，这种类型必须实现，如果不实现，就会出现错误。原理如下：

1. 进行模板实参推导的时候，如果形参定义为引用类型，则会进行正常的引用绑定规则：
   除了两种特殊情况以外，其他所有引用的类型都要与与之绑定的对象严格匹配，这两种特殊情况为：
   * 初始化常量引用（即引用指向的对象无法被修改）时允许用任意表达式作为初始值，只要该表达式的结果能转换成引用的类型即可。
   * 将基类的引用可以绑定到一个派生类对象
2. 在模板实参推导的时候，只会出现两种类型转换，一种是将非const转化为const；一种是当形参不是引用类型的时候，可以对数组名和函数名进行正常的指针转换。

所以如果没有数组这个模板，那么就会实例化第一个 `swap` 模板，所以 `lhs` 会被推导为数组引用类型，然后调用 `move` 函数，

```cpp
#include <iostream>
namespace mystl
{
  template<typename T>
  typename std::remove_reference<T>::type && move(T && arg) noexcept
  {
    return static_cast<typename std::remove_reference<T>::type &&>(arg);
  }
  
  /* First instantiated from: insights.cpp:22 */
  #ifdef INSIGHTS_USE_TEMPLATE
  template<>
  typename std::remove_reference<int (&)[3]>::type && move<int (&)[3]>(int (&arg)[3]) noexcept
  {
    return static_cast<typename std::remove_reference<int (&)[3]>::type &&>(arg);
  }
  #endif
  
  template<typename Tp>
  void swap(Tp & lhs, Tp & rhs)
  {
    auto tmp = move(lhs);
  }
  
  /* First instantiated from: insights.cpp:29 */
  #ifdef INSIGHTS_USE_TEMPLATE
  template<>
  void swap<int[3]>(int (&lhs)[3], int (&rhs)[3])
  {
    int * tmp = move(lhs);
  }
  #endif
  
  
}

int main()
{
  int a[3] = {0, 1, 2};
  int b[3] = {4, 5, 6};
  mystl::swap(a, b);
  std::operator<<(std::cout.operator<<(a[0]), " ").operator<<(b[0]).operator<<(std::endl);
  return 0;
}

```

# 匿名模板参数

```cpp
//默认构造函数
template<typename Other1 = Ty1, typename Other2 = Ty2,
    typename = typename std::enable_if<
    std::is_default_constructible<Other1>::value &&
    std::is_default_constructible<Other2>::value, void>::type>
    constexpr pair() : first(), second(){
        
    }
```

这里用到了三个模板参数，其中第一个和第二个模板参数 `Other1` 和 `Other2` 分别具有默认类型实参 `Ty1` `Ty2`，第三个模板参数是匿名模板参数（没有名字），主要用于 SFINAE（Substitution Failure Is Not An Error）机制。它的作用是启用或禁用这个模板构造函数，具体条件由 `std::enable_if` 的参数决定。如果 `Ty1` 和 `Ty2` 是可默认构造的类型，第三个匿名模板参数被解析为 `void`，使得该模板构造函数有效。如果 `Ty1` 或 `Ty2` 不可默认构造，`std::enable_if` 失败，此模板构造函数被排除（SFINAE）。

`std::enable_if` 用来启用或禁用特定模板。

`std::enable_if<condition, T>::type` 只有在 `condition` 为 `true` 时，才定义为类型 `T`，否则编译器会忽略该模板。

`constexpr`：声明该构造函数是一个常量表达式函数，这允许它在编译时被计算（如果使用的成员也是 `constexpr`）。

#### **检查构造函数**

如果需要判断某个构造函数是否存在并合法：

```cpp
std::is_constructible<Target, Source>::value
```

#### **检查隐式转换**

如果需要判断类型转换是否可以自动发生：

```cpp
std::is_convertible<Source, Target>::value
```
