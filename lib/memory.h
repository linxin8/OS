#pragma once

template <typename T>
class unique_ptr
{
public:
    unique_ptr(T* pointer) : p(pointer) {}
    unique_ptr(const unique_ptr<T>&& r) : p(r.p)
    {
        r.p = nullptr;
    }
    ~unique_ptr()
    {
        if (p != nullptr)
        {
            delete p;
        }
    }
    T& operator->()
    {
        return *p;
    }
    const T& operator->() const
    {
        return *p;
    }

private:
    T* p;
};

template <typename T>
struct remove_reference
{
    using type = T;
};

template <typename T>
struct remove_reference<T&>
{
    using type = T;
};

template <typename T>
T&& forword(typename remove_reference<T>::type& t)
{
    return static_cast<T&&>(t);
}

template <typename T>
T&& forword(T&& t)
{
    return static_cast<T&&>(t);
}

template <typename T, typename... Ts>
unique_ptr<T> make_unique(Ts&&... params)
{
    return unique_ptr<T>(new T(forword<Ts>(params)...));
}
