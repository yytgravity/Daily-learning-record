#ifndef __STL_ALGOBASE_H__

#define __STL_ALGOBASE_H__


#include "type_traits.h"
#include "stl_config.h"
#include <string.h>
#include "stl_iterator_base.h"
#include "stl_iterator.h"




__STL_BEGIN_NAMESPACE
//如下相当于是一个copy_backward函数
/*在一个vector中
将每个元素向后移一位。

如果正向遍历并copy的话，
后面的元素被前面覆盖了，
copy的结果就不对了。

所以必须反向copy。
http://\c.biancheng.net/view/605.html
就只是反向复制了而已
*/
//这里算偏特化，指定了萃取机，只有双向迭代器可以backward
template<class _BidirectionalIter1, class _BidirectionalIter2, class _Distance>
inline _BidirectionalIter2 __copy_backward(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result, bidirectional_iterator_tag, _Distance *) {
	while (__first != __last) {
		*--__result = *--__last;
	}
	return __result;
}
template<class _BidirectionalIter1, class _BidirectionalIter2, class _Distance>
inline _BidirectionalIter2 __copy_backward(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result, random_access_iterator_tag, _Distance *) {
	for (_Distance __n = __last - __first; __n > 0; --__n) {
		*--__result * --__last;
	}
	return __result;
}
//https://\blog.csdn.net/dongyu_1989/article/details/80061944
//https://\blog.csdn.net/chengzi_comm/article/details/51974119
//按我的理解，就相当于是 copy backward的预处理函数
template<class _BidirectionalIter1, class _BidirectionalIter2, class _BoolType>
struct __copy_backward_dispatch {
	//两个萃取
	typedef typename iterator_traits<_BidirectionalIter1>::iterator_category _Cat;
	typedef typename iterator_traits<_BidirectionalIter1>::difference_type _Distance;

	static _BidirectionalIter2
		copy(_BidirectionalIter1 __first, _BidirectionalIter1 __last, _BidirectionalIter2 __result) {
		return __copy_backward(__first, __last, __result, _Cat(), (_Distance *)0);
	}
};
//特化版本1，有“无关紧要的赋值操作符” 会执行下面这个函数：

/*
memmove的处理措施：

（1）当源内存的首地址等于目标内存的首地址时，不进行任何拷贝

（2）当源内存的首地址大于目标内存的首地址时，实行正向拷贝

（3）当源内存的首地址小于目标内存的首地址时，实行反向拷贝

memcpy和memmove都是标准C库函数，在库函数string.h中，它们都是从src所指向的内存中复制count个字节到dest所指向的内存中，并返回dest的值
void *memmove(void *dest,const void *src,size_t count);
*/
template<class _Tp>
struct __copy_backward_dispatch<_Tp *, _Tp *, __true_type> {
	static _Tp *copy(const _Tp *__first, const _Tp *__last, _Tp *__result) {
		const ptrdiff_t _Num = __last - __first;
		memmove(__result - _Num, __first, sizeof(_Tp) * _Num);
		return __result - _Num;
	}
};


//特化版本2，没有“无关紧要的赋值操作符” 会执行下面这个函数,两个都是truetype,上面针对普通，下面针对const
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
//copy assignment operator 赋值函数
////没有显式定义ctor/dtor/copy/assignemt所以都是trivial
//显式定义了构造函数，所以是non-trivial ctor
//https://\www.daimajiaoliu.com/daima/4edeeb6bc100410
//如果构造函数中写了，那就是no-trivial
//可能大家会有疑问，trivial_assignment_operator 的用途，即不重要的，还有non-trivial 就是重要的，这两个知识点会包含虚函数，或者，虚函数表，即虚表，也就是多态的基础，

//https://\blog.csdn.net/lihao21/article/details/50688337
/*
其次是泛化,再针对原生指针做出特化,如果指针所指对象拥有的是trivial assignment operator,复制操作可以不通过它(也就是ctor),也就是说我们直接用memmove来完成拷贝.如果对于原生指针,它指向的对象拥有的是non-trivial assignment operator,我们就使用for循环来慢慢拷贝了.

*/
template<class _Tp>
inline _Tp *__copy_trivial(const _Tp *__first, const _Tp *__last, _Tp *__result) {
	memmove(__result, __first, sizeof(_Tp) * (__last - __first));
	return __result + (__last - __first);
}


//以下为copy函数
template<class _Tp>
inline _Tp *__copy_aux2(const _Tp *__first, const _Tp *__last, _Tp *__result, __true_type) {
	return __copy_trivial(__first, __last, __result);
}
template<class _InputIter, class _OutputIter, class _Tp>
inline _OutputIter __copy_aux(_InputIter __first, _InputIter __last, _OutputIter __result, _Tp *) {
	typedef typename __type_traits<_Tp>::has_trivial_assignment_operator _Trivial;
	return __copy_aux2(__first, __last, __result, _Trivial());
}

//https://\www.jianshu.com/p/dc81e085b445
//第一个相当于copy的接口，调用里面的copy辅助函数进行copy，三个迭代器，头，尾，输出
template<class _InputIter, class _OutputIter>
inline _OutputIter copy(_InputIter __first, _InputIter __last, _OutputIter __result) {
	return __copy_aux(__first, __last, __result, __VALUE_TYPE(__first));
}
//copy辅助函数，调用copy辅助2



template<class _InputIter, class _OutputIter, class _Distance>
inline _OutputIter __copy(_InputIter __first, _InputIter __last, _OutputIter __result, input_iterator_tag, _Distance *) {
	for (; __first != __last; ++__first, ++__result) {
		*__result = *__first;
	}
	return *__result;
}

/*
fill函数的作用是：将一个区间的元素都赋予val值。函数参数：fill(first,last,val);//first为容器的首迭代器，last为容器的末迭代器，val为将要替换的值。
fill_n函数的作用是：给你一个起始点，然后再给你一个数值count和val。 把从起始点开始依次赋予count个元素val的值。 注意： 不能在没有元素的空容器上调用fill_n函数例题：给你n个数，然后输入一些操作：start,count,paint
*/
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
//min and max 最基本的，当然我们可以自己增加比较函数
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
//迭代器的swap

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

#endif // !__STL_ALGOBASE_H__
