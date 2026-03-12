// stb_impl.cpp — stb_image / stb_image_write 实现文件
// 必须在且仅在一个 .cpp 中定义这两个宏
// MCDK_LITE（含 MCDK_SERVER）不使用像素画工具，跳过 stb 编译单元
#ifndef MCDK_LITE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#endif // MCDK_LITE
