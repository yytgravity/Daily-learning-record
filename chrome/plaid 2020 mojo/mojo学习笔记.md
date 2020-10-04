## Mojo

##### 概述

一个消息管道（pipe）是一对端点（endpoints）。每个端点都有一个传入消息的队列，在一个端点上写一条消息可以有效地将该消息排队到另一个（对等）端点上。因此，消息管道是双向的。

mojom 文件描述了接口，接口是消息的强类型集合。

给定mojom接口和消息管道，可以将其中一个端点指定为 Remote，并用于发送接口描述的消息。另一个端点可以指定为 Receiver，用于接收接口消息。

- 注：消息管道仍然是双向的，mojom 消息可能需要回复。回复从 Receiver 端发送，并由 Remote 端点接收。

为了处理接收到的消息，Receiver 端点必须与其 mojom 接口的实现相关联（即 bound）。

##### 定义新的Frame Interface

假设我们想从render frame（呈现帧）向其对应在browser进程里的RenderFrameHostImpl实例发送一个“Ping”消息，我们需要去定义一个mojom interface，创建一个pipe去使用这个interface，然后绑定好pipe的两端以发送和接收消息。

###### 定义接口

首先创建一个定义接口的文件，后缀是 .mojom

```
// src/example/public/mojom/ping_responder.mojom
module example.mojom;

interface PingResponder {
  // Receives a "Ping" and responds with a random integer.
  Ping() => (int32 random);
};
```

然后，需要在规则文件中定义这个文件用于生成c++代码

```
# src/example/public/mojom/BUILD.gn
import("//mojo/public/tools/bindings/mojom.gni")
mojom("mojom") {
  sources = [ "ping_responder.mojom" ]
}
```

###### 创建pipe

- 作为一般规则和使用Mojo时的便利，接口的 client（即 Remote 端）通常是创建新管道的一方。这很方便，因为 Remote 可以用于立即开始发送消息，而无需等待 InterfaceRequest 端点在任何地方被传输或绑定。

```
// src/third_party/blink/example/public/ping_responder.h
mojo::Remote<example::mojom::PingResponder> ping_responder;
mojo::PendingReceiver<example::mojom::PingResponder> receiver =
    ping_responder.BindNewPipeAndPassReceiver();
```

在此示例中，ping_responder是Remote，并且receiver是PendingReceiver，这是Receiver的前身。 BindNewPipeAndPassReceiver是创建消息管道的最常⻅方法:它产生PendingReceiver作为返回值。 注意:一个PendingReceiver实际上不执行任何操作。它是单个消息管道端点的惰性持有者。它的存在 只是为了使其端点在编译时具有更强的类型，这表明该端点希望被绑定到具体的接口类型。





