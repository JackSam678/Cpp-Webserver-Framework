# Cpp-Webserver-Framework

一个轻量级、高性能的C++ Web服务器框架，支持静态文件服务、动态API、表单处理和多线程并发。

## 特性

- 基于C++17标准开发，跨平台兼容（Linux为主）
- 多线程处理并发请求，通过线程池提高性能
- 支持静态文件服务（HTML、CSS、JS、图片等）
- 动态路由系统，支持GET/POST等HTTP方法
- 表单数据处理与URL解码
- 简洁的API接口，易于扩展
- 响应式前端页面，基于Tailwind CSS构建

## 快速开始

### 环境要求

- C++17及以上编译器（g++ 7.3+ 或 clang++ 6.0+）
- Linux系统（依赖POSIX socket API）
- pthread库（通常系统自带）

### 编译与运行

1.克隆仓库：
  git clone https://github.com/JackSam678/Cpp-Webserver-Framework.git
  cd Cpp-Webserver-Framework
2.编译代码:
  g++ webserver.cpp main.cpp -o webserver -lpthread -std=c++17

3.启动服务器：
  ./webserver

4.在浏览器访问：
  http://localhost:8080
