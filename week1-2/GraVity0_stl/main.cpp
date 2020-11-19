#include <iostream>
#include <deque>
using namespace std;
int main()
{
    deque<int>d;
    d.push_back(1);
    auto first = d.begin();
    cout << *first << endl;
    //添加元素，会导致 first 失效
    d.push_back(2);
    cout << *first << endl;
    return 0;
}


/*
#include <iostream>
#include<vector>
#include<list>
#include<deque>
#include <queue>
#include <stack>
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<cstring>
using namespace::std;

int main()
{
 
    // Creating a sample vector
    vector <int> v = {1, 5, 10, 15, 20};
 
    // Changing vector while iterating over it
    // (This causes iterator invalidation)
    for (auto it = v.begin(); it != v.end(); it++)
        if ((*it) == 5)
            v.push_back(-1);
 
    for (auto it = v.begin(); it != v.end(); it++)
        cout << (*it) << " ";
         
    return 0;
}
*/

/*
#include<iostream>
#include "stl_vector.h"
int main() {
    // 创建含整数的 list
    yyt_stl::vector<int> v(5);
//    sakura_stl::list<int> l2(l);
    for (auto it = v.begin(); it != v.end(); it++)
            std::cout << (*it) << " ";
}
*/
