/* ************************************************************************
> File Name:     Util.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:38:28 PM CST
> Description:   
 ************************************************************************/

#include "Util.h"
#include "../Log.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <execinfo.h>  // backtrace
#include <cxxabi.h>  // __cxa_demangle
#include <sstream>  // stringstream

namespace util
{

pid_t GetThreadId()
{
  return syscall(SYS_gettid);
}

static std::string demangle(const char* str)
{
  size_t size = 0;
  int status = 0;
  std::string rt;
  rt.resize(256);
  if(1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0]))
  {
    char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
    if(v)
    {
      std::string result(v);
      free(v);
      return result;
    }
  }
  if(1 == sscanf(str, "%255s", &rt[0]))
  {
    return rt;
  }
  return str;
}

void Backtrace(std::vector<std::string>& bt, int size, int skip)
{
  void** array = (void**)malloc((sizeof(void*) * size));
  size_t s = ::backtrace(array, size);

  char** strings = backtrace_symbols(array, s);
  if(strings == NULL)
  {
    Debug("backtrace_synbols error");
    return;
  }

  for(size_t i = skip; i < s; ++i)
  {
    bt.push_back(demangle(strings[i]));
  }

  free(strings);
  free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix)
{
  std::vector<std::string> bt;
  Backtrace(bt, size, skip);
  std::stringstream ss;
  for(size_t i = 0; i < bt.size(); ++i)
  {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

uint64_t GetCurrentMS()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

}  // util