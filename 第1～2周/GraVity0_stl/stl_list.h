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
};
