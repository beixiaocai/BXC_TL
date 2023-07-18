#ifndef COMMON_H
#define COMMON_H
#include <time.h>
#include <string>
#include <chrono>
#pragma warning( disable : 4996 )
static int64_t getCurTimestamp()// 获取毫秒级时间戳（13位）
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).
        count();
}

static std::string getCurTimeStr() {
    const char* time_fmt = "%Y-%m-%d %H:%M:%S";
    time_t t = time(nullptr);
    char time_str[64];
    strftime(time_str, sizeof(time_str), time_fmt, localtime(&t));

    return time_str;
}

//  __FILE__ 获取源文件的相对路径和名字
//  __LINE__ 获取该行代码在文件中的行号
//  __func__ 或 __FUNCTION__ 获取函数名
#define LOGI(format, ...)  fprintf(stderr,"[I] %s [%s:%d] " format "\n", getCurTimeStr().data(),__func__,__LINE__,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[E] %s [%s:%d] " format "\n",getCurTimeStr().data(),__func__,__LINE__,##__VA_ARGS__)


#endif //COMMON_H