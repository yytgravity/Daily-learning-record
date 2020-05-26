//
// Created by yyt on 2020/5/6.
//

#ifndef YYT_STL_STL_CONSTRUCT_H
#define YYT_STL_STL_CONSTRUCT_H

#include <new>
#include "stl_config.h"
#include "type_traits.h"

__STL_BEGIN_NAMESPACE

/*new的功能实现：第一步分配内存，第二步调用类的构造函数。现在将其拆分为alloc和construct。
 placement new，就是在指针p所指向的内存空间创建一个T1类型的对象，但是对象的内容是从T2类型的对象转换过来的（调用了T1的构造函数，T1::T1(value)）。

 就是在已有空间的基础上重新调整分配的空间，类似于realloc函数。这个操作就是把已有的空间当成一个缓冲区来使用，这样子就减少了分配空间所耗费的时间，因为直接用new操作符分配内存的话，在堆中查找足够大的剩余空间速度是比较慢的。
 
 */
    template<class _T1, class _T2>
    inline void _Construct(_T1 *__p, const _T2 &__value)
    {
        new((void *) __p) _T1(__value);//placement new，调用T1::T1(value);
    }
    
    template<class _T1>
    inline void _Construct(_T1 *__p){
        new((void *) __p) _T1();
    }

    template<class _Tp>
    void destroy_one(_Tp *, __true_type)
    {}

    template<class _Tp>
    void destroy_one(_Tp *pointer, __false_type)
    {
        if(pointer != nullptr)
        {
            pointer->~_Tp();//显示调用析构函数
        }
    }
//has trival destructor萃取判断是否进行析构。
    template<class _Tp>
    inline void _Destroy(_Tp *__pointer)
    {
        destroy_one(__pointer, typename __type_traits<_Tp>::has_trivial_destructor());
    }

/* https:\//blog.csdn.net/vanturman/article/details/80269081? */
/*
如果你想直接告诉编译器__type_traits<_Tp>::has_trivial_destructor()是类型而不是变量，只需用typename修饰：这样编译器就可以确定__type_traits<_Tp>::has_trivial_destructor()是一个类型，而不再需要等到实例化时期才能确定。
*/
/*
在调用析构函数前，有必要解释下trivial destructor和non-trivial destructor。
如果用户不定义析构函数，那么编译器会自动合成析构函数，但是这个析构函数基本没什么用，所以称为
trivial destructor无意义的析构函数。
如果用户定义了析构函数，说明在释放对象所占用内存之前要做一些事情，就成为non-trivial destructor。
在C++中，如果只有基本数据类型，那么就不用调用析构函数，就认为析构函数是trivial destructor。
如果有像指针这样的数据类型，那么久有必要调用析构函数了，就认为析构函数是non-trivial destructor。
在STL中，调用析构函数时，都会用一个has_trivial_destructor来判断对象是否含有trivial destructor，如果是，
那么不会调用析构函数，什么也不做
void __destroy_aux(ForwardIterator, ForwardIterator, __true_type)是空函数。
反之，则调用析构函数，释放对象。
__destroy_aux(ForwardIterator first, ForwardIterator last, __false_type)调用析构函数
结合下面的图来看代码：
                                       特化
                                       ____>__destroy_aux(.,_false_type)
                                       |
                                       |NO _false_type 进行析构
                                       |
                    __泛化_>_destroy()--has trival destructor?
                    |                   |
                    |                   |YES _true_type  不进行析构
                    |                   |
                    |                   |_特化_>_destroy_aux(,_true_type)
destroy()--
                    |
                    |
                    |
                    |------特化------>不进行析构
                    |  (char* ,char*)
                    |
                    |------特化------>不进行析构
                    |  (wchar* ,wchar*)
                    |
                    |
                    |------特化------>pointer->~T()
                        (T* pointer)
*/

    template<class _ForwardIterator>
    void __destroy_aux(_ForwardIterator __first, _ForwardIterator __last, __false_type)
    {
        for(; __first != __last; ++__first)
        {
            
            _Destroy(&*__first);//*迭代器名 就表示迭代器所指向的元素，&引用
        }
    }

    template<class _ForwardIterator>
    inline void __destroy_aux(_ForwardIterator, _ForwardIterator, __true_type)
    {}

    template<class _ForwardIterator, class _Tp>
    inline void __destory(_ForwardIterator __first, _ForwardIterator __last, _Tp *)
    {
        typedef typename __type_traits<_Tp>::has_trivial_destructor _Trivial_destructor;
        __destroy_aux(__first, __last, _Trivial_destructor());
    }

    template<class _FowardIterator>
    inline void _Destroy(_FowardIterator __first, _FowardIterator __last){
        __destroy(__first, __last, __VALUE_TYPE(__first));
        //typedef void value_type; in stl_iterator.h
    }

    inline void _Destroy(char *, char *) {}

    inline void _Destroy(int *, int *) {}

    inline void _Destroy(float *, float *) {}

    inline void _Destroy(long *, long *) {}

    inline void _Destroy(double *, double *) {}

    template<class _T1, class _T2>
    inline void construct(_T1 *__p, const _T2 &__value) {
        _Construct(__p, __value);
    }

    template<class _T1>
    inline void construct(_T1 *_p) {
        _Construct(_p);
    }

    template<class _Tp>
    inline void destroy(_Tp *__pointer) {
        _Destroy(__pointer);
    }

    template<class _ForwardIterator>
    inline void destroy(_ForwardIterator __first, _ForwardIterator __last) {
        _Destroy(__first, __last);
    }

__STL_END_NAMESPACE
#endif //YYT_STL_STL_CONSTRUCT_H
