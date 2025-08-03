#include "FileManagerWindow.h"
#include "dobby.h"
#include <QApplication>
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <unistd.h>

typedef void (*RequestShowPage_t)(void *this_ptr, int page, bool flag);

// 原始函数指针，用于调用未 hook 的函数
RequestShowPage_t orig_request_show_page = nullptr;

// 自定义的 hook 函数
void my_request_show_page(void *this_ptr, int page, bool flag) {
  if (page == 9) {
    printf("[+] 检测到页面9，启动主程序...\n");

    // 创建并显示主程序窗口
    static FileManagerWindow *mainWindow = nullptr;
    static QApplication *app = nullptr;

    // 如果窗口已关闭，重置指针以便重新创建
    if (mainWindow && !mainWindow->isVisible()) {
      delete mainWindow;
      mainWindow = nullptr;
    }

    if (!mainWindow) {
      // 创建 QApplication 实例（如果尚未创建）
      if (!app) {
        int argc = 1;
        char *argv[] = {(char *)"FileManager"};
        app = new QApplication(argc, argv);
      }

      // 创建并显示主窗口
      mainWindow = new FileManagerWindow();
      mainWindow->setAttribute(Qt::WA_QuitOnClose,
                               false); // 改为false，防止关闭时退出应用
      mainWindow->resize(320, 170);
      mainWindow->move(0, 0);
      mainWindow->show();

      // 运行 Qt 事件循环
      app->exec();
    } else {
      // 窗口已存在，将其置于前台
      mainWindow->raise();
      mainWindow->activateWindow();
    }

    return;
  }

  // 调用原始函数
  if (orig_request_show_page) {
    orig_request_show_page(this_ptr, page, flag);
  } else {
    printf("[!] 错误: 原始函数指针无效!\n");
  }
}

// 安装 hook
void install_hook() {
  void *target_addr = (void *)0x5d726c;

  if (!target_addr) {
    printf("[-] 找不到目标符号 _ZN7YGlobal15requestShowPageEib\n    "
           "可能是版本不匹配导致程序硬编码地址失效\n");
    return;
  }

  int ret = DobbyHook(target_addr, (void *)my_request_show_page,
                      (void **)&orig_request_show_page);

  if (ret == 0) {
    printf("[+] Dobby hook 成功安装在 %p\n", target_addr);
  } else {
    printf("[-] Dobby hook 安装失败，返回值: %d\n", ret);
  }
}

__attribute__((constructor)) static void init() { install_hook(); }
