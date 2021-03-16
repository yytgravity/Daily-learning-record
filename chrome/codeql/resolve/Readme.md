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


//getUnderlyingType：在解析typedef后获取此表达式的类型。
//getAnAssignedValue：获取在程序中某处分配给该变量的表达式。
//isCompilerGenerated：如果此函数由编译器生成，则成立。
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

//getASuccessor：获取此指令的直接后继。
//getAChild：获取此元素的直接子元素。
//getEnclosingElement：获取封闭此对象的最近的Element。
//getAnOverridingFunction：获取直接重写的函数。
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

