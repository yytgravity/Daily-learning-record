## js深入浅出 一

### 参考资料：

MDN：https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/this

fun debug：https://blog.fundebug.com/2019/06/18/arrow-function-this/

### what is this

- A function's this keyword behaves a little differently in JavaScript compared to other languages. It also has some differences between strict mode and non-strict mode.


##### Global context

- Whether in strict mode or not , if this in the global execution context(outside of any function) , it refers to the global object.

eg.
```
//In web browsers, the window object is also the global object
console.log(this === window); // true

a = 37;
console.log(window.a); // 37

this.b = "MDN";
console.log(window.b)  // "MDN"
console.log(b)         // "MDN"
```

##### function context

- not in strict mode

```
function f1() {
  return this;
}

// In a browser:
f1() === window; // true

// In Node:
f1() === globalThis; // true
```

because the value of this is not set by the call, this will default to the global object, which is window in a browser.

 
- not in strict mode

```
function f2() {
  'use strict'; // see strict mode
  return this;
}

f2() === undefined; // true
```

In strict mode, however, if the value of this is not set when entering an execution context, it remains as undefined


##### Class context



