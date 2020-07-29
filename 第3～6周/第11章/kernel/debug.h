#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
void panic_spin(char* filename, int line, const char* func, const char* condition);

/***************************  __VA_ARGS__  *******************************
 * __VA_ARGS__ 是预处理器所支持的专用标识符。
 * 代表所有与省略号相对应的参数.
 * "..."表示定义的宏其参数可变.*/
#define PANIC(...) panic_spin (__FILE__, __LINE__, __func__, __VA_ARGS__)
 /***********************************************************************/

#ifdef NDEBUG
   #define ASSERT(CONDITION) ((void)0)
#else
   #define ASSERT(CONDITION)                                      \
      if (CONDITION) {} else {                                    \
  /* 符号#让编译器将宏的参数转化为字符串字面量 */          \
     PANIC(#CONDITION);                                       \
      }
#endif /*__NDEBUG */

#endif /*__KERNEL_DEBUG_H*/


/*
 #define PANIC(...) panic_spin (__FILE__, __LINE__, __func__, __VA_ARGS__)
 
 预处理器为形式参数（也就是PANIC的参数省略号）提供了一个标识符__VA_ARGS__，它只允许在具有可变参数的宏替换列表中出现， 它代表所有与省略号“...”相对应的参数。该参数至少有一个，但可以为空。
 所以，我们传给 panic_spin 的其中一个参数是__VA_ARGS__。同样作为参数的还有__FILE__，__LINE__， __func__，这三个是预定义的宏，分别表示被编译的文件名、被编译文件中的行号、被编译的函数名。
 咱们再看一下第 18 行，调用 PANIC 的形式为 PANIC(#CONDITION) ，即形参为#CONDITION，其中字 符’#’的作用是让预处理器把CONDITION转换成字符串常量。比如CONDITION若为var != 0，#CONDITION 的效果是变成了字符串“var != 0”。
 于是，传给 panic_spin 函数的第 4 个参数__VA_ARGS__，实际类型为字符串指针。
 */
