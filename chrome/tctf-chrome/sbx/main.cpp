#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>

using namespace std;
//
//
//void biggies(vector<string> &words, vector<string>::size_type sz) {
//    //对单词按长度排序
//    stable_sort(words.begin(), words.end(),
//                [](const string &a, const string &b) {
//                    return a.size() < b.size();
//                });
//    //find_if查找第一个具有特定大小的元素
////    _InputIterator
////    find_if(_InputIterator __first, _InputIterator __last, _Predicate __pred)
////    {
////        for (; __first != __last; ++__first)
////            if (__pred(*__first))
////                break;
////        return __first;
////    }
//    auto wc = find_if(words.begin(), words.end(),
//                      [&sz](const string &str) {
//                          return str.size() > sz;
//                      });
//
//    auto count = words.end() - wc;
//    cout << "count :" << count << endl;
//    for_each(wc, words.end(),
//             [](const string &str) {
//                 cout << str << " " << endl;
//             });
//}
//
//int add(int i, int j) {
//    return i + j;
//}
//
//auto mod = [](int i, int j) { return i % j; };
//
//int main() {
//    vector<string> v = {"aaa", "nnnn", "mmmmm", "zzzzzzzzz"};
//    biggies(v, 3);
//    return 0;
//}

//template<class D>
//class A {
//public:
//    typedef int INT;
//    int a;
//
//    A(int _a) : a(_a) {}
//};
//
//
//template<class D>
//class B : public A<D> {
//public:
//    B(int _a) : A<D>(_a) {}
//
//    typedef typename A<D>::INT INT;
//    using ATy = A<D>;
//    using ATy::a;
//
//    int aplus() {
//        INT x = 1;
//        a += x;
//        return a;
//    }
//};
//
//int main() {
//    B<int> b(1);
//    B<int>::INT a1 = 2;
//    B<int>::ATy a2(3);
//    cout << b.aplus() << endl;
//}

//int main(int argc, char **argv) {
//    char ch = 0;
//    std::vector<uint8_t> data;
//    if (argc > 1) {
//        std::ifstream fin;
//        fin.open(argv[1], std::ios::in);
//        while (!fin.eof()) {
//            ch = fin.get();
//            if(ch != EOF)
//            {
//                data.push_back(ch);
//            }
//        }
//        fin.close();
//    } else {
//        while ((ch = getchar()) != EOF) {
//            data.push_back(ch);
//        }
//    }
//}
//void alloc_printf(char* str...){
//    printf(str);
//}
//
//int main(){
//    char* str;
//    alloc_printf(AFL_PATH "/as");
//    return 0;
//}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <zconf.h>
#include <map>

//void test_func()
//{
//    static int count = 0;
//
//    printf("count is %d\n", count++);
//}

//void init_sigaction()
//{
//    struct sigaction act;
//
//    act.sa_handler = reinterpret_cast<void (*)(int)>(test_func); //设置处理信号的函数
//    act.sa_flags  = 0;
//
//    sigemptyset(&act.sa_mask);
//    sigaction(SIGALRM, &act, NULL);//时间到发送SIGROF信号
//}

//void init_time()
//{
//    static struct itimerval val;
//
//    val.it_value.tv_sec = 5; //1秒后启用定时器
//    val.it_value.tv_usec = 0;
//
////    val.it_interval = val.it_value; //定时器间隔为1s
//
//    setitimer(ITIMER_REAL, &val, NULL);
//}

//int main(int argc, char **argv)
//{
//
//    init_sigaction();
//    init_time();
//    printf("1\n");
//    while(1);
//    uint8_t i = 0x1;
//    uint8_t j = 0xff;
//    if(i && j == 0xff){
//        printf("%d\n", i && j);
//    }
//    return 0;
//}
//__attribute__((constructor(1))) void before_main1(){
//    printf("before_main1\n");
//}
//__attribute__((constructor(2))) void before_main2(){
//    printf("before_main2\n");
//}
//__attribute__((destructor(1))) void after_main1(){
//    printf("after_main1\n");
//}
//__attribute__((destructor(2))) void after_main2(){
//    printf("after_main2\n");
//}

//int main() {
//    long index = 0;
//    while (1) {
//        long x = random();
//        if (index % 10000 == 0) {
//            cout << x << endl;
//            cout << index << endl;
//        }
//        if (x % 65536 > 65536) {
//            cout << "!!!" << endl;
//            break;
//        } else {
//            index++;
//        }
//    }
////    printf("main\n");
////    long x = 0x3fffffffffffffff;
////    cout << x%65536 <<endl;
//}
//class A{
//public:
//    A(){
//       printf("A构造\n");
//    }
//    ~A(){
//        printf("A析构\n");
//    }
//};
//
//class B{
//public:
//    std::unique_ptr<A> a_;
//    void init(){
//        a_ = std::make_unique<A>();
//    }
//};
//int main(){
//    B b;
//    b.init();
//    b.init();
//    pause();
//}

//class A1{
//public:
//    A1(){
//        printf("A1构造\n");
//    }
//    ~A1(){
//        printf("A1析构\n");
//    }
//};
//
//class A2: public A1{
//public:
//    A2(){
//        printf("A2构造\n");
//    }
//    virtual ~A2() {
//        printf("A2析构\n");
//    }
//};
//int main(){;
//    A1* A1 = new A2();
//    delete A1;
//}

//class B;
//
//class A {
//public:
//    A() {
//        printf("A构造\n");
//        b_ = std::make_unique<B>(this);
//    }
//    ~A(){
//        printf("A析构\n");
//    }
//    std::unique_ptr<B> b_;
//};
//
//class B {
//public:
//    B(A *a) {
//        printf("B构造\n");
//        a_ = a;
//    }
//    ~B(){
//        printf("B析构\n");
//    }
//    A *a_;
//};
//
//int main(){
//    A* a = new A();
//    delete a;
//    pause();
//}


//class A{
//public:
//    A(){
//        printf("A构造\n");
//    }
//    A operator=(const A& a){
//        printf("A拷贝赋值\n");
//        return a;
//    }
//    ~A(){
//        printf("A析构\n");
//    }
//};
//
//class B{
//public:
//    A a_;
//    void init(){
//        a_ = A();
//    }
//};
//int main(){
//    A a;
//    a = A();
//    shared_ptr<A> sp1(new A());
//    shared_ptr<A> sp2(sp1), sp3;
//    sp3 = sp1;
//一个典型的错误用法
//    shared_ptr<A> sp4(sp1.get());
//    pause();
//    B b;
//    b.init();
//    b.a_ = A();
//    pause();
//}
//int main(){
//    std::map<std::string, std::vector<uint8_t> > data_store_;
//    std::string key = "sakura";
//    std::vector<uint8_t> data = {1,2,3,4};
//    data_store_[key] = data;
//    cout << 1 <<endl;
//}

template<typename T>
class VectorBuffer {
public:
    constexpr VectorBuffer() = default;

    VectorBuffer(size_t count)
            : buffer_(reinterpret_cast<T *>(
                              malloc(sizeof(T) * count))),
              capacity_(count) {
    }

    VectorBuffer(VectorBuffer &&other) noexcept
            : buffer_(other.buffer_), capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    VectorBuffer(const VectorBuffer &) = delete;

    VectorBuffer &operator=(const VectorBuffer &) = delete;

    ~VectorBuffer() { free(buffer_); }

    VectorBuffer &operator=(VectorBuffer &&other) {
        free(buffer_);
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;

        other.buffer_ = nullptr;
        other.capacity_ = 0;
        return *this;
    }

    size_t capacity() const { return capacity_; }

    T &operator[](size_t i) {
        // TODO(crbug.com/817982): Some call sites (at least circular_deque.h) are
        // calling this with `i == capacity_` as a way of getting `end()`. Therefore
        // we have to allow this for now (`i <= capacity_`), until we fix those call
        // sites to use real iterators. This comment applies here and to `const T&
        // operator[]`, below.
        return buffer_[i];
    }

    const T &operator[](size_t i) const {
        return buffer_[i];
    }

    T *begin() { return buffer_; }

    T *end() { return &buffer_[capacity_]; }

    // DestructRange ------------------------------------------------------------

    // Trivially destructible objects need not have their destructors called.
    template<typename T2 = T,
            typename std::enable_if<std::is_trivially_destructible<T2>::value,
                    int>::type = 0>
    void DestructRange(T *begin, T *end) {}

    // Non-trivially destructible objects must have their destructors called
    // individually.
    template<typename T2 = T,
            typename std::enable_if<!std::is_trivially_destructible<T2>::value,
                    int>::type = 0>
    void DestructRange(T *begin, T *end) {
        assert(begin <= end);
        while (begin != end) {
            begin->~T();
            begin++;
        }
    }

    // MoveRange ----------------------------------------------------------------
    //
    // Not trivially copyable, but movable: call the move constructor and
    // destruct the original.
    static void MoveRange(T *from_begin, T *from_end, T *to) {
        while (from_begin != from_end) {
            new(to) T(std::move(*from_begin));
            from_begin->~T();
            from_begin++;
            to++;
        }
    }

private:

    T *buffer_ = nullptr;
    size_t capacity_ = 0;
};

template<class T>
class circular_deque;

constexpr size_t kCircularBufferInitialCapacity = 3;

template<typename T>
class circular_deque_const_iterator {
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const T *;
    using reference = const T &;
    using iterator_category = std::random_access_iterator_tag;

    circular_deque_const_iterator() : parent_deque_(nullptr), index_(0) {
    }

    // Dereferencing.
    const T &operator*() const {
        CheckUnstableUsage();
        parent_deque_->CheckValidIndex(index_);
        return parent_deque_->buffer_[index_];
    }

    const T *operator->() const {
        CheckUnstableUsage();
        parent_deque_->CheckValidIndex(index_);
        return &parent_deque_->buffer_[index_];
    }

    const value_type &operator[](difference_type i) const { return *(*this + i); }

    // Increment and decrement.
    circular_deque_const_iterator &operator++() {
        Increment();
        return *this;
    }

    circular_deque_const_iterator operator++(int) {
        circular_deque_const_iterator ret = *this;
        Increment();
        return ret;
    }

    circular_deque_const_iterator &operator--() {
        Decrement();
        return *this;
    }

    circular_deque_const_iterator operator--(int) {
        circular_deque_const_iterator ret = *this;
        Decrement();
        return ret;
    }

    // Random access mutation.
    friend circular_deque_const_iterator operator+(
            const circular_deque_const_iterator &iter,
            difference_type offset) {
        circular_deque_const_iterator ret = iter;
        ret.Add(offset);
        return ret;
    }

    circular_deque_const_iterator &operator+=(difference_type offset) {
        Add(offset);
        return *this;
    }

    friend circular_deque_const_iterator operator-(
            const circular_deque_const_iterator &iter,
            difference_type offset) {
        circular_deque_const_iterator ret = iter;
        ret.Add(-offset);
        return ret;
    }

    circular_deque_const_iterator &operator-=(difference_type offset) {
        Add(-offset);
        return *this;
    }

    friend std::ptrdiff_t operator-(const circular_deque_const_iterator &lhs,
                                    const circular_deque_const_iterator &rhs) {
        lhs.CheckComparable(rhs);
        return lhs.OffsetFromBegin() - rhs.OffsetFromBegin();
    }

    // Comparisons.
    friend bool operator==(const circular_deque_const_iterator &lhs,
                           const circular_deque_const_iterator &rhs) {
        lhs.CheckComparable(rhs);
        return lhs.index_ == rhs.index_;
    }

    friend bool operator!=(const circular_deque_const_iterator &lhs,
                           const circular_deque_const_iterator &rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const circular_deque_const_iterator &lhs,
                          const circular_deque_const_iterator &rhs) {
        lhs.CheckComparable(rhs);
        return lhs.OffsetFromBegin() < rhs.OffsetFromBegin();
    }

    friend bool operator<=(const circular_deque_const_iterator &lhs,
                           const circular_deque_const_iterator &rhs) {
        return !(lhs > rhs);
    }

    friend bool operator>(const circular_deque_const_iterator &lhs,
                          const circular_deque_const_iterator &rhs) {
        lhs.CheckComparable(rhs);
        return lhs.OffsetFromBegin() > rhs.OffsetFromBegin();
    }

    friend bool operator>=(const circular_deque_const_iterator &lhs,
                           const circular_deque_const_iterator &rhs) {
        return !(lhs < rhs);
    }

protected:
    friend class circular_deque<T>;

    circular_deque_const_iterator(const circular_deque<T> *parent, size_t index)
            : parent_deque_(parent), index_(index) {
    }

    // Returns the offset from the beginning index of the buffer to the current
    // item.
    size_t OffsetFromBegin() const {
        if (index_ >= parent_deque_->begin_)
            return index_ - parent_deque_->begin_;  // On the same side as begin.
        return parent_deque_->buffer_.capacity() - parent_deque_->begin_ + index_;
    }

    // Most uses will be ++ and -- so use a simplified implementation.
    void Increment() {
        CheckUnstableUsage();
        parent_deque_->CheckValidIndex(index_);
        index_++;
        if (index_ == parent_deque_->buffer_.capacity())
            index_ = 0;
    }

    void Decrement() {
        CheckUnstableUsage();
        parent_deque_->CheckValidIndexOrEnd(index_);
        if (index_ == 0)
            index_ = parent_deque_->buffer_.capacity() - 1;
        else
            index_--;
    }

    void Add(difference_type delta) {
        CheckUnstableUsage();

        // It should be valid to add 0 to any iterator, even if the container is
        // empty and the iterator points to end(). The modulo below will divide
        // by 0 if the buffer capacity is empty, so it's important to check for
        // this case explicitly.
        if (delta == 0)
            return;

        difference_type new_offset = OffsetFromBegin() + delta;
        index_ = (new_offset + parent_deque_->begin_) %
                 parent_deque_->buffer_.capacity();
    }


    inline void CheckUnstableUsage() const {}

    inline void CheckComparable(const circular_deque_const_iterator &) const {}

    const circular_deque<T> *parent_deque_;
    size_t index_;

};

template<typename T>
class circular_deque_iterator : public circular_deque_const_iterator<T> {
    using base = circular_deque_const_iterator<T>;

public:
    friend class circular_deque<T>;

    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::random_access_iterator_tag;

    // Expose the base class' constructor.
    circular_deque_iterator() : circular_deque_const_iterator<T>() {}

    // Dereferencing.
    T &operator*() const { return const_cast<T &>(base::operator*()); }

    T *operator->() const { return const_cast<T *>(base::operator->()); }

    T &operator[](difference_type i) {
        return const_cast<T &>(base::operator[](i));
    }

    // Random access mutation.
    friend circular_deque_iterator operator+(const circular_deque_iterator &iter,
                                             difference_type offset) {
        circular_deque_iterator ret = iter;
        ret.Add(offset);
        return ret;
    }

    circular_deque_iterator &operator+=(difference_type offset) {
        base::Add(offset);
        return *this;
    }

    friend circular_deque_iterator operator-(const circular_deque_iterator &iter,
                                             difference_type offset) {
        circular_deque_iterator ret = iter;
        ret.Add(-offset);
        return ret;
    }

    circular_deque_iterator &operator-=(difference_type offset) {
        base::Add(-offset);
        return *this;
    }

    // Increment and decrement.
    circular_deque_iterator &operator++() {
        base::Increment();
        return *this;
    }

    circular_deque_iterator operator++(int) {
        circular_deque_iterator ret = *this;
        base::Increment();
        return ret;
    }

    circular_deque_iterator &operator--() {
        base::Decrement();
        return *this;
    }

    circular_deque_iterator operator--(int) {
        circular_deque_iterator ret = *this;
        base::Decrement();
        return ret;
    }

private:
    circular_deque_iterator(const circular_deque<T> *parent, size_t index)
            : circular_deque_const_iterator<T>(parent, index) {}
};

template<typename T>
class circular_deque {
private:
    using VectorBuffer = VectorBuffer<T>;

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    using iterator = circular_deque_iterator<T>;
    using const_iterator = circular_deque_const_iterator<T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ---------------------------------------------------------------------------
    // Constructor

    constexpr circular_deque() = default;

    // Constructs with |count| copies of |value| or default constructed version.
    circular_deque(size_type count) { resize(count); }

    circular_deque(size_type count, const T &value) { resize(count, value); }

    // Range constructor.
    template<class InputIterator>
    circular_deque(InputIterator first, InputIterator last) {
        assign(first, last);
    }

    // Copy/move.
    circular_deque(const circular_deque &other) : buffer_(other.size() + 1) {
        assign(other.begin(), other.end());
    }

    circular_deque(circular_deque &&other) noexcept
            : buffer_(std::move(other.buffer_)),
              begin_(other.begin_),
              end_(other.end_) {
        other.begin_ = 0;
        other.end_ = 0;
    }

    circular_deque(std::initializer_list<value_type> init) { assign(init); }

    ~circular_deque() { DestructRange(begin_, end_); }

    // ---------------------------------------------------------------------------
    // Assignments.
    //
    // All of these may invalidate iterators and references.

    circular_deque &operator=(const circular_deque &other) {
        if (&other == this)
            return *this;

        reserve(other.size());
        assign(other.begin(), other.end());
        return *this;
    }

    circular_deque &operator=(circular_deque &&other) noexcept {
        if (&other == this)
            return *this;

        // We're about to overwrite the buffer, so don't free it in clear to
        // avoid doing it twice.
        ClearRetainCapacity();
        buffer_ = std::move(other.buffer_);
        begin_ = other.begin_;
        end_ = other.end_;

        other.begin_ = 0;
        other.end_ = 0;

        IncrementGeneration();
        return *this;
    }

    circular_deque &operator=(std::initializer_list<value_type> ilist) {
        reserve(ilist.size());
        assign(std::begin(ilist), std::end(ilist));
        return *this;
    }

    void assign(size_type count, const value_type &value) {
        ClearRetainCapacity();
        reserve(count);
        for (size_t i = 0; i < count; i++)
            emplace_back(value);
        IncrementGeneration();
    }


    void assign(std::initializer_list<value_type> value) {
        reserve(std::distance(value.begin(), value.end()));
        assign(value.begin(), value.end());
    }

    // ---------------------------------------------------------------------------
    // Accessors.
    //
    // Since this class assumes no exceptions, at() and operator[] are equivalent.

    const value_type &at(size_type i) const {
        size_t right_size = buffer_.capacity() - begin_;
        if (begin_ <= end_ || i < right_size)
            return buffer_[begin_ + i];
        return buffer_[i - right_size];
    }

    const value_type &operator[](size_type i) const { return at(i); }

    value_type &front() {
        return buffer_[begin_];
    }

    const value_type &front() const {
        return buffer_[begin_];
    }

    value_type &back() {
        return *(--end());
    }

    const value_type &back() const {
        return *(--end());
    }

    // ---------------------------------------------------------------------------
    // Iterators.

    iterator begin() { return iterator(this, begin_); }

    const_iterator begin() const { return const_iterator(this, begin_); }

    const_iterator cbegin() const { return const_iterator(this, begin_); }

    iterator end() { return iterator(this, end_); }

    const_iterator end() const { return const_iterator(this, end_); }

    const_iterator cend() const { return const_iterator(this, end_); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const { return rbegin(); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const { return rend(); }

    // ---------------------------------------------------------------------------
    // Memory management.

    // IMPORTANT NOTE ON reserve(...): This class implements auto-shrinking of
    // the buffer when elements are deleted and there is "too much" wasted space.
    // So if you call reserve() with a large size in anticipation of pushing many
    // elements, but pop an element before the queue is full, the capacity you
    // reserved may be lost.
    //
    // As a result, it's only worthwhile to call reserve() when you're adding
    // many things at once with no intermediate operations.
    void reserve(size_type new_capacity) {
        if (new_capacity > capacity())
            SetCapacityTo(new_capacity);
    }

    size_type capacity() const {
        // One item is wasted to indicate end().
        return buffer_.capacity() == 0 ? 0 : buffer_.capacity() - 1;
    }

    void shrink_to_fit() {
        if (empty()) {
            // Optimize empty case to really delete everything if there was
            // something.
            if (buffer_.capacity())
                buffer_ = VectorBuffer();
        } else {
            SetCapacityTo(size());
        }
    }

    // ---------------------------------------------------------------------------
    // Size management.

    // This will additionally reset the capacity() to 0.
    void clear() {
        // This can't resize(0) because that requires a default constructor to
        // compile, which not all contained classes may implement.
        ClearRetainCapacity();
        buffer_ = VectorBuffer();
    }

    bool empty() const { return begin_ == end_; }

    size_type size() const {
        if (begin_ <= end_)
            return end_ - begin_;
        return buffer_.capacity() - begin_ + end_;
    }

    // When reducing size, the elements are deleted from the end. When expanding
    // size, elements are added to the end with |value| or the default
    // constructed version. Even when using resize(count) to shrink, a default
    // constructor is required for the code to compile, even though it will not
    // be called.
    //
    // There are two versions rather than using a default value to avoid
    // creating a temporary when shrinking (when it's not needed). Plus if
    // the default constructor is desired when expanding usually just calling it
    // for each element is faster than making a default-constructed temporary and
    // copying it.
    void resize(size_type count) {
        // SEE BELOW VERSION if you change this. The code is mostly the same.
        if (count > size()) {
            // This could be slighly more efficient but expanding a queue with
            // identical elements is unusual and the extra computations of emplacing
            // one-by-one will typically be small relative to calling the constructor
            // for every item.
            ExpandCapacityIfNecessary(count - size());
            while (size() < count)
                emplace_back();
        } else if (count < size()) {
            size_t new_end = (begin_ + count) % buffer_.capacity();
            DestructRange(new_end, end_);
            end_ = new_end;

            ShrinkCapacityIfNecessary();
        }
        IncrementGeneration();
    }

    void resize(size_type count, const value_type &value) {
        // SEE ABOVE VERSION if you change this. The code is mostly the same.
        if (count > size()) {
            ExpandCapacityIfNecessary(count - size());
            while (size() < count)
                emplace_back(value);
        } else if (count < size()) {
            size_t new_end = (begin_ + count) % buffer_.capacity();
            DestructRange(new_end, end_);
            end_ = new_end;

            ShrinkCapacityIfNecessary();
        }
        IncrementGeneration();
    }
    // ---------------------------------------------------------------------------
    // Begin/end operations.

    void push_front(const T &value) { emplace_front(value); }

    void push_front(T &&value) { emplace_front(std::move(value)); }

    void push_back(const T &value) { emplace_back(value); }

    void push_back(T &&value) { emplace_back(std::move(value)); }

    template<class... Args>
    reference emplace_front(Args &&... args) {
        ExpandCapacityIfNecessary(1);
        if (begin_ == 0)
            begin_ = buffer_.capacity() - 1;
        else
            begin_--;
        IncrementGeneration();
        new(&buffer_[begin_]) T(std::forward<Args>(args)...);
        return front();
    }

    template<class... Args>
    reference emplace_back(Args &&... args) {
        ExpandCapacityIfNecessary(1);
        new(&buffer_[end_]) T(std::forward<Args>(args)...);
        if (end_ == buffer_.capacity() - 1)
            end_ = 0;
        else
            end_++;
        IncrementGeneration();
        return back();
    }

    void pop_front() {
        buffer_.DestructRange(&buffer_[begin_], &buffer_[begin_ + 1]);
        begin_++;
        if (begin_ == buffer_.capacity())
            begin_ = 0;
        ShrinkCapacityIfNecessary();

        // Technically popping will not invalidate any iterators since the
        // underlying buffer will be stable. But in the future we may want to add a
        // feature that resizes the buffer smaller if there is too much wasted
        // space. This ensures we can make such a change safely.
        IncrementGeneration();
    }

    void pop_back() {
        if (end_ == 0)
            end_ = buffer_.capacity() - 1;
        else
            end_--;
        buffer_.DestructRange(&buffer_[end_], &buffer_[end_ + 1]);

        ShrinkCapacityIfNecessary();

        // See pop_front comment about why this is here.
        IncrementGeneration();
    }


private:
    friend circular_deque_iterator<T>;
    friend circular_deque_const_iterator<T>;

    // Moves the items in the given circular buffer to the current one. The
    // source is moved from so will become invalid. The destination buffer must
    // have already been allocated with enough size.
    static void MoveBuffer(VectorBuffer &from_buf,
                           size_t from_begin,
                           size_t from_end,
                           VectorBuffer *to_buf,
                           size_t *to_begin,
                           size_t *to_end) {
        size_t from_capacity = from_buf.capacity();

        *to_begin = 0;
        if (from_begin < from_end) {
            // Contiguous.
            from_buf.MoveRange(&from_buf[from_begin], &from_buf[from_end],
                               to_buf->begin());
            *to_end = from_end - from_begin;
        } else if (from_begin > from_end) {
            // Discontiguous, copy the right side to the beginning of the new buffer.
            from_buf.MoveRange(&from_buf[from_begin], &from_buf[from_capacity],
                               to_buf->begin());
            size_t right_size = from_capacity - from_begin;
            // Append the left side.
            from_buf.MoveRange(&from_buf[0], &from_buf[from_end],
                               &(*to_buf)[right_size]);
            *to_end = right_size + from_end;
        } else {
            // No items.
            *to_end = 0;
        }
    }

    // Expands the buffer size. This assumes the size is larger than the
    // number of elements in the vector (it won't call delete on anything).
    void SetCapacityTo(size_t new_capacity) {
        printf("new external Capacity is %d\n", new_capacity);
        if (new_capacity == 206) {
            printf("yes\n");
            exit(0);
        }
        if (new_capacity > 500) {
            printf("no\n");
            exit(-1);
        }
        // Use the capacity + 1 as the internal buffer size to differentiate
        // empty and full (see definition of buffer_ below).
        VectorBuffer new_buffer(new_capacity + 1);
        MoveBuffer(buffer_, begin_, end_, &new_buffer, &begin_, &end_);
        buffer_ = std::move(new_buffer);
    }

    void ExpandCapacityIfNecessary(size_t additional_elts) {
        size_t min_new_capacity = size() + additional_elts;
        if (capacity() >= min_new_capacity)
            return;  // Already enough room.

        min_new_capacity =
                std::max(min_new_capacity, kCircularBufferInitialCapacity);

        // std::vector always grows by at least 50%. WTF::Deque grows by at least
        // 25%. We expect queue workloads to generally stay at a similar size and
        // grow less than a vector might, so use 25%.
        size_t new_capacity =
                std::max(min_new_capacity, capacity() + capacity() / 4);
        printf("Expand: size is:%d;capacity() is %d\n", size(), capacity());
        SetCapacityTo(new_capacity);
    }

    void ShrinkCapacityIfNecessary() {
        // Don't auto-shrink below this size.
        if (capacity() <= kCircularBufferInitialCapacity)
            return;

        // Shrink when 100% of the size() is wasted.
        size_t sz = size();
        size_t empty_spaces = capacity() - sz;
        if (empty_spaces < sz)
            return;

        // Leave 1/4 the size as free capacity, not going below the initial
        // capacity.
        size_t new_capacity =
                std::max(kCircularBufferInitialCapacity, sz + sz / 4);
        if (new_capacity < capacity()) {
            // Count extra item to convert to internal capacity.
            printf("Shrink: size is %d;capacity() is %d\n", size(), capacity());
            SetCapacityTo(new_capacity);
        }
    }

    // Backend for clear() but does not resize the internal buffer.
    void ClearRetainCapacity() {
        // This can't resize(0) because that requires a default constructor to
        // compile, which not all contained classes may implement.
        DestructRange(begin_, end_);
        begin_ = 0;
        end_ = 0;
        IncrementGeneration();
    }

    // Calls destructors for the given begin->end indices. The indices may wrap
    // around. The buffer is not resized, and the begin_ and end_ members are
    // not changed.
    void DestructRange(size_t begin, size_t end) {
        if (end == begin) {
            return;
        } else if (end > begin) {
            buffer_.DestructRange(&buffer_[begin], &buffer_[end]);
        } else {
            buffer_.DestructRange(&buffer_[begin], &buffer_[buffer_.capacity()]);
            buffer_.DestructRange(&buffer_[0], &buffer_[end]);
        }
    }


    // No-op versions of these functions for release builds.
    void CheckValidIndex(size_t) const {}

    void CheckValidIndexOrEnd(size_t) const {}

    void ValidateIterator(const const_iterator &i) const {}

    void IncrementGeneration() {}

    // Danger, the buffer_.capacity() is the "internal capacity" which is
    // capacity() + 1 since there is an extra item to indicate the end. Otherwise
    // being compintely empty and compintely full are indistinguishable (begin ==
    // end). We could add a separate flag to avoid it, but that adds significant
    // extra complexity since every computation will have to check for it. Always
    // keeping one extra unused element in the buffer makes iterator computations
    // much simpler.
    //
    // Container internal code will want to use buffer_.capacity() for offset
    // computations rather than capacity().
    VectorBuffer buffer_;
    size_type begin_ = 0;
    size_type end_ = 0;

};


int main() {
    circular_deque<uint64_t> c;
    int val = 1;
    for (int i = 0; i < 97; i++)
        c.push_back(val);
    c.push_back(val);
    for (int i = 0; i < 49; i++)
        c.pop_front();
    // current capacity: 60, current size: 48
    for (int i = 0; i < 93 - 48; i++)
        c.push_back(val);
    for (int i = 0; i < 47; i++)
        c.pop_front();
    // current capacity: 57, current size: 46
    for (int i = 0; i < 88 - 46; i++)
        c.push_back(val);
    for (int i = 0; i < 44; i++)
        c.pop_front();
    // current capacity: 55, current size: 44
    for (int i = 0; i < 206 - 44 - 1; i++)
        c.push_back(val);
}