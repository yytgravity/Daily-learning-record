最近遇到很多重入js的漏洞，记录一下ql

重入js类
-------
- https://bugs.chromium.org/p/chromium/issues/detail?id=1106682
```
Resolve重入的漏洞，老面孔了。
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1051748
```
1、通过重入js Remove the iframe's ExecutionContext。
2、之后调用context时触发uaf
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1116706
```
在resolver_->Resolve(connection_);重入js，通过remove iframe使得被释放
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1116706
```
在resolver_->Resolve(connection_);处可以通过重入js的方式通过remove iframe来使得controller被析构，同样的ExecutionContext也将被析构
```

## ql

```
import cpp

class Iterator extends Variable {
    Iterator() {
        this.getUnderlyingType().getName().matches("%iterator%")
        // getType is inconsistent
        or this.getAnAssignedValue().(FunctionCall).getTarget().(MemberFunction).getName().regexpMatch("c?r?begin")
        or this.getAnAssignedValue().(FunctionCall).getTarget().(MemberFunction).getName().regexpMatch("c?r?end")
    }
    
    VariableAccess iterated(){
        // result = this.getAnAssignedValue().getChild(-1).(VariableAccess).getTarget().getAnAssignedValue()
        this.getAnAssignedValue().getChild(-1) = result and not result.getTarget().isCompilerGenerated()
        // show the iterable assigned to __range in ranged based for loops
        or (this.getAnAssignedValue().getChild(-1).(VariableAccess).getTarget().isCompilerGenerated()
        and result = this.getAnAssignedValue().getChild(-1).(VariableAccess).getTarget().getAnAssignedValue())
    }
}


class DispatchEventCall extends FunctionCall{
    predicate containedBy(Stmt other) {
        other.getASuccessor*() = this
        and other.getAChild*() = this
    }

    DispatchEventCall(){
        // 直接调用DispatchEvent
        (this.getTarget().(MemberFunction).getName().matches("DispatchEvent")
        and this.getTarget().(MemberFunction).getEnclosingElement().(Class).getName().matches("EventTarget"))
        // 间接调用DispatchEvent
        or exists (DispatchEventCall dc | 
            dc.containedBy(this.getTarget().getBlock()) 
            or 
            // 用于被重载的虚函数调用
            dc.containedBy(this.getTarget().(MemberFunction).getAnOverridingFunction().getBlock())
            )
    }
}

class ResolveCall extends FunctionCall{
    predicate containedBy(Stmt other) {
        other.getASuccessor*() = this
        and other.getAChild*() = this
    }

    ResolveCall(){
        // 直接调用Resolve
        (this.getTarget().(MemberFunction).getName().matches("Resolve")
        and this.getTarget().(MemberFunction).getEnclosingElement().(Class).getName().matches("ScriptPromiseResolver")
        and this.getTarget().(MemberFunction).getNumberOfParameters() = 1
        )
        // 间接调用Resolve
        or exists (ResolveCall dc | 
            (
            dc.containedBy(this.getTarget().getBlock()) 
            or 
            // 用于被重载的虚函数调用
            dc.containedBy(this.getTarget().(MemberFunction).getAnOverridingFunction().getBlock())
            )
            and not (
                this.getTarget().(MemberFunction).getName().matches("Resolve")
                and 
                this.getTarget().(MemberFunction).getEnclosingElement().(Class).getName().matches("ScriptPromiseResolver")
                and
                this.getTarget().(MemberFunction).getNumberOfParameters() = 0
            )
        )
    }
    ResolveCall child() {
        // result = this.getTarget().getBlock().getAChild*().(ResolveCall)
        // or
        // result = this.getTarget().(MemberFunction).getAnOverriddenFunction().getBlock().getAChild*().(ResolveCall)
        (result.containedBy(this.getTarget().getBlock()) 
            or 
            // 用于被重载的虚函数调用
        result.containedBy(this.getTarget().(MemberFunction).getAnOverridingFunction().getBlock()))
        and not (
                result.getTarget().(MemberFunction).getName().matches("Resolve")
                and 
                result.getTarget().(MemberFunction).getEnclosingElement().(Class).getName().matches("ScriptPromiseResolver")
                and
                result.getTarget().(MemberFunction).getNumberOfParameters() = 0
            )
    }
} 

// 因为操作符->经常在chrome里被重载，在这种情况下，此通用方法将跳过被重载的->，找到真正的限定符
Expr getQualifer(Expr fc){
    exists(Expr qualifer | 
    qualifer = fc.(FunctionCall).getQualifier() or 
    qualifer = fc.(FieldAccess).getQualifier() | 
    if qualifer.(FunctionCall).getTarget().hasName("operator->") then
        result = getQualifer(qualifer)
    else
        result = qualifer
    )
}

//宽松的约束条件，首先找到一组ResolveCall，再找到一组Iterator，如果ResolveCall和Iterator在同一scope里，即认为这是一个可能使迭代器实效的顶层调用
class TopResolveCall extends ResolveCall{
    Iterator iter;
    TopResolveCall(){
        this.containedBy(iter.getParentScope())
    }
    Iterator iterator(){
        result = iter
    }
}

//宽松的约束条件，首先找到一组DispatchEventCall，再找到一组Iterator，如果DispatchEventCall和Iterator在同一scope里，即认为这是一个可能使迭代器实效的顶层调用
class TopDispatchEventCall extends DispatchEventCall{
    Iterator iter;
    TopDispatchEventCall(){
        this.containedBy(iter.getParentScope())
    }
    Iterator iterator(){
        result = iter
    }
}

// https://en.cppreference.com/w/cpp/language/range-for
// 保守的DispatchEventCall顶层调用，基于逻辑如下,容易漏掉很多东西
// {
//     auto && __range = range_expression ;
//     for (auto __begin = begin_expr, __end = end_expr; __begin != __end; ++__begin) {
//          range_declaration = *__begin;
//          loop_statement
//     }
// }
// getQualifer(this)找到range_declaration的access，然后找到它的target，即range_declaration
// range_declaration的AssignedValue是call `opertor *`，取其getQualifer得到__begin的VariableAccess
// 再取其getTarget得到__begin，这就是我们之前通过Iterator得到的一组iter之一
class ConserveTopDispatchEventCall extends DispatchEventCall{
    Iterator iter;
    ConserveTopDispatchEventCall(){
        getQualifer(this).(VariableAccess).getTarget().getAnAssignedValue().(FunctionCall).getQualifier().(VariableAccess).getTarget() = iter
    }
    Iterator iterator(){
        result = iter
    }
}

// 测试TopDispatchEventCall
// from TopDispatchEventCall ec
// where ec.getLocation().toString().matches("%xr%")
// select ec, ec.iterator(), ec.iterator().iterated()

// from ConserveTopDispatchEventCall ec
// where ec.getLocation().toString().matches("%xr%")
// select ec, ec.iterator(), ec.iterator().iterated()

// 为了更好的进行分析，构建整条调用路径，示例如下:
predicate calls(Function caller, Function callee){
    callee.getACallToThisFunction().getEnclosingFunction() = caller
}

predicate virtuallCalls(Function a, Function b){
    a.calls(b) or
    a.calls(b.(MemberFunction).getAnOverriddenFunction+())
}

// class DispatchEvent extends Function{
//     DispatchEvent(){
//         this.getName().matches("DispatchEvent") and
//         this.getEnclosingElement().(Class).getName().matches("EventTarget")
//     }
// }
// class FocusedFrameChanged extends Function{
//     FocusedFrameChanged(){
//         this.getName().matches("FocusedFrameChanged") and
//         this.getEnclosingElement().(Class).getName().matches("XRSystem")
//     }
// }

// from  FocusedFrameChanged start, DispatchEvent end, Function f
// where virtuallCalls*(start, f) and virtuallCalls*(f, end)
// select f
```

```
import cpp
import user_callback

from TopResolveCall rc, ResolveCall arc
where not rc.getLocation().toString().matches("%bindings%")
// and not rc.iterator().iterated().getTarget() instanceof StackVariable
and not rc.child+().getLocation().toString().matches("%animation%")
and arc.child().(FunctionCall).getTarget().getName().matches("Resolve")
select rc
```

