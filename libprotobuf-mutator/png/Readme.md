本次的例子来自于https://github.com/google/fuzzing/blob/master/docs/structure-aware-fuzzing.md 中的png部分。

本次主要是学习自己如何写一个protobuf变异器。

### png文件格式

关于png文件格式，这里就直接放链接了，有需要去查阅一下就好：
https://zh.wikipedia.org/wiki/PNG
https://blog.mythsman.com/post/5d2d62b4a2005d74040ef7eb/

因为png是一种光栅图形文件格式，它由一系列length-tag-value-checksum块组成，也因此如果没有一个合适的基于突变的fuzzer的话，效果并不是很好，我们这里就先学习谷歌官方的fuzz，之后在做自己的修改

### libpng

https://github.com/google/oss-fuzz/tree/master/projects/libpng-proto

首先我们需要将png格式以protobuf的形式表示出来，也就是下面的内容：
```
syntax = "proto2";
// Very simple proto description of the PNG format,
// described at https://en.wikipedia.org/wiki/Portable_Network_Graphics

message IHDR {
  required uint32 width = 1;
  required uint32 height = 2;
  required uint32 other1 = 3;
  required uint32 other2 = 4;  // Only 1 byte used.
}

message PLTE {
  required bytes data = 1;
}

message IDAT {
  required bytes data = 1;
}

message iCCP  {
  required bytes data = 2;
}

message OtherChunk {
  oneof type {
    uint32 known_type = 1;
    uint32 unknown_type = 2;
  }
  required bytes data = 3;
}

message PngChunk {
  oneof chunk {
    PLTE plte = 1;
    IDAT idat = 2;
    iCCP iccp = 3;
    OtherChunk other_chunk = 10000;
  }
}

message PngProto {
  required IHDR ihdr = 1;
  repeated PngChunk chunks = 2;
}

// package fuzzer_examples;
```
对于定义的protobuf，我们需要一个可以将protobuf转化为png文件的c++代码：

```
// Example fuzzer for PNG using protos.
#include <string>
#include <sstream>
#include <fstream>
#include <zlib.h>  // for crc32

#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "png_fuzz_proto.pb.h"


//首先定义一些工具函数
static void WriteInt(std::stringstream &out, uint32_t x) {
  x = __builtin_bswap32(x);
  out.write((char *)&x, sizeof(x));
}

static void WriteByte(std::stringstream &out, uint8_t x) {
  out.write((char *)&x, sizeof(x));
}

static std::string Compress(const std::string &s) {
  std::string out(s.size() + 100, '\0');
  size_t out_len = out.size();
  compress((uint8_t *)&out[0], &out_len, (uint8_t *)s.data(), s.size());
  out.resize(out_len);
  return out;
}

// Chunk is written as:
//  * 4-byte length
//  * 4-byte type
//  * the data itself
//  * 4-byte crc (of type and data)
static void WriteChunk(std::stringstream &out, const char *type,
                       const std::string &chunk, bool compress = false) {
  std::string compressed;
  const std::string *s = &chunk;
  if (compress) {
    compressed = Compress(chunk);
    s = &compressed;
  }
  uint32_t len = s->size();
  uint32_t crc = crc32(crc32(0, (const unsigned char *)type, 4),
                       (const unsigned char *)s->data(), s->size());
  WriteInt(out, len);
  out.write(type, 4);
  out.write(s->data(), s->size());
  WriteInt(out, crc);
}


std::string ProtoToPng(const PngProto &png_proto) {
  std::stringstream all;
  const unsigned char header[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}; //png头，这是一段固定的值，我们直接写死
  all.write((const char*)header, sizeof(header));
  std::stringstream ihdr_str;
  auto &ihdr = png_proto.ihdr();  //获取protobuf中的ihdr信息
  // Avoid large images.
  // They may have interesting bugs, but OOMs are going to kill fuzzing.
  uint32_t w = std::min(ihdr.width(), 4096U); //限制图片的最大长度
  uint32_t h = std::min(ihdr.height(), 4096U); //限制图片的最大宽度
  WriteInt(ihdr_str, w); //写入字节流
  WriteInt(ihdr_str, h); //写入字节流
  WriteInt(ihdr_str, ihdr.other1()); //将Bit depth写入字节流
  WriteByte(ihdr_str, ihdr.other2()); //将Colortype写入字节流
  WriteChunk(all, "IHDR", ihdr_str.str()); //将整个ihdr chunk写入总字节流

  for (size_t i = 0, n = png_proto.chunks_size(); i < n; i++) { //循环解析剩余的chunk
    auto &chunk = png_proto.chunks(i);
    if (chunk.has_plte()) { //如果有plte chunk
      WriteChunk(all, "PLTE", chunk.plte().data());
    } else if (chunk.has_idat()) { //如果有idat chunk
      WriteChunk(all, "IDAT", chunk.idat().data(), true);
    } else if (chunk.has_iccp()) { //如果有iccp chunk
      std::stringstream iccp_str;
      iccp_str << "xyz";  // don't fuzz iCCP name field.
      WriteByte(iccp_str, 0);
      WriteByte(iccp_str, 0);
      auto compressed_data = Compress(chunk.iccp().data());
      iccp_str.write(compressed_data.data(), compressed_data.size());
      WriteChunk(all, "iCCP", iccp_str.str());
    } else if (chunk.has_other_chunk()) { //如果有other chunk
      auto &other_chunk = chunk.other_chunk();
      char type[5] = {0};
      if (other_chunk.has_known_type()) { //如果other chunk是已知的类型
        static const char * known_chunks[] = {
            "bKGD", "cHRM", "dSIG", "eXIf", "gAMA", "hIST", "iCCP",
            "iTXt", "pHYs", "sBIT", "sPLT", "sRGB", "sTER", "tEXt",
            "tIME", "tRNS", "zTXt", "sCAL", "pCAL", "oFFs",
        };
        size_t known_chunks_size =
            sizeof(known_chunks) / sizeof(known_chunks[0]);
        size_t chunk_idx = other_chunk.known_type() % known_chunks_size;
        memcpy(type, known_chunks[chunk_idx], 4);
      } else if (other_chunk.has_unknown_type()) {
        uint32_t unknown_type_int = other_chunk.unknown_type();
        memcpy(type, &unknown_type_int, 4);
      } else {
        continue;
      }
      type[4] = 0;
      WriteChunk(all, type, other_chunk.data());
    }
  }
  WriteChunk(all, "IEND", "");

  std::string res = all.str();
  if (const char *dump_path = getenv("PROTO_FUZZER_DUMP_PATH")) {
    // With libFuzzer binary run this to generate a PNG file x.png:
    // PROTO_FUZZER_DUMP_PATH=x.png ./a.out proto-input
    std::ofstream of(dump_path);
    of.write(res.data(), res.size());
  }
  return res;
}

// The actual fuzz target that consumes the PNG data.
extern "C" int FuzzPNG(const uint8_t* data, size_t size);

DEFINE_PROTO_FUZZER(const PngProto &png_proto) {
  auto s = ProtoToPng(png_proto);
  FuzzPNG((const uint8_t*)s.data(), s.size());
}
```
上面的c++代码，主要就是对我们定义的protobuf做一个处理，
