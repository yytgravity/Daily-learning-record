- https://bugs.chromium.org/p/chromium/issues/detail?id=1135857
```
scoped_refptr<UsbDeviceHandle> device_handle
可以通过多次执行self->device_handle_ = std::move(handle);使旧scoped_refptr指针被析构。
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1133671
```
RegisterKeyPressHandler(
          base::Bind(&AutofillPopupControllerImpl::HandleKeyPressEvent,
                     base::Unretained(this)));
注册handler时，传入了一个未保存的原始指针。
```


- https://bugs.chromium.org/p/chromium/issues/detail?id=1101509
```
1、RawClipboardHostImpl中保存了render_frame_host
2、但他的生命周期独立于render_frame_host，这就导致了render_frame_host被释放后在RawClipboardHostImpl中仍可使用失效的指针。
（ps 一般通过Impl继承自WebObserver等来观察renderframehost的生命周期，当renderframehost析构的时候会通知Impl做出正确的处理）
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1122917
```
1、DirectSocketsServiceImpl中，通过MakeSelfOwnedReceiver函数来创建了一个self-owned的receiver
2、DirectSocketsServiceImpl中保存了一个render_frame_host_对象
3、但是没有通过任何方法来将InstalledAppProviderImpl和RenderFrameHost的生命周期绑定，这就导致了UAF的产生。
（ps 一般通过Impl继承自WebObserver等来观察renderframehost的生命周期，当renderframehost析构的时候会通知Impl做出正确的处理）
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1116304
```
1、consumer_feedback_observer_中保存了launched_device_的raw pointer
2、stopping the device 会导致launched_device_被reset
3、但是launched_device_被reset后，observer中保存的raw pointer并没有被影响，这就导致了UAF的产生
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1119865
```
1、ProfilerGroup::StopProfiler函数中resolve可以重入js调用用户自定义的函数
2、StopProfiler会调用DeleteProfile释放掉profile
3、通过resolve连续调用StopProfiler，导致profile发生uaf
```

- https://bugs.chromium.org/p/project-zero/issues/detail?id=1755  
```
一个新模式：通过持有remote端回到js执行代码
关键有两点：
1、FileWrite的生命周期并没有被FileSystemManager所管理，我们是可以断开message pipe的方法来析构掉FileWrite的。
2、base::Unretained(this)，被Unretained修饰的this指针，只由回调的调用者来保证回调执行时，this指针仍然可用。这里如果换成WeakPtr，那么在this被析构后，回调就不会被执行。
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1108497
```
迭代器失效漏洞
1、在WatchAvailabilityInternal函数中可以添加回调函数availability_callbacks_.insert(id, callback)；
2、callback->Run：回调函数可以套娃调用WatchAvailabilityInternal，这样insert(id, callback)会导致availability_callbacks_重分配，迭代器失效。
```
- https://bugs.chromium.org/p/chromium/issues/detail?id=925864
```
1、id是一个int，并且没有对溢出的检测，存在一个整数溢出。
2、通过溢出可以得到：
operations_.emplace(1, std::move(operation1));
operations_.emplace(overflow(1), std::move(operation2));
3、在overflow(1)的第二次emplace时，因为operations_是一个unique_key的容器，它不允许key相同，所以第二次装入是失败的，operation2这个unique_ptr被析构，且此时它里面保存的原始指针不为空，从而使得指针指向的FileSystemOperation也被析构，operation_raw变成悬空指针。
4、operation_raw->Truncate就会触发uaf
```

- 一个好玩的模式
```
func1(){
   ptr* p;
   unique_ptr<A> a = new A(p);
   func2(std::move(a))
   use p;//p->xxx
}
func2(unique_ptr<A> a1){
  if(xx){
    return;
  }
  xx = std::move(a1);
}
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1133635
```
1、调用driver_的虚函数时，没有空指针的check。
2、ContentPasswordManagerDriver的生命周期被绑定到了RenderFrameHost；
3、remove iframe后执行driver_->GeneratedPasswordAccepted的话就会触发uaf
```

- https://bugs.chromium.org/p/chromium/issues/detail?id=1125614
```
1、InternalAuthenticator创建时生命周期同render_frame_host绑定。
2、InternalAuthenticator在SecurePaymentConfirmationAppFactory::Create函数中所有权传递给了SecurePaymentConfirmationAppFactory::OnIsUserVerifyingPlatformAuthenticatorAvailable这个回调函数
3、回调函数被加入到了回调队列，并且该队列不会被render_frame_host影响
4、destory frame，render_frame_host被释放，InternalAuthenticator被释放，但是在回调队列中他依旧存在，之后回调触发导致uaf。
```


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

