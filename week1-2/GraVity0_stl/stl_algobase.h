//
// Created by yyt on 2020/5/8.
//

#ifndef YYT_STL_STL_ALGOBASE_H
#define YYT_STL_STL_ALGOBASE_H

#include "stl_config.h"
#include "stl_iterator_base.h"
#include "stl_iterator.h"
#include "type_traits.h"
#include <string.h>

__STL_BEGIN_NAMESPACE
//copy_backward

    template<class _BidirectionalIter1, class _BidirectionalIter2, class _Distance>
    inline _BidirectionalIter2
    __copy_backward(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result,
                    bidirectional_iterator_tag, _Distance *) {
        while (__first != __last) {
            *--__result = *--__last;
        }
        return __result;
    }

    template<class _BidirectionalIter1, class _BidirectionalIter2, class _Distance>
    inline _BidirectionalIter2
    __copy_backward(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result,
                    random_access_iterator_tag, _Distance *) {
        for (_Distance __n = __last - __first; __n > 0; --__n) {
            *--__result = *--__last;
        }
        return __result;
    }

    template<class _BidirectionalIter1, class _BidirectionalIter2, class _BoolType>
    struct __copy_backward_dispatch {
        typedef typename iterator_traits<_BidirectionalIter1>::iterator_category _Cat;
        typedef typename iterator_traits<_BidirectionalIter1>::difference_type _Distance;

        static _BidirectionalIter2
        copy(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result) {
            return __copy_backward(__first, __last, __result, _Cat(), (_Distance *) 0);
        }
    };

    template<class _Tp>
    struct __copy_backward_dispatch<_Tp *, _Tp *, __true_type> {
        static _Tp *copy(const _Tp *__first, const _Tp *__last, _Tp *__result) {
            const ptrdiff_t _Num = __last - __first;
            memmove(__result - _Num, __first, sizeof(_Tp) * _Num);
            return __result - _Num;
        }
    };

    template<class _Tp>
    struct __copy_backward_dispatch<const _Tp *, _Tp *, __true_type> {
        static _Tp *copy(const _Tp *__first, const _Tp *__last, _Tp *__result) {
            return __copy_backward_dispatch<_Tp *, _Tp *, __true_type>::copy(__first, __last, __result);
        }
    };

    template<class _BI1, class _BI2>
    inline _BI2 copy_backward(_BI1 __first, _BI1 __last, _BI2 __result) {
        typedef typename __type_traits<typename iterator_traits<_BI2>::value_type>::has_trivial_assignment_operator _Trivial;
        return __copy_backward_dispatch<_BI1, _BI2, _Trivial>::copy(__first, __last, __result);
    }

//copy
    template<class _InputIter, class _OutputIter>
    inline _OutputIter copy(_InputIter __first, _InputIter __last, _OutputIter __result) {
        return __copy_aux(__first, __last, __result, __VALUE_TYPE(__first));
    }

    template<class _InputIter, class _OutputIter, class _Tp>
    inline _OutputIter __copy_aux(_InputIter __first, _InputIter __last, _OutputIter __result, _Tp *) {
        typedef typename __type_traits<_Tp>::has_trivial_assignment_operator _Trivial;
        return __copy_aux2(__first, __last, __result, _Trivial());
    }

    template<class _InputIter, class _OutputIter>
    inline _OutputIter __copy_aux2(_InputIter __first, _InputIter __last, _OutputIter __result, __false_type) {
        return __copy(__first, __last, __result, __ITERATOR_CATEGORY(__first), __DISTANCE_TYPE(__first));
    }

    template<class _InputIter, class _OutputIter>
    inline _OutputIter __copy_aux2(_InputIter __first, _InputIter __last, _OutputIter __result, __true_type) {
        return __copy(__first, __last, __result, __ITERATOR_CATEGORY(__first), __DISTANCE_TYPE(__first));
    }

    template<class _Tp>
    inline _Tp *__copy_aux2(const _Tp *__first, const _Tp *__last, _Tp *__result, __true_type) {
        return __copy_trivial(__first, __last, __result);
    }

    template<class _Tp>
    inline _Tp *
    __copy_trivial(const _Tp *__first, const _Tp *__last, _Tp *__result) {
        memmove(__result, __first, sizeof(_Tp) * (__last - __first));
        return __result + (__last - __first);
    }

    template<class _InputIter, class _OutputIter, class _Distance>
    inline _OutputIter
    __copy(_InputIter __first, _InputIter __last, _OutputIter __result, input_iterator_tag, _Distance *) {
        for (; __first != __last; ++__first, ++__result) {
            *__result = *__first;
        }
        return *__result;
    }

//fill
    template<class _ForwardIter, class _Tp>
    void fill(_ForwardIter __first, _ForwardIter __last, const _Tp &__value) {
        for (; __first != __last; ++__first) {
            *__first = __value;
        }
    }

//fill_n
    template<class _OutputIter, class _Size, class _Tp>
    _OutputIter fill_n(_OutputIter __first, _Size __n, const _Tp &__value) {
        for (; __n > 0; --__n, ++__first) {
            *__first = __value;
        }
        return __first;
    }

//min and max
    template<class _Tp>
    inline const _Tp &min(const _Tp &__a, const _Tp &__b) {
        return __b < __a ? __b : __a;
    }

    template<class _Tp>
    inline const _Tp &max(const _Tp &__a, const _Tp &__b) {
        return __a < __b ? __b : __a;
    }

    template<class _Tp, class _Compare>
    inline const _Tp &min(const _Tp &__a, const _Tp &__b, _Compare __comp) {
        return __comp(__b, __a) ? __b : __a;
    }

    template<class _Tp, class _Compare>
    inline const _Tp &max(const _Tp &__a, const _Tp &__b, _Compare __comp) {
        return __comp(__a, __b) ? __b : __a;
    }

//swap and iterator_swap
    template<class _ForwardIter1, class _ForwardIter2, class _Tp>
    inline void __iter_swap(_ForwardIter1 __a, _ForwardIter2 __b, _Tp *) {
        _Tp __tmp = *__a;
        *__a = *__b;
        *__b = __tmp;
//    swap(*__a, *__b);
    }

    template<class _ForwardIter1, class _ForwardIter2>
    inline void iter_swap(_ForwardIter1 __a, _ForwardIter2 __b) {
        __iter_swap(__a, __b, __VALUE_TYPE(__a));
    }

    template<class _Tp>
    inline void swap(_Tp &__a, _Tp &__b) {
        //引用本身是不能变的，只会变引用指向的值。
        _Tp __tmp = __a;
        __a = __b;
        __b = __tmp;
    }

    //equal
    template<class _InputIter1, class _InputIter2, class _BinaryPredicate>
    inline bool equal(_InputIter1 __first1, _InputIter1 __last1,
                      _InputIter2 __first2, _BinaryPredicate __binary_pred) {
        for (; __first1 != __last1; ++__first1, ++__first2)
            if (!__binary_pred(*__first1, *__first2))
                return false;
        return true;
    }

    template<class _InputIter1, class _InputIter2>
    inline bool equal(_InputIter1 __first1, _InputIter1 __last1,
                      _InputIter2 __first2) {
        for (; __first1 != __last1; ++__first1, ++__first2)
            if (*__first1 != *__first2)
                return false;
        return true;
    }

//--------------------------------------------------
// lexicographical_compare and lexicographical_compare_3way.
// (the latter is not part of the C++ standard.)

    template<class _InputIter1, class _InputIter2>
    bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                                 _InputIter2 __first2, _InputIter2 __last2) {
        for (; __first1 != __last1 && __first2 != __last2; ++__first1, ++__first2) {
            if (*__first1 < *__first2)
                return true;
            if (*__first2 < *__first1)
                return false;
        }
        return __first1 == __last1 && __first2 != __last2;
    }

    template<class _InputIter1, class _InputIter2, class _Compare>
    bool lexicographical_compare(_InputIter1 __first1, _InputIter1 __last1,
                                 _InputIter2 __first2, _InputIter2 __last2,
                                 _Compare __comp) {
        for (; __first1 != __last1 && __first2 != __last2; ++__first1, ++__first2) {
            if (__comp(*__first1, *__first2))
                return true;
            if (__comp(*__first2, *__first1))
                return false;
        }
        return __first1 == __last1 && __first2 != __last2;
    }

    inline bool
    lexicographical_compare(const unsigned char *__first1,
                            const unsigned char *__last1,
                            const unsigned char *__first2,
                            const unsigned char *__last2) {
        const size_t __len1 = __last1 - __first1;
        const size_t __len2 = __last2 - __first2;
        const int __result = memcmp(__first1, __first2, min(__len1, __len2));
        return __result != 0 ? __result < 0 : __len1 < __len2;
    }

__STL_END_NAMESPACE
#endif //yyt_STL_STL_ALGOBASE_H
