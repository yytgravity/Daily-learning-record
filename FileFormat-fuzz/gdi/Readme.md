简单说一下图片生成：

这里直接剽窃一张图片（老懒狗了）：

![](./img/1.png)

emf图片在每做一次处理后，就会产生一个用于记录的结构体，例如：图中的EMRSETDIBITSTODEVICE

图中的不同bHandle对应了对不同结构的处理，可以对应起来写harness。
![](./img/2.png)