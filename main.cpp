#include "webserver.h"

// 解析表单数据
std::map<std::string, std::string> parseFormData(const std::string& body) {
    std::map<std::string, std::string> form_data;
    size_t pos = 0;
    
    while (pos < body.size()) {
        size_t eq_pos = body.find('=', pos);
        size_t and_pos = body.find('&', pos);
        if (eq_pos == std::string::npos) break;
        
        std::string key = body.substr(pos, eq_pos - pos);
        std::string value = (and_pos != std::string::npos) 
            ? body.substr(eq_pos + 1, and_pos - eq_pos - 1)
            : body.substr(eq_pos + 1);
        
        // URL解码
        key = urlDecode(key);
        value = urlDecode(value);
        
        form_data[key] = value;
        pos = (and_pos != std::string::npos) ? and_pos + 1 : body.size();
    }
    
    return form_data;
}

int main() {
    // 创建Web服务器实例，监听8080端口，使用4个工作线程
    WebServer server(8080, 4);
    
    // 设置静态文件目录
    server.router().setStaticDir("./static");
    
    // 服务器状态API
    server.router().get("/api/status", [&server](const Request& req, Response& res) {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::ctime(&now_time);
        time_str.pop_back(); // 移除换行符

        // 构建JSON响应
        std::string json = "{";
        json += "\"time\": \"" + time_str + "\",";
        json += "\"requests\": " + std::to_string(server.getRequestCount()) + ",";
        json += "\"threads\": " + std::to_string(server.thread_pool()->getWorkerCount());
        json += "}";

        res.setHeader("Content-Type", "application/json");
        res.setContent(json);
    });
    
    // 处理表单提交
    server.router().post("/submit", [](const Request& req, Response& res) {
        // 解析表单数据
        std::map<std::string, std::string> form_data = parseFormData(req.body());

        // 生成提交结果页面
        std::string html = "<html>"
            "<head>"
            "<title>提交成功</title>"
            "<script src='https://cdn.tailwindcss.com'></script>"
            "<link href='https://cdn.jsdelivr.net/npm/font-awesome@4.7.0/css/font-awesome.min.css' rel='stylesheet'>"
            "<link rel='stylesheet' href='/css/style.css'>"
            "</head>"
            "<body class='bg-gray-50 min-h-screen'>"
            "<nav class='bg-blue-600 text-white shadow-md'>"
            "<div class='container mx-auto px-4 py-3 flex justify-between items-center'>"
            "<div class='text-xl font-bold'><i class='fa fa-server mr-2'></i>C++ Web Server</div>"
            "<div class='space-x-4'>"
            "<a href='/' class='hover:text-blue-200 transition'><i class='fa fa-home'></i> 首页</a>"
            "<a href='/about.html' class='hover:text-blue-200 transition'><i class='fa fa-info'></i> 关于</a>"
            "<a href='/form.html' class='hover:text-blue-200 transition'><i class='fa fa-form'></i> 表单</a>"
            "</div></div></nav>"
            "<div class='container mx-auto px-4 py-8 max-w-2xl'>"
            "<div class='bg-white rounded-lg shadow-lg p-6 mt-8'>"
            "<h1 class='text-2xl font-bold text-green-600 mb-4'><i class='fa fa-check-circle mr-2'></i>提交成功！</h1>"
            "<div class='space-y-3 text-gray-700'>"
            "<p><strong>姓名：</strong>" + form_data["name"] + "</p>"
            "<p><strong>邮箱：</strong>" + form_data["email"] + "</p>"
            "<p><strong>留言：</strong>" + form_data["message"] + "</p>"
            "</div>"
            "<div class='mt-6'><a href='/form.html' class='text-blue-600 hover:underline'><i class='fa fa-arrow-left mr-1'></i>返回表单</a></div>"
            "</div>"
            "</div>"
            "<footer class='bg-gray-800 text-white py-6 mt-12'>"
            "<div class='container mx-auto px-4 text-center'>"
            "<p>&copy; 2023 C++ Web服务器 | 基于C++17构建</p>"
            "</div></footer>"
            "</body>"
            "</html>";
        
        res.setHtml(html);
    });
    
    // 自定义404页面
    server.setNotFoundHandler([](const Request& req, Response& res) {
        res.setStatusCode(404, "Not Found");
        res.setHtml("<html>"
                    "<head>"
                    "<title>页面未找到</title>"
                    "<script src='https://cdn.tailwindcss.com'></script>"
                    "<link href='https://cdn.jsdelivr.net/npm/font-awesome@4.7.0/css/font-awesome.min.css' rel='stylesheet'>"
                    "</head>"
                    "<body class='bg-gray-50 min-h-screen'>"
                    "<nav class='bg-blue-600 text-white shadow-md'>"
                    "<div class='container mx-auto px-4 py-3 flex justify-between items-center'>"
                    "<div class='text-xl font-bold'><i class='fa fa-server mr-2'></i>C++ Web Server</div>"
                    "<div class='space-x-4'>"
                    "<a href='/' class='hover:text-blue-200 transition'><i class='fa fa-home'></i> 首页</a>"
                    "<a href='/about.html' class='hover:text-blue-200 transition'><i class='fa fa-info'></i> 关于</a>"
                    "<a href='/form.html' class='hover:text-blue-200 transition'><i class='fa fa-form'></i> 表单</a>"
                    "</div></div></nav>"
                    "<div class='container mx-auto px-4 py-16 text-center'>"
                    "<div class='inline-block p-8 bg-white rounded-lg shadow-lg'>"
                    "<div class='text-red-500 text-5xl mb-4'><i class='fa fa-exclamation-triangle'></i></div>"
                    "<h1 class='text-3xl font-bold text-gray-800 mb-2'>404 - 页面未找到</h1>"
                    "<p class='text-gray-600 mb-6'>抱歉，您请求的页面 \"" + req.path() + "\" 不存在</p>"
                    "<a href='/' class='bg-blue-600 text-white px-6 py-2 rounded-md hover:bg-blue-700 transition duration-300'>"
                    "<i class='fa fa-home mr-1'></i>返回首页"
                    "</a>"
                    "</div>"
                    "</div>"
                    "</body>"
                    "</html>");
    });
    
    // 启动服务器
    if (!server.start()) {
        std::cerr << "服务器启动失败" << std::endl;
        return 1;
    }
    
    return 0;
}
    
