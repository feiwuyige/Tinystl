#ifndef ZYL_TYPE_TRAITS_H
#define ZYL_TYPE_TRAITS_H

#include <type_traits>
namespace mystl{

    //辅助类
    template <typename T, T v>
    class m_integral_constant{
    public:
        static constexpr T value = v;
    };

    //定义模板别名
    //m_bool_constant是一个模板别名
    template <bool b>
    using m_bool_constant = m_integral_constant<bool, b>;

    typedef m_bool_constant<true> m_true_type;
    typedef m_bool_constant<false> m_false_type;

    //type traits
    //is_pair
    //前向声明
    template <typename T1, typename T2>
    class pair;

    //继承
    template <typename T>
    class is_pair : public mystl::m_false_type {};

    template <typename T1, typename T2>
    class is_pair<mystl::pair<T1, T2> > : public mystl::m_true_type{};
} // namespace stl
#endif // !ZYL_TYPE_TRAITS_H
