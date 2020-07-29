# frida从入门到放弃


主要分为两部分：frida基础和frida hook fuzz尝试

## Frida基础

### 环境
- 不说了，翻墙 pip即可

### 工具

- fride
  - 类似python等脚本解释器的REPL

- frida-ps
  - 列出可附加的进程/app列表

- frida-trace
  - 根据 glob 匹配符号并⾃自动⽣生成 hook 代码框架 修改 \_\_handlers__ 中的脚本后会⾃自动重新载⼊入   

- frida-ls-devices
  - 列出可用的设备

- frida-kill
  - 杀进程

- frida-discover
  - 记录一段时间内各线程调用的函数和符号名

### 基本框架

```
import frida
def on_message(message, data):
    print("[%s] => %s" % (message, data))
session = frida.attach('xxx')
script = session.create_script('some js code here')
script.on('message', on_message)
script.load()
sys.stdin.read()
session.deatch()
```     
- 在这段代码中，开始的部分定义了一个python函数-on_message，其功能是将参数输出到控制台，这两个参数是python回调接口需要的。

- 第5行是调用python的接口Session.create_script，其功能是根据字符串创建一个script对象，其参数便是js脚本代码。
- 第六行script.on(‘message’ , on_message)则是将这段脚本和自己自定义的on_message联系在了一起，之后通过下面的script.load来执行js脚本。

### python api

- \_init_.py里面有一些全局接口
  ![](./img/1.png)
  
- core.py里面则储存了其他接口
  ![](./img/2.png)
  
##### 设备api

- 获取设备

```
all_devices = frida.enumerate_devices()
local = frida.get_local_device()
usb = frida.get_usb_device()
remote = frida.get_device_manger().add_remote_device(ip)
```

- 设备事件处理

```
device_manger = frida.get_device_manger()
device_manger.on('changed', on_changed)  #listen
device_manger.off('changed', on_changed)  #remove listener
```

- 监听设备拔插

```
device_manger.on('add', on_changed)
device_manger.on('changed', on_changed)
device_manger.on('remove', on_removed)
```

- 进程管理

```
pid = device.spawn('com.apple.mobilesafari')
device.resume(pid)
device.kill(pid)
```

- APP信息

```
device.enumerate_applications()
```

##### spawn
spawn和attach的区别：
- spawn启动新的实例
    - 可指定启动参数
    - 支持在进程初始化之前做一些操作
    - ios上如果已经app运行（包括后台休眠），会导致失败。

- attach附加到现有进程
    - 可能会错过hook时机

spawn参数
![](./img/3.png)

##### Session
Session可以理解为一次会话，比如附加到某个进程就算是启动了一次会话。

Session对象方法
 
 - on/off：添加/删除事件监听回调
 
 - detached事件：会话断开（进程终止等）
 - create_script：从js代码创建Script对象
 - compile_script/create_script_from_bytes：将js编译为字节码，然后创建Script
 - enable_debugger/disable_debugger：启用/禁用外部调试器
 - enable_jit：切换到支持JIT的v8脚本引擎（不支持ios）
 - enable_child_gating/disable_spawn_gating：启用/禁用子进程收集。

 
 
### js api
 
 看雪有中文翻译感觉还不错，在这里贴一下：
 一：https://zhuanlan.kanxue.com/article-342.htm
 二：https://zhuanlan.kanxue.com/article-414.htm
 三：https://zhuanlan.kanxue.com/article-4277.htm
 
### 示例学习

##### 事件和异常处理

- 脚本使⽤用 send 和 recv 与 python 绑定进⾏双向通信

- recv 返回的对象提供 wait ⽅方法，可阻塞等待 python 端返回
- 在 js 的回调中产⽣生的异常，会⽣生成⼀一个 type: “error” 的消息
- 除了了消息之外，python 还可调⽤用 js 导出的 rpc.exports 对象中的⽅方法。通 过返回 Promise 对象来⽀支持异步任务(详⻅见后续章节*)
- rpc 接⼝口产⽣生的异常会直接抛出到 python，⽽而不不是交给 on(‘message’) 的 回调函数

```
# -*- coding: utf-8 -*-
from __future__ import print_function

import frida
import os


def user_input():
    prompt = 'please input something: '
    try:
        return raw_input(prompt)
    except:
        return input(prompt)


# attach to python interpreter process itself :)
session = frida.attach(os.getpid())
script = session.create_script("""\
'use strict';
rpc.exports = {
  hello: function () {
    return 'Hello';
  },
  failPlease: function () {
    // this exception may crash python
    throw new Error('failed to call rpc method');
  },
  wait: function() {
    return new Promise(function(resolve, reject) {
      setTimeout(function() {
        resolve('wait for me')
      }, 200)
    })
  }
};
// send a log to client
console.warn("alert");
// send JSON message and binary payload to client
send({
  topic: "greet",
  format: "json"
}, new Buffer("hello, world"));
setTimeout(function() {
  // this exception will only emit an event
  throw new Error('other exception');
}, 100);
setImmediate(function() {
  recv('input', function(msg) {
    console.log('>>> you have just input', msg.payload);
  }).wait();
});
""")

def on_message(msg, payload):
    print('msg', msg, payload)

    if msg.get('payload') and msg.get('payload').get('topic') == 'greet':
      script.post({ 'type': 'input', 'payload': user_input() })

def on_console_log(level, text):
    print('console.' + level + ':', text)

script.on('message', on_message)
script.set_log_handler(on_console_log)
script.load()
api = script.exports
print("api.hello() =>", api.hello())

try:
    api.fail_please()
except frida.core.RPCException as e:
    print('rpc error', e)

print('api.wait() =>', api.wait())
```
刚入门看不太明白，在下面补充一点基础知识：

- JavaScript 异步编程（这里简单提一下，下面会细说）
    - 以上述脚本中的代码为例：
        
        ```
        setTimeout(function() {
            throw new Error('other exception');
        }, 100);
        ```
    - setTimeout函数，他的第一个参数是一个回调函数，第二个参数是毫秒数，这个函数执行之后会产生一个子线程，子线程会等待0.1秒（参数2），然后执行回调函数。
    - setImmediate 该方法用来把一些需要长时间运行的操作放在一个回调函数里,在完成后面的其他语句后,就立刻执行这个回调函数。
    - Promise对象：https://www.runoob.com/js/js-promise.html

- 主控端和目标进程的交互（message）
    - Js脚本提供了向主控端发送数据的接口—send和从主控端接收数据的接口—recv，而在主控端是通过python脚本的回调来接收数据，并使用python提供的接口post来向目标进程发送数据。这里的数据需要是可以序列化成json字符串的。

- 用到的一些js api
    -  console.warn(line)：向标准输入输出界面写入 line 字符串。
    -  rpc.exports: 可以在你的程序中导出一些 RPC-Style API函数，Key指定导出的名称，Value指定导出的函数，函数可以直接返回一个值，也可以是异步方式以 Promise 的方式返回，也就是上面代码所展示的那样：
        ```
        rpc.exports = {
            hello: function () {
                return 'Hello';
            },
            failPlease: function () {
                // this exception may crash python
                throw new Error('failed to call rpc method');
            },
            wait: function() {
                return new Promise(function(resolve, reject) {
                    setTimeout(function() {
                        resolve('wait for me')
                    }, 200)
                })
            }
        };
        ```
   
    - 对于Python主控端可以使用下面这样的脚本使用导出的函数：
        
        ```
        api = script.exports
        print("api.hello() =>", api.hello())

        try:
            api.fail_please()
        except frida.core.RPCException as e:
            print('rpc error', e)

        print('api.wait() =>', api.wait())

        ```
        
### js 进阶

#### frida的javascript引擎

- 由于 iOS 的 JIT 限制，以及嵌⼊入式设备的内存压⼒力力，新版将默认脚在 Android 等⽀支持 v8 的平台上仍然可以使⽤用 enable-jit 选项切换回 v8Duktape ⽐比 v8 缺失了了⾮非常多 ECMAScript 6 特性，如箭头表达式、 let 关键字 http://wiki.duktape.org/PostEs5Features.html ，我们下面去看一下这些变化。

###### 箭头函数

- ECMAScript 6 引⼊入的书写匿匿名函数的特性需要启⽤用 JIT，或 frida-compile 转译才可在 frida 中使⽤比 function 表达式简洁。适合编写逻辑较短的回调函数语义并⾮非完全等价。箭头函数中的 this 指向⽗父作⽤用域中的上下⽂文;⽽而 function可以通过 Function.prototype.bind的方法指定上下⽂，以下两⾏行行代码等价 ：

    ```
    Process.enumerateModulesSync().filter(function(module){return module.path.startsWith('/Applications')})
    
    Process.enumerateMoudlesSync().filter(moudle => moudle.path.startWith('/Applications'))
    ```
    
###### generator 函数
![](./img/4.png)
![](./img/5.png)


###### async / await

- 调⽤用⼀一个 async 函数会返回⼀一个 Promise 对象。当这个 async 函数 返回⼀一个值时，Promise 的 resolve ⽅法会负责传递这个值;
- 当 async 函数抛出异常时，Promise 的 reject ⽅方法也会传递这个异常值 
```
async function name([param[, param[, ... param]]]) { statements }
```

- async 函数中可能会有 await 表达式，这会使 async 函数暂停执 ⾏行行，等待表达式中的 Promise 解析完成后继续执⾏行行 async 函数并返 回解决结果
- await 关键字仅仅在 async function中有效。如果在 async function 函数体外使⽤用 await ，会得到⼀一个语法错误(SyntaxError)

    ![](./img/6.png)
    

###### Promise
- Promise 对象是⼀一个代理理对象(代理理⼀一个值)，被代理理的值在 Promise 对象创建时可 能是未知的。它允许你为异步操作的成功和失败分别绑定相应的处理理⽅方法 (handlers)。 这让异步⽅方法可以像同步⽅方法那样返回值，但并不不是⽴立即返回最终执 ⾏行行结果，⽽而是⼀一个能代表未来出现的结果的 promise 对象

- 一个 Promise有以下几种状态:
    - pending: 初始状态，既不不是成功，也不不是失败状态。 
    
    - fulfilled: 意味着操作成功完成。
    - rejected: 意味着操作失败。

- 因为 Promise.prototype.then 和 Promise.prototype.catch 方法返回promise 对象， 所以它们可以被链式调⽤用。    
    
    ![](./img/7.png)
    
- 使用 Promise
    - Duktape 原⽣生⽀支持 Promise 规范，但 async/await 或 yield 需要使用 frida-compile 转译回 ES5
    
    - rpc.exports 接⼝支持 Promise，可实现等待回调函数返回。示例代码:可以参考上面的那个例子。
    - frida 内置与 I/O 相关的接口 Socket, SocketListener, IOStream 及其子类均使⽤ Promise 的接口。结合 Stream 可实现⼤文件传输等异步任务

    
## frida hook fuzz尝试

##### 我们先从简单的小目标练起

上一段c代码：
```
#include <stdio>
#include <stdlib.h>
#include <string.h>

void pwn(char *b)
{
    char buffer[1337];
    strcpy(buffer,b);
}

int main(int argc, char **argv)
{
    pwn(argv[1]);
}
```

这个漏洞很明显，在pwn函数中存在溢出，我们下面来通过frida的方式来进行一个模糊测试：

```
#!/usr/bin/python3

import frida
import time
import sys

def on_message(message, data):
    print(message)

js = """

// Maximum payload size
var size = 2000;

// Argument for the fuzzed function
var arg = Memory.alloc(size);
var fuzzData = [0x41];

var pwnAddr = null;
var pwnHandle = null;

// Find the vulnerable function in the target process
// and get a handle to it
Module.enumerateSymbolsSync("test").forEach(function(symbol){
        switch (symbol.name) {
            case "pwn":
                pwnAddr = symbol.address;
                // use the function prototype to create a handle
                lolHandle = new NativeFunction(ptr(pwnAddr), "void", ["pointer"]);
                console.log("[i] lol() is at " + pwnAddr);
        }
    });

if (pwnAddr == null) {
    die("Error finding symbol");
}

// Fuzz the function in-process
Interceptor.attach(ptr(pwnAddr), {
    // Begin fuzzing as soon as the application calls the function itself
    onEnter: function(args) {
        console.log("[i] Original argument: " + args[0].readCString());

        console.log("[*] Fuzzing now");
        while(fuzzData.length < size) {
            fuzzData.push(0x41);
            Memory.writeByteArray(arg, fuzzData);
            try {
                pwnHandle(arg);
            }
            catch(e) {
                console.log("[!] Crash found for size " + fuzzData.length);
                break;
            }
        }
    },
});
"""

# Spawn the target process
pid = frida.spawn(["./test", "hello"])
session = frida.attach(pid)

# Inject dem scriptz
script = session.create_script(js)
script.on('message', on_message)
script.load()

# Continue execution of the target
frida.resume(pid)

sys.stdin.read()
```


- 大致思路：我们通过spawn的方式使Frida生成进程，将我们写好的js代码注入其中，在恢复执行等待它调用pwn()函数，js为他定义了一个hook，我们的hook主要作用是不断增加输入来进行fuzz，直到目标崩溃。

补充一点关键知识：

###### Interceptor
- Interceptor.attach(target, callbacks): 在target指定的位置进行函数调用拦截，target是一个NativePointer参数，用来指定你想要拦截的函数的地址，有一点需要注意，在32位ARM机型上，ARM函数地址末位一定是0（2字节对齐），Thumb函数地址末位一定1（单字节对齐），如果使用的函数地址是用Frida API获取的话， 那么API内部会自动处理这个细节（比如：Module.findExportByName()）。其中callbacks参数是一个对象，大致结构如下： 
    - onEnter: function(args): 被拦截函数调用之前回调，其中原始函数的参数使用args数组（NativePointer对象数组）来表示，可以在这里修改函数的调用参数。
    
    - onLeave: function(retval): 被拦截函数调用之后回调，其中retval表示原始函数的返回值，retval是从NativePointer继承来的，是对原始返回值的一个封装，你可以使用retval.replace(1337)调用来修改返回值的内容。需要注意的一点是，retval对象只在 onLeave函数作用域范围内有效，因此如果你要保存这个对象以备后续使用的话，一定要使用深拷贝来保存对象，比如：ptr(retval.toString())。
    
    
- 补充：在这里我们使用了下面的方式来获取函数地址

    ```
    Module.enumerateSymbolsSync("test").forEach(function(symbol){
        switch (symbol.name) {
            case "pwn":
                pwnAddr = symbol.address;
                // use the function prototype to create a handle
                lolHandle = new NativeFunction(ptr(pwnAddr), "void", ["pointer"]);
                console.log("[i] lol() is at " + pwnAddr);
        }
    });

    ```
    - 因为这是我们的练习所以可以这样获取。

- 在实际的hook中，我们可以通过Process.enumerateModulesSync()和Module.enumerateSymbols("<module>")获取目标函数的地址，也可以Module.findExportByName(module | null, exp)或者是获取偏移地址Module.findBaseAddress(module)+偏移。（https://blog.csdn.net/friendan/article/details/105048853）

