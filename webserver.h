#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstddef>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <ctime>
#include <algorithm>

// 声明urlDecode函数
std::string urlDecode(const std::string& s);

// 前置声明
class Request;
class Response;

// 请求类：解析HTTP请求
class Request {
private:
    std::string method_;
    std::string path_;
    std::string body_;
    std::map<std::string, std::string> query_params_;
    std::map<std::string, std::string> headers_;

public:
    Request() = default;
    ~Request() = default;

    // 解析原始请求数据
    bool parse(const std::string& data);

    // 获取请求方法（GET/POST等）
    const std::string& method() const { return method_; }
    // 获取请求路径
    const std::string& path() const { return path_; }
    // 获取请求体
    const std::string& body() const { return body_; }
    // 获取查询参数
    std::string queryParam(const std::string& key) const {
        auto it = query_params_.find(key);
        return (it != query_params_.end()) ? it->second : "";
    }
    // 获取请求头
    std::string header(const std::string& key) const {
        auto it = headers_.find(key);
        return (it != headers_.end()) ? it->second : "";
    }
};

// 响应类：构建HTTP响应
class Response {
private:
    int socket_fd_;
    int status_code_ = 200;
    std::string status_text_ = "OK";
    std::map<std::string, std::string> headers_;
    std::string body_;

    // 构建完整响应字符串
    std::string buildResponse() const;

public:
    Response(int socket_fd) : socket_fd_(socket_fd) {
        // 设置默认响应头
        headers_["Content-Type"] = "text/html; charset=UTF-8";
        headers_["Connection"] = "close";
    }
    ~Response() = default;

    // 设置状态码
    void setStatusCode(int code, const std::string& text) {
        status_code_ = code;
        status_text_ = text;
    }

    // 设置响应头
    void setHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

    // 设置HTML响应体
    void setHtml(const std::string& html) {
        body_ = html;
        headers_["Content-Length"] = std::to_string(html.size());
    }

    // 设置通用内容（用于二进制数据）
    void setContent(const std::string& content) {
        body_ = content;
        headers_["Content-Length"] = std::to_string(content.size());
    }

    // 发送响应
    bool send() const;
};

// 路由处理函数类型
using HandlerFunc = std::function<void(const Request&, Response&)>;

// 路由类：管理URL与处理函数的映射
class Router {
private:
    std::map<std::string, std::map<std::string, HandlerFunc>> routes_;
    HandlerFunc not_found_handler_;
    std::string static_dir_;

public:
    Router() {
        // 默认404处理函数
        not_found_handler_ = [](const Request& req, Response& res) {
            res.setStatusCode(404, "Not Found");
            res.setHtml("<html>"
                        "<head><title>404 Not Found</title></head>"
                        "<body><h1>404 Not Found</h1></body></html>");
        };
    }

    // 注册GET请求处理
    void get(const std::string& path, HandlerFunc handler) {
        routes_["GET"][path] = handler;
    }

    // 注册POST请求处理
    void post(const std::string& path, HandlerFunc handler) {
        routes_["POST"][path] = handler;
    }

    // 设置静态文件目录
    void setStaticDir(const std::string& dir) {
        static_dir_ = dir;
    }

    // 获取静态文件目录
    const std::string& getStaticDir() const {
        return static_dir_;
    }

    // 处理请求
    void handle(const Request& req, Response& res) const;

    // 设置404处理函数
    void setNotFoundHandler(HandlerFunc handler) {
        not_found_handler_ = handler;
    }
};

// 线程池类：管理工作线程
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    bool stop_ = false;
    std::vector<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;

    // 工作线程函数
    void workerThread() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = tasks_.front();
                tasks_.erase(tasks_.begin());
            }
            task();
        }
    }

public:
    // 构造函数：创建指定数量的工作线程
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&ThreadPool::workerThread, this);
        }
    }

    // 析构函数：停止所有工作线程
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    // 添加任务到线程池
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.push_back(task);
        }
        condition_.notify_one();
    }

    // 获取工作线程数量（公开接口）
    size_t getWorkerCount() const {
        return workers_.size();
    }
};

// Web服务器类：核心服务类
class WebServer {
private:
    int port_;
    int server_fd_ = -1;
    std::unique_ptr<ThreadPool> thread_pool_;
    Router router_;
    sockaddr_in address_;
    size_t request_count_ = 0;
    mutable std::mutex request_mutex_;

    // 处理客户端连接
    void handleClient(int client_socket);

public:
    // 构造函数：指定端口和线程数量
    WebServer(int port, size_t thread_count);
    ~WebServer() {
        if (server_fd_ != -1) {
            close(server_fd_);
        }
    }

    // 获取路由实例
    Router& router() { return router_; }

    // 设置404处理函数
    void setNotFoundHandler(HandlerFunc handler) {
        router_.setNotFoundHandler(handler);
    }

    // 获取请求计数
    size_t getRequestCount() const {
        std::lock_guard<std::mutex> lock(request_mutex_);
        return request_count_;
    }

    // 增加请求计数
    void incrementRequestCount() {
        std::lock_guard<std::mutex> lock(request_mutex_);
        request_count_++;
    }

    // 获取线程池
    const std::unique_ptr<ThreadPool>& thread_pool() const {
        return thread_pool_;
    }

    // 启动服务器
    bool start();
};

#endif // WEBSERVER_H
    
