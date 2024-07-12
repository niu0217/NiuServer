/* ************************************************************************
> File Name:     FdManager.h
> Author:        niu0217
> Created Time:  Fri 12 Jul 2024 10:41:22 AM CST
> Description:   文件句柄管理类
 ************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include "Thread.h"
#include "Singleton.h"

/// @brief 文件句柄上下文类
/// @details 管理文件句柄类型(是否socket)
///          是否阻塞,是否关闭,读/写超时时间
class FdCtx : public std::enable_shared_from_this<FdCtx>
{
public:
  typedef std::shared_ptr<FdCtx> ptr;

  FdCtx(int fd);
  ~FdCtx();

  /// @brief 是否初始化完成
  bool isInit() const { return m_isInit; }

  /// @brief 是否socket
  bool isSocket() const { return m_isSocket; }

  /// @brief 是否已关闭
  bool isClose() const { return m_isClosed; }

  /// @brief 用户主动设置非阻塞
  /// @param v 是否阻塞
  void setUserNonblock(bool v) { m_userNonblock = v; }

  /// @brief 获取是否用户主动设置的非阻塞
  bool getUserNonblock() const { return m_userNonblock; }

  /// @brief 设置系统非阻塞
  /// @param v 是否阻塞
  void setSysNonblock(bool v) { m_sysNonblock = v; }

  /// @brief 获取系统非阻塞
  bool getSysNonblock() const { return m_sysNonblock; }

  /// @brief 设置超时时间
  /// @param type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
  /// @param v 时间毫秒
  void setTimeout(int type, uint64_t v);

  /// @brief 获取超时时间
  /// @param type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
  /// @return 超时时间毫秒
  uint64_t getTimeout(int type);

private:
  /// @brief 初始化
  bool init();

private:
  bool m_isInit: 1;        // 是否初始化
  bool m_isSocket: 1;      // 是否socket
  bool m_sysNonblock: 1;   // 是否hook非阻塞
  bool m_userNonblock: 1;  // 是否用户主动设置非阻塞
  bool m_isClosed: 1;      // 是否关闭
  int m_fd;                // 文件句柄 
  uint64_t m_recvTimeout;  // 读超时时间毫秒
  uint64_t m_sendTimeout;  // 写超时时间毫秒
};

/// @brief 文件句柄管理类
class FdManager
{
public:
  typedef RWMutex RWMutexType;

  FdManager();

  /// @brief 获取/创建文件句柄类FdCtx
  /// @param fd 文件句柄
  /// @param auto_create 是否自动创建
  /// @return 返回对应文件句柄类FdCtx::ptr
  FdCtx::ptr get(int fd, bool auto_create = false);

  /// @brief 删除文件句柄类
  /// @param fd 文件句柄
  void del(int fd);

private:
  RWMutexType m_mutex;             // 读写锁
  std::vector<FdCtx::ptr> m_datas; // 文件句柄集合
};

/// 文件句柄单例
typedef Singleton<FdManager> FdMgr;