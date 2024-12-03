#ifndef ZYL_UTIL_H
#define ZYL_UTIL_H
//包含一些通用工具


#include <cstddef>
#include "type_traits.h"

namespace mystl{
    //move:其实并不执行“移动”操作，
    //而是一个 类型转换函数，它将其参数转换为 右值引用
    //因为::运算符可以访问类型成员，也可以访问静态数据成员，
    //cpp里面当通过作用域访问运算符的时候，我们默认是访问类中的静态数据成员，如果
    //要使用一个模板类型参数的类型成员，就必须显式的告诉编译器该名字是一个类型
    //
    template <typename T>
    typename std::remove_reference<T>::type&& //返回类型
    move(T&& arg) //定义为右值引用，可以接受左值和右值
    noexcept { 
        //表示不会抛出异常，用于编译器优化
        //static_cast<type>(args):将args转化为type类型
        return static_cast<typename std::remove_reference<T>::type&&>(arg);
    
    }    

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

    //swap:要交换连个对象的值，定义为引用，不然是在处理局部变量
    template <typename Tp>
    void swap(Tp &lhs, Tp &rhs){
        auto tmp(mystl::move(lhs));
        lhs = mystl::move(rhs);
        rhs = mystl::move(tmp);
    }

    //(void) ++first2 强制编译器 忽略 ++first2 的返回值，避免了编译器可能产生的警告，例如 未使用的值 警告。
    template<typename ForwardIter1, typename ForwardIter2>
    ForwardIter2 swap_range(ForwardIter1 first1, ForwardIter1 last1, ForwardIter2 first2){
        for(; first1 != last1; ++first1, (void) ++first2){
            mystl::swap(*first1, *first2);
        }
        return first2;
    }

    //重载swap模板
    //形参为数组的引用，数组的大小是数组类型的一部分
    //这里要定义这个版本，否则当我们使用数组作为实参的时候
    //若推导为第一个swap，会使用move函数，而数组类型没有右值引用
    template <class Tp, size_t N>
    void swap(Tp(&a)[N], Tp(&b)[N]){
        mystl::swap_range(a, a + N, b);
    }

    //pair
    template<typename Ty1, typename Ty2>
    class pair{
        public:
            typedef Ty1 first_type;
            typedef Ty2 second_type;

            first_type first; //第一个数据
            second_type second; //第二个数据

            //默认构造函数
            template<typename Other1 = Ty1, typename Other2 = Ty2,
                typename = typename std::enable_if<
                std::is_default_constructible<Other1>::value &&
                std::is_default_constructible<Other2>::value, void>::type>
                constexpr pair() : first(), second(){
                    
                }
            //允许隐式构造
            template<typename U1 = Ty1, typename U2 = Ty2,
                typename std::enable_if<
                std::is_copy_constructible<U1>::value &&
                std::is_copy_constructible<U2>::value &&
                std::is_convertible<const U1&, Ty1>::value &&
                std::is_convertible<const U2&, Ty2>::value, int>::type = 0>
                constexpr pair(const Ty1& a, const Ty2& b) : first(a), second(b){

                }
            //显式构造
            template <typename U1 = Ty1, typename U2 = Ty2,
                typename std::enable_if<
                std::is_copy_constructible<U1>::value &&
                std::is_copy_constructible<U2>::value &&
                (!std::is_convertible<const U1&, Ty1> :: value ||
                 !std::is_convertible<const U2&, Ty2>::value), int>::type = 0>
                explicit constexpr pair(const Ty1& a, const Ty2& b) : first(a), second(b){

                }
            //定义拷贝构造函数和移动构造函数
            pair(const pair& rhs) = default;
            pair(pair&& rhs) = default;

            // implicit constructiable for other type
            template < typename Other1, typename Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, Other1>::value &&
                std::is_constructible<Ty2, Other2>::value &&
                std::is_convertible<Other1&&, Ty1>::value &&
                std::is_convertible<Other2&&, Ty2>::value, int>::type = 0>
                constexpr pair(Other1&& a, Other2&& b)
                : first(mystl::forward<Other1>(a)), second(mystl::forward<Other2>(b)){

                }

            // explicit constructiable for other type
            template <typename Other1, typename Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, Other1>::value &&
                std::is_constructible<Ty2, Other2>::value &&
                (!std::is_convertible<Other1, Ty1>::value ||
                 !std::is_convertible<Other2, Ty2>::value), int>::type = 0>
                explicit constexpr pair(Other1&& a, Other2&& b)
                : first(mystl::forward<Other1>(a)), second(mystl::forward<Other2>(b)){

                }

            // implicit constructiable for other pair
            template <class Other1, class Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, const Other1&>::value &&
                std::is_constructible<Ty2, const Other2&>::value &&
                std::is_convertible<const Other1&, Ty1>::value &&
                std::is_convertible<const Other2&, Ty2>::value, int>::type = 0>
                constexpr pair(const pair<Other1, Other2>& other)
                : first(other.first),second(other.second){
            }
            
            // explicit constructiable for other pair
            template <class Other1, class Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, Other1>::value &&
                std::is_constructible<Ty2, Other2>::value &&
                (!std::is_convertible<Other1, Ty1>::value ||
                 !std::is_convertible<Other2, Ty2>::value), int>::type = 0>
                explicit constexpr pair(pair<Other1, Other2>&& other)
                : first(mystl::forward<Other1>(other.first)),
                  second(mystl::forward<Other2>(other.second)) {
            }

            //移动构造，隐式
            template<typename Other1, typename Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, Other1>::value &&
                std::is_constructible<Ty2, Other2>::value &&
                std::is_convertible<Other1, Ty1>::value &&
                std::is_convertible<Other2, Ty2>::value, int>::type = 0>
                constexpr pair(pair<Other1, Other2>&& other)
                : first(mystl::forward<Other1>(other.first)),
                  second(mystl::forward<Other2>(other.second)){

                  }
            
            //移动构造，显式
            template<typename Other1, typename Other2,
                typename std::enable_if<
                std::is_constructible<Ty1, Other1>::value &&
                std::is_constructible<Ty2, Other2>::value &&
                (!std::is_convertible<Other1, Ty1>::value ||
                 !std::is_convertible<Other2, Ty2>::value), int>::type = 0>
                explicit constexpr pair(pair<Other1, Other2>&& other)
                : first(mystl::forward<Other1>(other.first)),
                  second(mystl::forward<Other2>(other.second)){

                  }

            //赋值运算符
            //赋值运算的结果是它的左侧运算对象，并且是一个左值
            //赋值运算符通常应该返回一个指向其左侧运算对象的引用
            //两个pair类型必须完全一致
            pair& operator=(const pair& rhs){
                if(this != &rhs){
                    first = rhs.first;
                    second = rhs.second;
                }
                return *this;
            }

            //移动赋值
            pair& operator=(pair&& rhs){
                if(this != &rhs){
                    first = mystl::move(rhs.first);
                    second = mystl::move(rhs.second);
                }
                return *this;
            }
            
            //pair<Ty1,Ty2> = pair<Other1, Other2>
            template<typename Other1, typename Other2>
            pair& operator=(const pair<Other1, Other2>& other){
                first = other.first;
                second = other.second;
                return *this;
            }

            template<typename Other1, typename Other2>
            pair& operator=(pair<Other1, Other2>&& other){
                first = mystl::forward<Other1>(other.first);
                second = mystl::forward<Other2>(other.second);
                return *this;
            }

            ~pair() = default;

            void swap(pair& other){
                if(this != &other){
                    mystl::swap(first, other.first);
                    mystl::swap(second, other.second);
                }
            }

    };

    //重载比较运算符
    template <typename Ty1, typename Ty2>
    bool operator==(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }

    template <typename Ty1, typename Ty2>
    bool operator<(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        return lhs.first < rhs.first || (lhs.first == rhs.first && lhs.second < rhs.second);
    }

    template <typename Ty1, typename Ty2>
    bool operator!=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        return !(rhs == lhs);
    }

    template <typename Ty1, typename Ty2>
    bool operator>(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        return rhs < lhs;
    }

    template <typename Ty1, typename Ty2>
    bool operator<=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        //大于取反即可
        return !(rhs < lhs);
    }

    template <typename Ty1, typename Ty2>
    bool operator>=(const pair<Ty1, Ty2>& lhs, const pair<Ty1, Ty2>& rhs){
        //小于取反即可
        return !(lhs < rhs);
    }

    //重载swap
    template <typename Ty1, typename Ty2>
    void swap(pair<Ty1, Ty2>& lhs, pair<Ty1, Ty2>& rhs){
        lhs.swap(rhs);
    }

    //全局函数，让两个数据成为一个pair
    template <typename Ty1, typename Ty2>
    pair<Ty1, Ty2> make_pair(Ty1&& first, Ty2&& second){
        return pair<Ty1, Ty2>(mystl::forward<Ty1>(first), mystl::forward<Ty2>(second));
    }


}


#endif //!ZYL_UTIL_H
