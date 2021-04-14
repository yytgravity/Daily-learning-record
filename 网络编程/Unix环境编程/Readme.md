## 第三章 文件IO

### 文件描述符相关(重点)

- open函数
- close函数
- write函数
- read函数
- lseek函数
- dup\dup2函数
- fcntl函数

```c
//fcntl函数常用的操作:

//1复制一个新的文件描述符:
int newfd = fcntl(fd, F_DUPFD, 0);
//2获取文件的属性标志
int flag = fcntl(fd, F_GETFL, 0)
//3设置文件状态标志
flag = flag | O_APPEND;
fcntl(fd, F_SETFL, flag)

//4常用的属性标志
O_APPEND-----设置文件打开为末尾添加
O_NONBLOCK-----设置打开的文件描述符为非阻塞

```

- perror函数

### 文件目录相关(非重点)

- stat、lstat函数及其区别
- opendir函数
- readdir函数
- closedir函数

读取目录一般操作

```c
 DIR *pDir = opendir(“dir”);   //打开目录
 while((p=readdir(pDir))!=NULL){
   //循环读取文件
 }  
 closedir(pDir);  //关闭目录

```