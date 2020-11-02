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
          vector.count = 0;
          vector.items = nullptr;
      }
      ~Vector() {
          Clear();
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
          auto _first = items;
          auto _finish = items + count;
          
          for(; _first != _finish ; _first++)
          {
              _first->~T();
          }
          free(items);
          count = 0;
          items = nullptr;
      }

      void Add(const T& item) {
          T* new_items = static_cast<T*>(malloc(sizeof(T) * (count + 1)));
          if(!new_items)
          {
              cout<<"malloc error"<<endl;
          }
          auto new_count = count + 1;
          for(int i=0;i<count;i++)
          {
              new(new_items + i) T(move (*(items + i)));
          }
          new(new_items + count) T(move(item));
          Clear();
          items = new_items;
          count = new_count;
      }
      bool Insert(const T& item,int index) {
          auto new_items = static_cast<T*>(malloc(sizeof(T) * (count + 1)));
          auto new_count = count + 1;
          if (index > count || index < 0)
              return false;
          for (int i = 0; i < new_count; i++) {
              if(i<index)
                  new (new_items + i) T(move(*(items + i)));
              else if(i>index)
                  new (new_items + i) T(move(*(items + i-1)));
              else
                  new (new_items + index) T(item);
          }
          Clear();
          items = new_items;
          count = new_count;
          return true;
      }
      bool Remove(int index) {
          auto new_items = static_cast<T*>(malloc(sizeof(T) * (count - 1)));
          auto new_count = count - 1;
          if (index >= count || index < 0)
              return false;
          for (int i = 0; i < new_count; i++) {
              if (i < index)
                  new (new_items + i) T(*(move(items + i)));
              else
                  new (new_items + i) T(move(*(items + i + 1)));
          }
          Clear();
          items = new_items;
          count = new_count;
          return true;
      }
      int Contains(const T& item) {
          for(int i = 0 ; i < count ; i++) {
              if(*(items + i) == item)
                  return i;
          }
          return -1;
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

