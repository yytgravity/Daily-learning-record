### gdb
起服务：
```
python -m SimpleHTTPServer 8000
```

gdb_test.sh:
```
file ./chromeset args --headless --disable-gpu --user-data-dir=./userdata --remote-debugging-port=1338 --enable-blink-features=MojoJS http://127.0.0.1:8000/exp.htmlset follow-fork-mode parent
```

gdb -x gdb_test.sh

### 下断点

gdb断点：
有符号的时候可以直接在函数下断点：b content::xxx::xxx

exp断点：
```
function debug(){    for(let i = 0; i < 0x100000; i++){        for(let j = 0; j < 0x100000; j++){            var x = x + i + j;        }    }}
```

### chrome中调d8

sh中增加--js-flags="--allow-natives-syntax"

可以在exp中添加%DebugPrint();来提供调试信息，下断点的方式就是用上面提到的debug()函数，在循环的时候ctrl+c，在 ps ax，新开一个gdb attach到 d8进程。

![](./img/1.png)