## Member

Member<T>表示对类型的对象的强引用T，这意味着只要所有者类实例处于活动状态，被引用的对象就会保持活动状态。

```
template <typename T,
          TracenessMemberConfiguration tracenessConfiguration =
              TracenessMemberConfiguration::kTraced>
class MemberBase {
  DISALLOW_NEW();

 public:
  MemberBase() : raw_(nullptr) { SaveCreationThreadState(); }

  MemberBase(std::nullptr_t) : raw_(nullptr) { SaveCreationThreadState(); }

  explicit MemberBase(T* raw) : raw_(raw) {
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  explicit MemberBase(T& raw) : raw_(&raw) {
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  MemberBase(WTF::HashTableDeletedValueType)
      : raw_(reinterpret_cast<T*>(kHashTableDeletedRawValue)) {
    SaveCreationThreadState();
  }

  MemberBase(const MemberBase& other) : raw_(other) {
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  template <typename U>
  MemberBase(const Persistent<U>& other) : raw_(other) {
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  template <typename U>
  MemberBase(const MemberBase<U>& other) : raw_(other) {
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  template <typename U>
  MemberBase& operator=(const Persistent<U>& other) {
    SetRaw(other);
    CheckPointer();
    WriteBarrier();
    return *this;
  }

  MemberBase& operator=(const MemberBase& other) {
    SetRaw(other);
    CheckPointer();
    WriteBarrier();
    return *this;
  }

  template <typename U>
  MemberBase& operator=(const MemberBase<U>& other) {
    SetRaw(other);
    CheckPointer();
    WriteBarrier();
    return *this;
  }

  template <typename U>
  MemberBase& operator=(U* other) {
    SetRaw(other);
    CheckPointer();
    WriteBarrier();
    return *this;
  }

  MemberBase& operator=(WTF::HashTableDeletedValueType) {
    SetRaw(reinterpret_cast<T*>(-1));
    return *this;
  }

  MemberBase& operator=(std::nullptr_t) {
    SetRaw(nullptr);
    return *this;
  }

  void Swap(MemberBase<T>& other) {
    T* tmp = GetRaw();
    SetRaw(other.GetRaw());
    other.SetRaw(tmp);
    CheckPointer();
    WriteBarrier();
    other.WriteBarrier();
  }

  explicit operator bool() const { return GetRaw(); }
  operator T*() const { return GetRaw(); }
  T* operator->() const { return GetRaw(); }
  T& operator*() const { return *GetRaw(); }

  T* Get() const { return GetRaw(); }

  void Clear() { SetRaw(nullptr); }

  T* Release() {
    T* result = GetRaw();
    SetRaw(nullptr);
    return result;
  }

  static bool IsMemberHashTableDeletedValue(const T* t) {
    return t == reinterpret_cast<T*>(kHashTableDeletedRawValue);
  }

  bool IsHashTableDeletedValue() const {
    return IsMemberHashTableDeletedValue(GetRaw());
  }

 protected:
  static constexpr intptr_t kHashTableDeletedRawValue = -1;

  enum class AtomicCtorTag { Atomic };

  // MemberBase ctors that use atomic write to set raw_.

  MemberBase(AtomicCtorTag, T* raw) {
    SetRaw(raw);
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  MemberBase(AtomicCtorTag, T& raw) {
    SetRaw(&raw);
    SaveCreationThreadState();
    CheckPointer();
    // No write barrier for initializing stores.
  }

  void WriteBarrier() const {
    MarkingVisitor::WriteBarrier(
        reinterpret_cast<void**>(const_cast<std::remove_const_t<T>**>(&raw_)));
  }

  void CheckPointer() {
#if DCHECK_IS_ON()
    // Should not be called for deleted hash table values. A value can be
    // propagated here if a MemberBase containing the deleted value is copied.
    if (IsHashTableDeletedValue())
      return;
    pointer_verifier_.CheckPointer(GetRaw());
#endif  // DCHECK_IS_ON()
  }

  void SaveCreationThreadState() {
#if DCHECK_IS_ON()
    pointer_verifier_.SaveCreationThreadState(GetRaw());
#endif  // DCHECK_IS_ON()
  }

  ALWAYS_INLINE void SetRaw(T* raw) {
    if (tracenessConfiguration == TracenessMemberConfiguration::kUntraced)
      raw_ = raw;
    else
      WTF::AsAtomicPtr(&raw_)->store(raw, std::memory_order_relaxed);
  }
  ALWAYS_INLINE T* GetRaw() const { return raw_; }

 private:
  // Thread safe version of Get() for marking visitors.
  // This is used to prevent data races between concurrent marking visitors
  // and writes on the main thread.
  const T* GetSafe() const {
    // TOOD(omerkatz): replace this cast with std::atomic_ref (C++20) once it
    // becomes available
    return WTF::AsAtomicPtr(&raw_)->load(std::memory_order_relaxed);
  }

  T* raw_;
#if DCHECK_IS_ON()
  MemberPointerVerifier<T, tracenessConfiguration> pointer_verifier_;
#endif  // DCHECK_IS_ON()

  friend class Visitor;
};
```
 
```
template <typename T>
class Member : public MemberBase<T, TracenessMemberConfiguration::kTraced> {
  DISALLOW_NEW();
  typedef MemberBase<T, TracenessMemberConfiguration::kTraced> Parent;

 public:
  Member() : Parent() {}
  Member(std::nullptr_t) : Parent(nullptr) {}
  Member(T* raw) : Parent(raw) {}
  Member(T& raw) : Parent(raw) {}
  Member(WTF::HashTableDeletedValueType x) : Parent(x) {}

  Member(const Member& other) : Parent(other) {}

  template <typename U>
  Member(const Member<U>& other) : Parent(other) {}

  template <typename U>
  Member(const Persistent<U>& other) : Parent(other) {}

  template <typename U>
  Member& operator=(const Persistent<U>& other) {
    Parent::operator=(other);
    return *this;
  }

  Member& operator=(const Member& other) {
    Parent::operator=(other);
    return *this;
  }

  template <typename U>
  Member& operator=(const Member<U>& other) {
    Parent::operator=(other);
    return *this;
  }

  template <typename U>
  Member& operator=(const WeakMember<U>& other) {
    Parent::operator=(other);
    return *this;
  }

  template <typename U>
  Member& operator=(U* other) {
    Parent::operator=(other);
    return *this;
  }

  Member& operator=(WTF::HashTableDeletedValueType x) {
    Parent::operator=(x);
    return *this;
  }

  Member& operator=(std::nullptr_t) {
    Parent::operator=(nullptr);
    return *this;
  }

 private:
  using typename Parent::AtomicCtorTag;
  Member(AtomicCtorTag atomic, T* raw) : Parent(atomic, raw) {}
  Member(AtomicCtorTag atomic, T& raw) : Parent(atomic, raw) {}

  template <typename P, typename Traits, typename Allocator>
  friend class WTF::MemberConstructTraits;
};
```

## WeakMember

WeakMember<T>是对类型对象的弱引用T。与不同Member<T>，WeakMember<T>它不会使指向对象保持活动状态。
```
template <typename T>
class WeakMember : public MemberBase<T, TracenessMemberConfiguration::kTraced> {
  typedef MemberBase<T, TracenessMemberConfiguration::kTraced> Parent;

 public:
  WeakMember() : Parent() {}

  WeakMember(std::nullptr_t) : Parent(nullptr) {}

  WeakMember(T* raw) : Parent(raw) {}

  WeakMember(WTF::HashTableDeletedValueType x) : Parent(x) {}

  template <typename U>
  WeakMember(const Persistent<U>& other) : Parent(other) {}

  template <typename U>
  WeakMember(const Member<U>& other) : Parent(other) {}

  template <typename U>
  WeakMember& operator=(const Persistent<U>& other) {
    Parent::operator=(other);
    return *this;
  }

  template <typename U>
  WeakMember& operator=(const Member<U>& other) {
    Parent::operator=(other);
    return *this;
  }

  template <typename U>
  WeakMember& operator=(U* other) {
    Parent::operator=(other);
    return *this;
  }

  WeakMember& operator=(std::nullptr_t) {
    this->SetRaw(nullptr);
    return *this;
  }

 private:
  using typename Parent::AtomicCtorTag;
  WeakMember(AtomicCtorTag atomic, T* raw) : Parent(atomic, raw) {}
  WeakMember(AtomicCtorTag atomic, T& raw) : Parent(atomic, raw) {}

  template <typename P, typename Traits, typename Allocator>
  friend class WTF::MemberConstructTraits;
};
```