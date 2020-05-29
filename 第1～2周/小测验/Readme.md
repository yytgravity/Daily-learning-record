这一次我们要用之前学习过的关于右值引用和std::move的相关知识，自行实现一个Vector。除此之外，我们还会向大家介绍一个新的知识：“定位放置（placement new)”。

对于已经学习过《C++ 程序设计》的同学，应该知道new运算符的使用方法——而placement new则是new运算符的一种变种。跟 C 语言中的malloc函数不同，new的作用是，首先分配一片大小足够的内存，然后在这片内存上调用指定的构造函数，构造一个对象。而placement new则是在指定的一片分配好的内存上，对对象进行构造。

在这一节，我们将会使用placement new，配合std::move来进行对象构造，并且完成一个Vector类——在《C++ 程序设计》最后一章的实验中，大家应该已经试着自己实现过一个Vector类。而这一节我们要实现的Vector类则更加复杂，功能更多，而且效率更高。

placement new的使用方法如下所示：

```
new(位置)Object(构造函数的参数)
```
具体到这次我们要实现的Vector类，在测试中，我们会使用它来填充元素Element——它在调用不同的构造函数的时候，会输出不同的文字信息。大家需要正确使用std::move，调用正确的构造函数，才能得到正确的结果。

Element类的定义，Vector类的框架与几个构造函数，都已经给出——其中Vector类包含一个指针变量成员items和一个整形变量成员count，分别用来保存堆对象的位置，以及当前已经储存的元素个数。大家需要实现Vector的右值引用构造函数与析构函数，以及以下几个成员函数：

void Add(const T& item)这个函数类似于std::vector提供的push_back方法，作用是向当前元素数组的末尾添加一个新的元素——如果内存空间不足的话你就需要对内存进行重新分配，不能让Vector类的使用者自己去考虑数组是否会越界的问题。
bool Insert(const T& item,int index)向索引index的位置插入元素——然后将index后面的元素依次向后移动一个位置。同样，你需要自己处理内存空间问题。除此之外，你还要检查索引是否合法——如果合法则返回true，不合法则直接返回false，不执行任何操作。
bool Remove(int index)删除索引index位置的元素，并且将index后面的所有元素向前移动一位。其他细节跟Insert函数保持一致。
int Contains(const T& item)遍历整个数组，看是否包含元素item。如果包含则返回item的索引，如果不包含则返回-1
void Clear()清除Vector的所有元素，将指针置为nullptr，将计数器归零——注意为了避免内存泄漏，Clear()和Vector的析构函数必须要先调用所有元素的析构函数再对堆内存进行free
所有成员函数在修改了元素个数的时候，都必须同步更新count。

主函数、Element和Vector框架都已经给出并锁定，你不能更改它们。

样例输入
 
样例输出
```
ctor
copy ctor
dtor
ctor
right value ctor
copy ctor
dtor
dtor
ctor
right value ctor
right value ctor
copy ctor
dtor
dtor
dtor
ctor
right value ctor
right value ctor
right value ctor
copy ctor
dtor
dtor
dtor
dtor
0 1 2 3 
ctor
right value ctor
right value ctor
copy ctor
right value ctor
right value ctor
dtor
dtor
dtor
dtor
0 1 4 2 3 
right value ctor
right value ctor
right value ctor
right value ctor
dtor
dtor
dtor
dtor
dtor
0 1 2 3 
ctor
ctor
1
-1
0 1 2 3 
copy ctor
1 
dtor
dtor
dtor
dtor
dtor
dtor
dtor
dtor
dtor
dtor
dtor
dtor
```
```
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;
class Element {
private:
    int number;
public:
	Element() :number(0) {
	    cout << "ctor" << endl;
	}
  	Element(int num):number(num) {
  		cout << "ctor" << endl;
  	}
  	Element(const Element& e):number(e.number) {
  		cout << "copy ctor" << endl;
  	}
  	Element(Element&& e):number(e.number) {
  		cout << "right value ctor" << endl;
  	}
  	~Element() {
  		cout << "dtor" << endl;
  	}
  	void operator=(const Element& item) {
  		number = item.number;
  	}
  	bool operator==(const Element& item) {
  		return (number == item.number);
  	}
  	void operator()() {
  		cout << number ;
  	}
  	int GetNumber() {
  		return number;
  	}
};
template<typename T>
class Vector {
private:
  	T* items;
  	int count;
public:
  	Vector() :count{ 0 }, items{nullptr} {

  	}
  	Vector(const Vector& vector) :count{vector.count} {
  		items = static_cast<T*>(malloc(sizeof(T) * count));
  		memcpy(items, vector.items, sizeof(T) * count);
  	}
  	Vector(Vector&& vector) :count{ vector.count }, items{ vector.items } {
  		//TODO
  	}
  	~Vector() {
  		//TODO
  	}
    T& operator[](int index){
    	if (index<0||index>=count) {
    		cout<<"invalid index"<<endl;
    		return items[0];
    	}
    	return items[index];
    }
    int returnCount(){
    	return count;
    }
  	void Clear() {
  		//TODO
  	}

  	void Add(const T& item) {
  		//TODO
  	}
  	bool Insert(const T& item,int index) {
  		//TODO
  	}
  	bool Remove(int index) {
  		//TODO
  	}
  	int Contains(const T& item) {
  		//TODO
  	}
};
template<typename T>
void PrintVector(Vector<T>& v){
	  int count=v.returnCount();
	  for (int i = 0; i < count; i++)
	  {
		  v[i]();
		  cout << " ";
	  }
	  cout << endl;
}
int main() {
  	Vector<Element>v;
  	for (int i = 0; i < 4; i++) {
  		Element e(i);
  		v.Add(e);
  	}
  	PrintVector(v);
  	Element e2(4);
  	if (!v.Insert(e2, 10))
  	{
  		v.Insert(e2, 2);
  	}
  	PrintVector(v);
  	if (!v.Remove(10))
  	{
  		v.Remove(2);
  	}
  	PrintVector(v);
  	Element e3(1), e4(10);
  	cout << v.Contains(e3) << endl;
  	cout << v.Contains(e4) << endl;
  	Vector<Element>v2(v);
  	Vector<Element>v3(move(v2));
  	PrintVector(v3);
  	v2.Add(e3);
  	PrintVector(v2);
  	return 0;
}
```