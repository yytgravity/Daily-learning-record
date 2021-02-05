https://bugs.chromium.org/p/chromium/issues/detail?id=1051748

We can trigger this edge case through the following steps:

1. Create an iframe and append it to our document to get a new ExecutionContext
2. Create a AudioWorklet within our new iframe's ExecutionContext
3. Remove the iframe's ExecutionContext, which calls ContextLifecycleObserver::ContextDestroyed
4. Force a WebGL console error message, which will attempt to use the context that was uninitialized

```
void WebGLRenderingContextBase::PrintWarningToConsole(const String& message) {
  blink::ExecutionContext* context = Host()->GetTopExecutionContext();
  if (context) { // <------------------ UAP, does not check if the context has been destroyed
    context->AddConsoleMessage(
        ConsoleMessage::Create(mojom::ConsoleMessageSource::kRendering,
                               mojom::ConsoleMessageLevel::kWarning, message));
  }
}
```

