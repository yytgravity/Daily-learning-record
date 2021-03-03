这个补丁让类型识别的情况下，在这样的语句' for (var i = start;我<……;i += increment){…循环变量可以变成' NaN '，因为' start '和' increment '是符号不同的' Infinity '值。

不幸的是，引入的检查不足以捕获所有可能产生“NaN”的循环。这段代码假设当递增变量既可以是正数也可以是负数时，结果类型将是' kInteger '(不包括' NaN ')。然而，由于' increment '的值可以在循环体内部更改，所以可以一直从' i '中减去，直到它达到' -∞'，然后将' increment '设置为' +∞'。这将使“i”在循环的下一次迭代中变成“NaN”。

捷径:如果变量的增量可以是正的，也可以是负的，那么变量的增量可以任意增大，所以只返回整数。


```js
  var value = Math.max(i, 1024);  //console.log("value0 "+value);  value = -value;  //console.log("value1 "+value);  value = Math.max(value, -1025);  //console.log("value2 "+value);  value = -value;  //console.log("value3 "+value);
```
![](./img/2.png)

value0 NaNvalue1 NaNvalue2 -2147483648value3 -2147483648
  
``` js 
  value -= 1022;
  value >>= 1; 
  value += 10; 
```

实际长度：1073741323
v8以为的长度：11