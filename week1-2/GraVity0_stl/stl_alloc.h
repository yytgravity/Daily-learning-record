//
// Created by yyt on 2020/5/19.
//

#ifndef YYT_STL_STL_ALLOC_H
#define YYT_STL_STL_ALLOC_H

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include "stl_config.h"
#include "stl_construct.h"

__STL_BEGIN_NAMESPACE

//以下为第一级配置器，直接使用malloc、realloc、free来实现功能。

    template<int __inst>
    class __malloc_alloc_template {
    public:
        static void *allocate(size_t __n) {
            void *__result = malloc(__n);
            if (0 == __result){
                fprintf(stderr, "out of memory");
                exit(1);
            }
            return __result;
        }
        static void deallocate(void *__p, size_t /*__n*/ )
        {
            free(__p);
        }

        static void *reallocate(void *__p, size_t /* old_size*/, size_t __new_sz)
        {
            void *__result = realloc(__p, __new_sz);
            if(0 == __result)
            {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
            return __result;
        }
    };
    /*参数 inst完全没起到作用，这里直接设置为0*/
    typedef __malloc_alloc_template<0> malloc_alloc;
    
    //多一层包装，使alloc具备标准接口：
    template<class _Tp, class _Alloc>
    class simple_alloc{
    public :
        static _Tp *alloc(size_t __n)
        {
            return 0 == __n ? 0 : (_Tp*) _Alloc::allocate(__n * sizeof(_Tp));
        }
        
        static _Tp *allocate(size_t __n){
            return (_Tp *) _Alloc::allocate(sizeof(_Tp));
        }
        
        static void deallocate(_Tp *__p)
        {
            _Alloc::deallocate(__p, sizeof(_Tp));
        }
        
        static void deallocate(_Tp *__p, size_t __n)
        {
            if(0 != __n)
            {
                _Alloc::deallocate(__p, __n * sizeof(_Tp));
            }
        }
    };
    
    //将alloc定义为一级分配器
    typedef malloc_alloc alloc;

__STL_END_NAMESPACE
#endif //YYT_STL_STL_ALLOC_H
