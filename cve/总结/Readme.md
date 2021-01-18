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

