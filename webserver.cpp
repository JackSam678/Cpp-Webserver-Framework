#include "webserver.h"
#include <arpa/inet.h>

// URL解码函数实现
std::string urlDecode(const std::string& s) {
    std::string res;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%') {
            if (i + 2 < s.size()) {
                char hex1 = s[i+1];
                char hex2 = s[i+2];
                int val = 0;
                if (hex1 >= '0' && hex1 <= '9') val += (hex1 - '0') * 16;
                else if (hex1 >= 'A' && hex1 <= 'F') val += (hex1 - 'A' + 10) * 16;
                else if (hex1 >= 'a' && hex1 <= 'f') val += (hex1 - 'a' + 10) * 16;
                
                if (hex2 >= '0' && hex2 <= '9') val += (hex2 - '0');
                else if (hex2 >= 'A' && hex2 <= 'F') val += (hex2 - 'A' + 10);
                else if (hex2 >= 'a' && hex2 <= 'f') val += (hex2 - 'a' + 10);
                
                res += (char)val;
                i += 2;
                continue;
            }
        } else if (s[i] == '+') {
            res += ' ';
            continue;
        }
        res += s[i];
    }
    return res;
}

// Request类实现：解析HTTP请求
bool Request::parse(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    // 解析请求行（方法、路径、协议）
    if (!std::getline(iss, line)) return false;
    std::istringstream request_line(line);
    if (!(request_line >> method_ >> path_)) return false;

    // 解析查询参数（路径中的?后面部分）
    size_t query_pos = path_.find('?');
    if (query_pos != std::string::npos) {
        std::string query = path_.substr(query_pos + 1);
        path_ = path_.substr(0, query_pos);

        // 解析key=value形式的查询参数
        size_t pos = 0;
        while (pos < query.size()) {
            size_t eq_pos = query.find('=', pos);
            size_t and_pos = query.find('&', pos);
            if (eq_pos == std::string::npos || eq_pos > and_pos) break;

            std::string key = query.substr(pos, eq_pos - pos);
            std::string value = (and_pos != std::string::npos) 
                ? query.substr(eq_pos + 1, and_pos - eq_pos - 1)
                : query.substr(eq_pos + 1);
            
            // URL解码
            key = urlDecode(key);
            value = urlDecode(value);
            
            query_params_[key] = value;
            pos = (and_pos != std::string::npos) ? and_pos + 1 : query.size();
        }
    }

    // 解析请求头
    while (std::getline(iss, line) && line != "\r") {
        if (line.back() == '\r') line.pop_back(); // 移除\r
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // 跳过冒号和空格
            headers_[key] = value;
        }
    }

    // 解析请求体（简化实现，仅处理Content-Length指定的长度）
    auto it = headers_.find("Content-Length");
    if (it != headers_.end()) {
        try {
            size_t len = std::stoul(it->second);
            char* buffer = new char[len + 1];
            iss.read(buffer, len);
            buffer[len] = '\0';
            body_ = buffer;
            delete[] buffer;
        } catch (...) {
            return false;
        }
    }

    return true;
}

// Response类实现：构建和发送响应
std::string Response::buildResponse() const {
    std::string response;

    // 状态行
    response += "HTTP/1.1 " + std::to_string(status_code_) + " " + status_text_ + "\r\n";

    // 响应头（C++11兼容写法）
    for (std::map<std::string, std::string>::const_iterator it = headers_.begin(); 
         it != headers_.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }

    // 空行分隔头和体
    response += "\r\n";

    // 响应体
    response += body_;

    return response;
}

bool Response::send() const {
    std::string response = buildResponse();
    // 使用全局send函数（::避免与类方法冲突）
    ssize_t bytes_sent = ::send(socket_fd_, response.c_str(), response.size(), 0);
    if (bytes_sent == -1) {
        perror("发送响应失败");
        return false;
    }
    return true;
}

// Router类实现：处理路由和静态文件
void Router::handle(const Request& req, Response& res) const {
    // 先检查是否是静态文件请求
    if (!static_dir_.empty() && req.method() == "GET") {
        std::string file_path = static_dir_ + req.path();
        
        // 处理根路径请求（返回index.html）
        if (req.path() == "/") {
            file_path += "index.html";
        }
        
        // 尝试打开文件
        std::ifstream file(file_path, std::ios::binary);
        if (file.good()) {
            // 读取文件内容
            std::string content((std::istreambuf_iterator<char>(file)), 
                               std::istreambuf_iterator<char>());
            
            // 设置正确的Content-Type
            std::string ext = file_path.substr(file_path.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            std::map<std::string, std::string> mime_types = {
                {"html", "text/html"},
                {"css", "text/css"},
                {"js", "application/javascript"},
                {"png", "image/png"},
                {"jpg", "image/jpeg"},
                {"jpeg", "image/jpeg"},
                {"gif", "image/gif"},
                {"ico", "image/x-icon"},
                {"svg", "image/svg+xml"}
            };
            
            if (mime_types.count(ext)) {
                res.setHeader("Content-Type", mime_types[ext]);
            } else {
                res.setHeader("Content-Type", "application/octet-stream");
            }
            
            res.setContent(content);
            return;
        }
    }

    // 处理路由
    auto method_it = routes_.find(req.method());
    if (method_it != routes_.end()) {
        auto path_it = method_it->second.find(req.path());
        if (path_it != method_it->second.end()) {
            path_it->second(req, res);
            return;
        }
    }
    
    // 未找到路由，使用404处理函数
    not_found_handler_(req, res);
}

// WebServer类实现：服务器核心逻辑
WebServer::WebServer(int port, size_t thread_count)
    : port_(port) {
    // 初始化线程池（C++11兼容方式）
    thread_pool_.reset(new ThreadPool(thread_count));

    // 初始化地址结构
    std::memset(&address_, 0, sizeof(address_));
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);
}

void WebServer::handleClient(int client_socket) {
    // 增加请求计数
    incrementRequestCount();
    
    // 读取客户端请求
    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';

    // 解析请求
    Request req;
    if (!req.parse(buffer)) {
        Response res(client_socket);
        res.setStatusCode(400, "Bad Request");
        res.setHtml("<html>"
                    "<head><title>400 Bad Request</title></head>"
                    "<body><h1>400 Bad Request</h1></body></html>");
        res.send();
        close(client_socket);
        return;
    }

    // 处理请求
    Response res(client_socket);
    router_.handle(req, res);
    res.send();

    // 关闭连接
    close(client_socket);
}

bool WebServer::start() {
    // 创建服务器套接字
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == 0) {
        perror("socket创建失败");
        return false;
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt失败");
        close(server_fd_);
        return false;
    }

    // 绑定端口
    if (bind(server_fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
        perror("绑定端口失败");
        close(server_fd_);
        return false;
    }

    // 开始监听
    if (listen(server_fd_, 10) < 0) {
        perror("监听失败");
        close(server_fd_);
        return false;
    }

    std::cout << "服务器启动成功，监听端口 " << port_ << std::endl;
    std::cout << "线程池大小: " << thread_pool_->getWorkerCount() << std::endl;
    std::cout << "静态文件目录: " << router_.getStaticDir() << std::endl;  // 使用getter方法
    std::cout << "访问地址: http://localhost:" << port_ << std::endl;

    // 主循环：接受客户端连接
    while (true) {
        socklen_t addr_len = sizeof(address_);
        int client_socket = accept(server_fd_, (struct sockaddr*)&address_, &addr_len);
        if (client_socket < 0) {
            perror("接受连接失败");
            continue;
        }

        // 提交任务到线程池
        thread_pool_->enqueue([this, client_socket]() {
            handleClient(client_socket);
        });
    }

    return true;
}
    
