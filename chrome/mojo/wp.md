
在content::PlaidStoreImpl::Create处下断点：
```
   0x5555591ac4a0    mov    rbx, rdi   0x5555591ac4a3    mov    edi, 0x28   0x5555591ac4a8    call   0x55555ac584b0 <0x55555ac584b0>    0x5555591ac4ad    lea    rcx, [rip + 0x635e2ec]   0x5555591ac4b4    mov    qword ptr [rax], rcx   0x5555591ac4b7    mov    qword ptr [rax + 8], rbx   0x5555591ac4bb    lea    rcx, [rax + 0x18]   0x5555591ac4bf    xorps  xmm0, xmm0   0x5555591ac4c2    movups xmmword ptr [rax + 0x18], xmm0   0x5555591ac4c6    mov    qword ptr [rax + 0x10], rcx   0x5555591ac4ca    mov    qword ptr [rbp - 0x20], rax
```

![](./img/1.png)

```
pwndbg> x/10gx 0x3bcb6cdfe7200x3bcb6cdfe720:	0x000055555f50a7a0 <= vtable    	0x00003bcb6cd45400 <= render_frame_host0x3bcb6cdfe730:	0x00003bcb6ccd2870 <= data_store_	0x00003bcb6ccd2870
```

```
pwndbg> x/10gx 0x00003bcb6ccd28700x3bcb6ccd2870:	0x0000000000000000       	0x00000000000000000x3bcb6ccd2880:	0x00003bcb6cdfe738      	0x000055555824ff010x3bcb6ccd2890:	0x0000000000616161 <= key	0x00000000000000000x3bcb6ccd28a0:	0x0300000000000000      	0x00003bcb6cddf7400x3bcb6ccd28b0:	0x00003bcb6cddf750 <= data	0x00003bcb6cddf750
```

```
pwndbg> x/10gx 0x00003bcb6cddf7400x3bcb6cddf740:	0x3131313131313131	0x31313131313131310x3bcb6cddf750:	0xffffc40000000002	0xfffffffd55553ec20x3bcb6cddf760:	0xffffc40000000001	0xfffffffd55553ec20x3bcb6cddf770:	0xffffc40000000002	0xfffffffd55553ec20x3bcb6cddf780:	0xffffc40000000002	0xfffffffd55553ec2
```
