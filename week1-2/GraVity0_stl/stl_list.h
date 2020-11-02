#ifndef YYT_STL_STL_LIST_H
#define YYT_STL_STL_LIST_H

#include <cstddef>
#include "stl_config.h"
#include "stl_iterator_base.h"
#include "stl_alloc.h"
#include "stl_iterator.h"
#include "stl_algobase.h"

__STL_BEGIN_NAMESPACE

//list链表节点的数据结构
struct _List_node_base
{
    _List_node_base *_M_next; //指向直接后继节点
    _List_node_base *_M_prev; //指向直接前驱节点
};

template<class _Tp>
struct _List_node : public _List_node_base
{
    _Tp _M_data; //节点存储的数据
};

struct _List_iterator_base
{
    //定义数据类型
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    //注意：list迭代器的类型是双向迭代器bidirectional_iterator
    typedef bindirectional_iterator_tag iterator_category;
    _List_node_base *_M_node;
    _List_iterator_base(_List_node_base *__x) : _M_node(__x){}
    
    _List_iterator_base() {}
    
    //更新节点
    void _M_incr() { _M_node = _M_node = _M_node->_M_next; } //指向后继
    void _M_decr() { _M_node = _M_node = _M_node->_M_prev; } //指向前驱
    
    //操作符重载
    bool operation==(const _List_iterator_base &__x) const
    {
        return _M_node == _x._M_node;
    }
    bool operation!=(const _List_iterator_base &__x) const
    {
        return _M_node != __x._M_node;
    }
};

template<class _Tp, class _ref, class _Ptr>
struct _List_iterator : public _list_iterator_base
{
    typedef _List_iterator<_Tp, _Tp &, _Tp *> iterator;
    typedef _List_iterator<_Tp, const _Tp &, const _Tp *> const_iterator;
    typedef _List_iterator<_Tp, _Ref, _Ptr> _Self;
    typedef _Tp value_type;
    typedef _Ref reference;
    typedef _Ptr pointer;
    typedef _List_node<_Tp> _Node;
    
    //构造函数
    _List_iterator(_Node *__x) : _List_iterator_base(__x) {}
    _List_iterator() {}
    _List_iterator(const iterator &__x) : _List_iterator_base(__x._M_node){}
    
    //重载获取节点数据
    reference operator*() const
    {
        return ((_Node *) _M_node)->_M_data;
    }
    
    pointer operator->() const
    {
        return &(operator*());
    }
    
    _Self &operator++()
    {
        this->_M_incr();
        return *this;
    }
    
    _Self operator++(int) {
        _Self __tmp = *this;
        this->_M_incr();
        return __tmp;
    }

    _Self &operator--() {
        this->_M_decr();
        return *this;
    }

    _Self operator--(int) {
        _Self __tmp = *this;
        this->_M_decr();
        return __tmp;
    }
};

template<class _Tp, class _Alloc>
class _List_base
{
public:
    typedef _Alloc allocator_type;
    
    allocator_type get_allocator() const { return allocator_type(); }
    
    _List_base(const allocator_type &)
    {
        _M_node = _M_get_node();
        _M_node->_M_next = _M_node;
        _M_node->_M_prev = _M_node;
    }
    
    ~_List_base()
    {
        clear();
        _M_put_node(_M_node);
    }
    
    void clear();

protected :
    typedef simple_alloc<_List_node<_Tp>, _Alloc> _Alloc_type;
    
    _List_node<_Tp> *_M_get_node()
    {
        return _Alloc_type::allocate(1);
    }
    
    void _M_put_node(_List_node<_Tp> *__p)
    {
        _Alloc_type::deallocate(__p , 1);
    }
    
protected:
    _Lsit_node<_Tp> *_M_node;
};

template<class _Tp, _Alloc>::clear()
{
    //取到链表头
    _List_node<_Tp> *__cur = (_List_node<_Tp> *) _M_node->_M_next;
    //while循环直到到达尾部
    while (__cur != _M_node)
    {
        _List_node<_Tp> *__tmp = __cur;
        __cur = (_List_node<_Tp> *) __cur->_M_next;
        _Destory(&__tmp->_M_data);
        _M_put_node(__tmp);
    }
    //最后设置_M_node的next和prev为本身，表示list为空
    _M_node->_M_next = _M_node;
    _M_node->_M_prev = _M_node;
}


//双向链表list类
template<class _Tp, class _Alloc = alloc>
class list : protected _List_base<_Tp, _Alloc>
{
    typedef _List_base<_Tp, _Alloc> _Base;
protected:
    typedef void *_Void_pointer;
public:
    typedef _Tp value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef _List_node<_Tp> _Node;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef typename _Base::allocator_type allocator_type;
    
    allocator_type get_allocator() const
    {
        return _Base::get_allocator();
    }
    
public:
    typedef _List_iterator<_Tp, _Tp &, _Tp *> iterator;
    typedef _List_iterator<_Tp, const _ Tp &, const _Tp *> const_iterator;
    typedef reverse_iterator<const_iterator> const_reverse_iterator;
    typedef reverse_iterator<iterator> reverse_iterator;
    
protected:
    using _Base::_M_node;
    using _Base::_M_put_node;
    using _Base::_M_get_node;
    
    _Node *_M_create_node(const _Tp &__x) {
        _Node *__p = _M_get_node();
        try {
            _Construct(&__p->_M_data, __x);
        }
        catch (...) {
            _M_put_node(__p);
            throw;
        }
        return __p;
    }

    _Node *_M_create_node() {
        _Node *__p = _M_get_node();
        try {
            _Construct(&__p->_M_data);
        }
        catch (...) {
            _M_put_node(__p);
            throw;
        }
        return __p;
    }
    
public:
    explicit list(const allocator_type &__a = allocator_type()) : _Base(__a) {}

    iterator begin() {
        return (_Node *) (_M_node->_M_next);
    }

    const_iterator begin() const {
        return (_Node *) (_M_node->_M_next);
    }

    iterator end() {
        return (_Node *) (_M_node);
    }

    const_iterator end() const {
        return (_Node *) (_M_node);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    const_iterator rbegin() const {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const {
        return reverse_iterator(end());
    }

    bool empty() const {
        return _M_node->_M_next == _M_node;
    }

    size_type size() const {
        size_type __result = 0;
        distance(begin(), end(), __result);
        return __result;
    }

    size_type max_size() const {
        return size_type(-1);
    }

    reference front() {
        return *begin();
    }

    const_reference front() const {
        return *begin();
    }

    reference back() {
        return *--end();
    }

    const_reference back() const {
        return *--end();
    }

    void swap(list<_Tp, _Alloc> &__x) {
        sakura_stl::swap(_M_node, __x._M_node);
    }

    //重要函数！！！：在指定的位置插入初始值为x的节点
    iterator insert(iterator __position, const _Tp &__x) {
        //首先创建一个初始值为x的节点，并返回该节点的地址
        _Node *__tmp = _M_create_node(__x);
        //调整节点指针，把新节点插入到指定位置
        __tmp->_M_next = __position._M_node;
        __tmp->_M_prev = __position._M_node->_M_prev;
        __position._M_node->_M_prev->_M_next = __tmp;
        __position._M_node->_M_prev = __tmp;
        //返回新节点
        return __tmp;
    }

    //在指定的位置插入为默认值的节点
    iterator insert(iterator __position) { return insert(__position, _Tp()); }

    //在指定位置插入n个初始值为x的节点
    template<class _Integer>
    void _M_insert_dispatch(iterator __pos, _Integer __n, _Integer __x, __true_type)
    {
        _M_fill_insert(__pos, (size_type) __n, (_Tp) __x);
    }

    
    template<class _InputIterator>
    void _M_insert_dispatch(iterator __pos, _InputIterator __first, _InputIterator __last, __false_type);

    template<class _InputIterator>
    void insert(iterator __pos, _InputIterator __first, _InputIterator __last)
    {
        typedef typename _Is_integer<_InputIterator>::_Integral _Integral;
        _M_insert_dispatch(__pos, __first, __last, _Integral());
    }

    void insert(iterator __pos, size_type __n, const _Tp &__x)
    {
        _M_fill_insert(__pos, __n, __x);
        
    }

    void _M_fill_insert(iterator __pos, size_type __n, const _Tp &__x);

    void push_front(const _Tp &__x)
    {
        insert(begin(), __x);
    }

    void push_front()
    {
        insert(begin());
    }

    void push_back(const _Tp &__x)
    {
        insert(end(), __x);
    }

    void push_back()
    {
        insert(end());
    }
    
    iterator erase(iterator __position) {
        _List_node_base *__next_node = __position._M_node->_M_next;
        _List_node_base *__prev_node = __position._M_node->_M_prev;
        _Node *__n = (_Node *) __position._M_node;
        __prev_node->_M_next = __next_node;
        __next_node->_M_prev = __prev_node;
        _Destroy(&__n->_M_data);
        _M_put_node(__n);
        return iterator((_Node *) __next_node);
    }

    iterator erase(iterator __first, iterator __last);

    void clear() { _Base::clear(); }

    void resize(size_type __new_size, const _Tp &__x);

    void resize(size_type __new_size) { this->resize(__new_size, _Tp()); }

    void pop_front() {
        erase(begin());
    }

    void pop_back() {
        iterator __tmp = end();
        erase(--__tmp);
    }

    list(size_type __n, const _Tp &__value,
         const allocator_type &__a = allocator_type())
            : _Base(__a) { insert(begin(), __n, __value); }

    explicit list(size_type __n)
            : _Base(allocator_type()) { insert(begin(), __n, _Tp()); }

    template<class _InputIterator>
    list(_InputIterator __first, _InputIterator __last,
         const allocator_type &__a = allocator_type())
            : _Base(__a) { insert(begin(), __first, __last); }

    list(const list<_Tp, _Alloc> &__x) : _Base(__x.get_allocator()) { insert(begin(), __x.begin(), __x.end()); }

    ~list() {}

    list<_Tp, _Alloc> &operator=(const list<_Tp, _Alloc> &__x);

public:
    void assign(size_type __n, const _Tp &__val) {
        _M_fill_assign(__n, __val);
    }

    void _M_fill_assign(size_type __n, const _Tp &__val);

    template<class _InputIterator>
    void assign(_InputIterator __first, _InputIterator __last) {
        typedef typename _Is_integer<_InputIterator>::_Integral _Integral;
        _M_assign_dispatch(__first, __last, _Integral());
    }

    template<class _Integer>
    void _M_assign_dispatch(_Integer __n, _Integer __val, __true_type) {
        _M_fill_assign((size_type) __n, (_Tp) __val);
    }

    template<class _InputIterator>
    void _M_assign_dispatch(_InputIterator __first, _InputIterator __last, __false_type);

protected:
    void transfer(iterator __position, iterator __first, iterator __last) {
        if (__position != __last) {
            //Remove [first, last) from its old position
            __last._M_node->_M_prev->_M_next = __position._M_node;
            __first._M_node->_M_prev->_M_next = __last._M_node;
            __position._M_node->_M_prev->_M_next = __first._M_node;

            //Splice [first, last) into its new position
            _List_node_base *__tmp = __position._M_node->_M_prev;
            __position._M_node->_M_prev = __last._M_node->_M_prev;
            __last._M_node->_M_prev = __first._M_node->_M_prev;
            __first._M_node->_M_prev = __tmp;
        }
    }

public:
    void splice(iterator __position, list &__x) {
        if (!__x.empty()) {
            this->transfer(__position, __x.begin(), __x.end());
        }
    }

    void splice(iterator __position, list &, iterator __i) {
        iterator __j = __i;
        ++__j;
//            if(__position == __i || __position == __j) return;
        this->transfer(__position, __i, __j);
    }

    void splice(iterator __position, list &, iterator __first, iterator __last) {
        if (__first != __last)
            this->transfer(__position, __first, __last);
    }

    void remove(const _Tp &__value);

    void unique();

    void merge(list &__x);

    void reverse();

    void sort();

    template<class _Predicate>
    void remove_if(_Predicate);

    template<class _BinaryPredicate>
    void unique(_BinaryPredicate);

    template<class _StrictWeakOrdering>
    void merge(list &, _StrictWeakOrdering);

    template<class _StrictWeakOrdering>
    void sort(_StrictWeakOrdering);
};

template<class _Tp, class _Alloc>
inline bool operator==(const list<_Tp, _Alloc> &__x, const list<_Tp, _Alloc> &__y) {
    typedef typename list<_Tp, _Alloc>::const_iterator const_iterator;
    const_iterator __end1 = __x.end();
    const_iterator __end2 = __y.end();

    const_iterator __i1 = __x.begin();
    const_iterator __i2 = __y.begin();
    while (__i1 != __end1 && __i2 != __end2 && *__i1 == *__i2) {
        ++__i1;
        ++__i2;
    }
    return __i1 == __end1 && __i2 == __end2;
}

template<class _Tp, class _Alloc>
inline bool operator<(const list<_Tp, _Alloc> &__x,
                      const list<_Tp, _Alloc> &__y) {
    return lexicographical_compare(__x.begin(), __x.end(),
                                   __y.begin(), __y.end());
}

template<class _Tp, class _Alloc>
inline bool operator!=(const list<_Tp, _Alloc> &__x,
                       const list<_Tp, _Alloc> &__y) {
    return !(__x == __y);
}

template<class _Tp, class _Alloc>
inline bool operator>(const list<_Tp, _Alloc> &__x,
                      const list<_Tp, _Alloc> &__y) {
    return __y < __x;
}

template<class _Tp, class _Alloc>
inline bool operator<=(const list<_Tp, _Alloc> &__x,
                       const list<_Tp, _Alloc> &__y) {
    return !(__y < __x);
}

template<class _Tp, class _Alloc>
inline bool operator>=(const list<_Tp, _Alloc> &__x,
                       const list<_Tp, _Alloc> &__y) {
    return !(__x < __y);
}

template<class _Tp, class _Alloc>
inline void
swap(list<_Tp, _Alloc> &__x, list<_Tp, _Alloc> &__y) {
    __x.swap(__y);
}

//该函数用于在 pos 前插入来自范围 [first, last) 的元素。
template<class _Tp, class _Alloc>
template<class _InputIterator>
void
list<_Tp, _Alloc>::_M_insert_dispatch(iterator __pos, _InputIterator __first, _InputIterator __last, __false_type) {
    for (; __first != __last; ++__first) {
        insert(__pos, *__first);
    }
}

template<class _Tp, class _Alloc>
void
list<_Tp, _Alloc>::_M_fill_insert(iterator __position,
                                  size_type __n, const _Tp &__x) {
    for (; __n > 0; --__n)
        insert(__position, __x);
}

//移除范围 [first; last) 中的元素。后随最后移除元素的迭代器。若 pos 指代末元素，则返回 end() 迭代器。
template<class _Tp, class _Alloc>
typename list<_Tp, _Alloc>::iterator list<_Tp, _Alloc>::erase(iterator __first, iterator __last) {
    while (__first != __last) {
        erase(__first++);
    }
    return __last;
}

//重设容器大小以容纳 count 个元素。
//若当前大小大于 count ，则减小容器为其首 count 个元素。
//若当前大小小于 count ，则后附额外元素，并以 value 的副本初始化。
template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::resize(size_type __new_size, const _Tp &__x) {
    iterator __i = begin();
    size_type __len = 0;
    for (; __i != end() && __len < __new_size; ++__i, ++__len);
    //代表当前大小大于等于count
    if (__len == __new_size) {
        erase(__i, end());
    } else {// __i == end()
        insert(end(), __new_size - __len, __x);
    }
}

template<class _Tp, class _Alloc>
list<_Tp, _Alloc> &list<_Tp, _Alloc>::operator=(const list<_Tp, _Alloc> &__x) {
    if (this != &__x) {
        iterator __first1 = begin();
        iterator __last1 = end();
        const_iterator __first2 = __x.begin();
        const_iterator __last2 = __x.end();
        while (__first1 != __last1 && __first2 != __last2) {
            *__first1++ = *__first2++;
        }
        if (__first2 == __last2) {
            erase(__first1, __last1);
        } else {
            insert(__last1, __first2, __last2);
        }
    }
    return *this;
}

//用于assign
//以 count 份 value 的副本替换内容。
template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::_M_fill_assign(size_type __n, const _Tp &__val) {
    iterator __i = begin();
    for (; __i != end() && __n > 0; ++__i, --__n) {
        *__i = __val;
    }
    if (__n > 0) {
        insert(end(), __n, __val);
    } else {
        //移除范围 [first; last) 中的元素。
        erase(__i, end());
    }
}

//用于assign
//以范围 [first, last) 中元素的副本替换内容。若任一参数是指向 *this 中的迭代器则行为未定义。
template<class _Tp, class _Alloc>
template<class _InputIter>
void
list<_Tp, _Alloc>::_M_assign_dispatch(_InputIter __first2, _InputIter __last2,
                                      __false_type) {
    iterator __first1 = begin();
    iterator __last1 = end();
    for (; __first1 != __last1 && __first2 != __last2; ++__first1, ++__first2)
        *__first1 = *__first2;
    if (__first2 == __last2)
        erase(__first1, __last1);
    else
        insert(__last1, __first2, __last2);
}

template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::remove(const _Tp &__value) {
    iterator __first = begin();
    iterator __last = end();
    while (__first != __last) {
        iterator __next = __first;
        ++__next;
        if (*__first == __value) {
            erase(__first);
        }
        __first = __next;
    }
}

//从容器移除所有相继的重复元素。只留下相等元素组中的第一个元素。第一版本用 operator== 比较元素，第二版本用二元谓词 p 比较元素
template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::unique() {
    iterator __first = begin();
    iterator __last = end();
    iterator __next = __first;
    while (++__next != __last) {
        if (*__first == *__next) {
            erase(__next);
        } else {
            //只有在first != next的时候，first才向前移动
            __first = __next;
        }
        __next = __first;
    }
}

//    归并二个已排序链表为一个。链表应以升序排序。
template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::merge(list<_Tp, _Alloc> &__x) {

}

inline void __List_base_reverse(_List_node_base *__p) {
    _List_node_base *__tmp = __p;
    do {
        sakura_stl::swap(__tmp->_M_next, __tmp->_M_prev);
        __tmp = __tmp->_M_prev;  // Old next node is now prev.
    } while (__tmp != __p);
}

template<class _Tp, class _Alloc>
inline void list<_Tp, _Alloc>::reverse() {
    __List_base_reverse(this->_M_node);
}

template<class _Tp, class _Alloc>
void list<_Tp, _Alloc>::sort() {
    // Do nothing if the list has length 0 or 1.

}

template<class _Tp, class _Alloc>
template<class _Predicate>
void list<_Tp, _Alloc>::remove_if(_Predicate __pred) {
    iterator __first = begin();
    iterator __last = end();
    while (__first != __last) {
        iterator __next = __first;
        ++__next;
        if (__pred(*__first)) erase(__first);
        __first = __next;
    }
}

template<class _Tp, class _Alloc>
template<class _BinaryPredicate>
void list<_Tp, _Alloc>::unique(_BinaryPredicate __binary_pred) {
    iterator __first = begin();
    iterator __last = end();
    if (__first == __last) return;
    iterator __next = __first;
    while (++__next != __last) {
        if (__binary_pred(*__first, *__next))
            erase(__next);
        else
            __first = __next;
        __next = __first;
    }
}


__STL_END_NAMESPACE

#endif
