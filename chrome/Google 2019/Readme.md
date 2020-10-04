# monochromatic

### 题目分析

```
+interface BeingCreatorInterface {
+  CreatePerson() => (blink.mojom.PersonInterface? person);
+  CreateDog() => (blink.mojom.DogInterface? dog);
+  CreateCat() => (blink.mojom.CatInterface? cat);
+};

+interface CatInterface {
+  GetName() => (string name);
+  SetName(string new_name) => ();
+  GetAge() => (uint64 age);
+  SetAge(uint64 new_age) => ();
+  GetWeight() => (uint64 weight);
+  SetWeight(uint64 new_weight) => ();
+  CookAndEat(blink.mojom.FoodInterface food) => ();
+};

+interface DogInterface {
+  GetName() => (string name);
+  SetName(string new_name) => ();
+  GetAge() => (uint64 age);
+  SetAge(uint64 new_age) => ();
+  GetWeight() => (uint64 weight);
+  SetWeight(uint64 new_weight) => ();
+  CookAndEat(blink.mojom.FoodInterface food) => ();
+};

+interface FoodInterface {
+  GetDescription() => (string description);
+  SetDescription(string new_description) => ();
+  GetWeight() => (uint64 weight);
+  SetWeight(uint64 new_weight) => ();
+};

+interface PersonInterface {
+  GetName() => (string name);
+  SetName(string new_name) => ();
+  GetAge() => (uint64 age);
+  SetAge(uint64 new_age) => ();
+  GetWeight() => (uint64 weight);
+  SetWeight(uint64 new_weight) => ();
+  CookAndEat(blink.mojom.FoodInterface food) => ();
+};
```
我们可以看到在补丁中添加了很多mojo接口的实现，除了food，所以我们需要在renderer进程中为他做一个实现。

```
function FoodInterfaceImpl() {
      this.binding = new mojo.Binding(blink.mojom.FoodInterface, this);
    }

    // Food interface implementation.
    FoodInterfaceImpl.prototype = {
        getDescription: async () => {
        },
        setDescription: async (arg) => {
        },
        getWeight: async () => {
        },
        setWeight: async (arg) => {
        },
    };

    // 3. Create Food object.
    let food_impl = new FoodInterfaceImpl();
    let food_impl_ptr = new blink.mojom.FoodInterfacePtr();
    food_impl.binding.bind(mojo.makeRequest(food_impl_ptr));
```

### 漏洞分析
首先先简述一下c++接口实现：

- c++的接口实现主要基于回调，接口实现的方法不会通过return语句返回值。相反，它的工作方式是方法期望一个回调作为一个参数(例如PersonInterfaceImpl::CookAndEat中的CookAndEatCallback callback参数)。调用这个回调通知该方法完成了它的执行，.run()的参数作为返回值（返回值数目大于等于1）。

上代码：
```
void PersonInterfaceImpl::AddWeight(
    PersonInterfaceImpl::CookAndEatCallback callback,
    blink::mojom::FoodInterfacePtr foodPtr, uint64_t weight_) {
  weight += weight_;
  std::move(callback).Run();
}

void PersonInterfaceImpl::CookAndEat(blink::mojom::FoodInterfacePtr foodPtr,
                                     CookAndEatCallback callback) {
  blink::mojom::FoodInterface *raw_food = foodPtr.get();

  raw_food->GetWeight(base::BindOnce(&PersonInterfaceImpl::AddWeight,
                                     base::Unretained(this),
                                     std::move(callback), std::move(foodPtr)));
}

//由于Food只有接口，具体的实现是由我们自己来控制的，这里先借用person的GetWeight来帮助理解
void PersonInterfaceImpl::GetWeight(GetWeightCallback callback) {
  std::move(callback).Run(weight);
}
```
- 我们从GetWeight看起，该方法的参数是一个回调，该方法执行完毕会将weight作为返回值传递给回调函数。

- 接下来看CookAndEat，在调用raw_food的GetWeight方法时，传递了一个参数，它是一个回调函数，这里通过base::BindOnce将AddWeight和执行所需要的参数绑定在一起（除了weight），GetWeight执行结束后会将weight传递给AddWeight并执行AddWeight。
- 最后在AddWeight中将获得的weight_参数加到当前的weight上。

漏洞点：base::Unretained(this)，被Unretained修饰的this指针，只由回调的调用者来保证回调执行时，this指针仍然可用。
- （以person为例）说人话就是：当我们的在food的GetWeight方法中free掉person，但是调用者（food）依旧可以保证this可用，这样的话AddWeight就可以修改被释放的内存。

### 漏洞利用

在漏洞利用之前，我们首先分析一下cat、dog、person的结构：

```
pwndbg> x/20gx 0xdb099c30f400xdb099c30f40:	0x000055555e515ae0	0x00000db099c5cb800xdb099c30f50:	0x0000000000000030	0x80000000000000400xdb099c30f60:	0x0000000000000000	0x0000000000000000

pwndbg> x/20gx 0x00000db099c5cb800xdb099c5cb80:	0x4141414141414100	0x41414141414141410xdb099c5cb90:	0x4141414141414141	0x41414141414141410xdb099c5cba0:	0x4141414141414141	0x41414141414141410xdb099c5cbb0:	0x0000000000000000	0x0000000000000000
```

