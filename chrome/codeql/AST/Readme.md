因为自己之前是没去写编译器的，感觉codeql这部分和ast关系还蛮大的，于是在此记录一下自己对ast的一个学习：


### AST

抽象语法树 (Abstract Syntax Tree)，简称 AST，它是源代码语法结构的一种抽象表示。它以树状的形式表现编程语言的语法结构，树上的每个节点都表示源代码中的一种结构。

### AST的生成

举个例子：js代码要生成ast一般需要经过下面两个步骤：

- 分词：将整个代码字符串分割成最小语法单元数组。
- 语法分析：在分词基础上建立分析语法单元之间的关系。

##### 词法分析

词法分析，也称之为扫描（scanner），简单来说就是调用 next() 方法，一个一个字母的来读取字符，然后与定义好的 JavaScript 关键字符做比较，生成对应的Token。Token 是一个不可分割的最小单元:

例如 var 这三个字符，它只能作为一个整体，语义上不能再被分解，因此它是一个 Token。

词法分析器里，每个关键字是一个 Token ，每个标识符是一个 Token，每个操作符是一个 Token，每个标点符号也都是一个 Token。除此之外，还会过滤掉源程序中的注释和空白字符（换行符、空格、制表符等。

最终，整个代码将被分割进一个tokens列表（或者说一维数组）。

##### 语法分析

语法分析会将词法分析出来的 Token 转化成有语法含义的抽象语法树结构。同时，验证语法，语法如果有错的话，抛出语法错误。


### 实例分析

这里我用了这个在线网站来完成js到ast的转化：
https://astexplorer.net/

###### 例一
首先从最简单的代码开始学起：

``` js
const fn = a => a
```

AST:
```
{
  "type": "Program",
  "start": 0,
  "end": 18,
  "body": [
    {
      "type": "VariableDeclaration",
      "start": 0,
      "end": 17,
      "declarations": [
        {
          "type": "VariableDeclarator",
          "start": 6,
          "end": 17,
          "id": {
            "type": "Identifier",
            "start": 6,
            "end": 8,
            "name": "fn"
          },
          "init": {
            "type": "ArrowFunctionExpression",
            "start": 11,
            "end": 17,
            "id": null,
            "expression": true,
            "generator": false,
            "async": false,
            "params": [
              {
                "type": "Identifier",
                "start": 11,
                "end": 12,
                "name": "a"
              }
            ],
            "body": {
              "type": "Identifier",
              "start": 16,
              "end": 17,
              "name": "a"
            }
          }
        }
      ],
      "kind": "const"
    }
  ],
  "sourceType": "module"
}
```

我们先来一个词法分析，该代码中的token一共有：

```
const
fn
=
a
=>
a
```
这么几个，那我们具体结合ast来做一个分析：

const 代表了ast中的
```
[
    {
      "type": "VariableDeclaration",
      
        ............
      
      ],
      "kind": "const"
    }
  ],
```
VariableDeclaration表示它是一个声明变量操作，并且它的kind是const

= 代表了ast中的：

```
"declarations": [
{
    "type": "VariableDeclarator",
          
    .........
            
}
],
```
"type": "VariableDeclarator",表示它是一个变量声明符
表达式的内容都是declarations的子树。

fn对应的是：
```
"id": {
            "type": "Identifier",
            "start": 6,
            "end": 8,
            "name": "fn"
          },
```

他表示是一个标识符，名字是fn

=> 对应
```
"init": {
            "type": "ArrowFunctionExpression",
            "start": 11,
            "end": 17,
            "id": null,
            "expression": true,
            "generator": false,
            "async": false,
            "params": [
              {
                "type": "Identifier",
                "start": 11,
                "end": 12,
                "name": "a"
              }
            ],
            "body": {
              "type": "Identifier",
              "start": 16,
              "end": 17,
              "name": "a"
            }
          }
```
ArrowFunctionExpression表示这是一个箭头函数表达式

其中的params和body分别代表了两个a，也就是参数是a，函数体也是a。


###### 例二

``` js
const fn = a => {
    let i = 1;
  return a + i;
};
```

AST:
```
{
  "type": "Program",
  "start": 0,
  "end": 51,
  "body": [
    {
      "type": "VariableDeclaration",
      "start": 0,
      "end": 51,
      "declarations": [
        {
          "type": "VariableDeclarator",
          "start": 6,
          "end": 50,
          "id": {
            "type": "Identifier",
            "start": 6,
            "end": 8,
            "name": "fn"
          },
          "init": {
            "type": "ArrowFunctionExpression",
            "start": 11,
            "end": 50,
            "id": null,
            "expression": false,
            "generator": false,
            "async": false,
            "params": [
              {
                "type": "Identifier",
                "start": 11,
                "end": 12,
                "name": "a"
              }
            ],
            "body": {
              "type": "BlockStatement",
              "start": 16,
              "end": 50,
              "body": [
                {
                  "type": "VariableDeclaration",
                  "start": 22,
                  "end": 32,
                  "declarations": [
                    {
                      "type": "VariableDeclarator",
                      "start": 26,
                      "end": 31,
                      "id": {
                        "type": "Identifier",
                        "start": 26,
                        "end": 27,
                        "name": "i"
                      },
                      "init": {
                        "type": "Literal",
                        "start": 30,
                        "end": 31,
                        "value": 1,
                        "raw": "1"
                      }
                    }
                  ],
                  "kind": "let"
                },
                {
                  "type": "ReturnStatement",
                  "start": 35,
                  "end": 48,
                  "argument": {
                    "type": "BinaryExpression",
                    "start": 42,
                    "end": 47,
                    "left": {
                      "type": "Identifier",
                      "start": 42,
                      "end": 43,
                      "name": "a"
                    },
                    "operator": "+",
                    "right": {
                      "type": "Identifier",
                      "start": 46,
                      "end": 47,
                      "name": "i"
                    }
                  }
                }
              ]
            }
          }
        }
      ],
      "kind": "const"
    }
  ],
  "sourceType": "module"
}
```

我们直接去看ArrowFunctionExpression的body部分：
它有两个子树分别对应了函数体中两行代码的解析：

第一行：
```
    {
        "type": "BlockStatement",
        "start": 16,
        "end": 50,
        "body": [
        {
            "type": "VariableDeclaration",
            "start": 22,
            "end": 32,
            "declarations": [
            {
                "type": "VariableDeclarator",
                "start": 26,
                "end": 31,
                "id": {
                "type": "Identifier",
                "start": 26,
                "end": 27,
                "name": "i"
                },
                "init": {
                "type": "Literal",
                "start": 30,
                "end": 31,
                "value": 1,
                "raw": "1"
                }
            }
            ],
            "kind": "let"
        },
```
它是一个块语句，表示给标识符i赋值1

第二行：

```
    {
            "type": "ReturnStatement",
            "start": 35,
            "end": 48,
            "argument": {
            "type": "BinaryExpression",
            "start": 42,
            "end": 47,
            "left": {
                "type": "Identifier",
                "start": 42,
                "end": 43,
                "name": "a"
            },
            "operator": "+",
            "right": {
                "type": "Identifier",
                "start": 46,
                "end": 47,
                "name": "i"
            }
            }
        }
```
第二行表示它是一个返回语句，他的内容是一个二元运算，左值为a，右值为i，操作符为 +

###### 例三

``` js
function test()
{
  a = 1;
  console.log(a)
}
```
我们通过它来熟悉一下函数调用

AST：
```
{
  "type": "Program",
  "start": 0,
  "end": 45,
  "body": [
    {
      "type": "FunctionDeclaration",
      "start": 0,
      "end": 45,
      "id": {
        "type": "Identifier",
        "start": 9,
        "end": 13,
        "name": "test"
      },
      "expression": false,
      "generator": false,
      "async": false,
      "params": [],
      "body": {
        "type": "BlockStatement",
        "start": 16,
        "end": 45,
        "body": [
          {
            "type": "ExpressionStatement",
            "start": 20,
            "end": 26,
            "expression": {
              "type": "AssignmentExpression",
              "start": 20,
              "end": 25,
              "operator": "=",
              "left": {
                "type": "Identifier",
                "start": 20,
                "end": 21,
                "name": "a"
              },
              "right": {
                "type": "Literal",
                "start": 24,
                "end": 25,
                "value": 1,
                "raw": "1"
              }
            }
          },
          {
            "type": "ExpressionStatement",
            "start": 29,
            "end": 43,
            "expression": {
              "type": "CallExpression",
              "start": 29,
              "end": 43,
              "callee": {
                "type": "MemberExpression",
                "start": 29,
                "end": 40,
                "object": {
                  "type": "Identifier",
                  "start": 29,
                  "end": 36,
                  "name": "console"
                },
                "property": {
                  "type": "Identifier",
                  "start": 37,
                  "end": 40,
                  "name": "log"
                },
                "computed": false,
                "optional": false
              },
              "arguments": [
                {
                  "type": "Identifier",
                  "start": 41,
                  "end": 42,
                  "name": "a"
                }
              ],
              "optional": false
            }
          }
        ]
      }
    }
  ],
  "sourceType": "module"
}
```

我们主要看console.log(a)

```
          {
            "type": "ExpressionStatement",
            "start": 29,
            "end": 43,
            "expression": {
              "type": "CallExpression",
              "start": 29,
              "end": 43,
              "callee": {
                "type": "MemberExpression",
                "start": 29,
                "end": 40,
                "object": {
                  "type": "Identifier",
                  "start": 29,
                  "end": 36,
                  "name": "console"
                },
                "property": {
                  "type": "Identifier",
                  "start": 37,
                  "end": 40,
                  "name": "log"
                },
                "computed": false,
                "optional": false
              },
              "arguments": [
                {
                  "type": "Identifier",
                  "start": 41,
                  "end": 42,
                  "name": "a"
                }
              ],
              "optional": false
            }
          }
```

它是一个表达式语句，表达式的类型为CallExpression（函数调用表达式）

MemberExpression成员表达式节点,即表示引用对象成员的语句,
object是引用对象的表达式节点, property是表示属性名称,
1. computed如果为 false,是表示.(点)来引用成员 即 property应该为一个 Identifier节点,
2. computed如果为true,则是来进行引用, 即 property是一个 Expression节点,名称是表达式的结果值。

