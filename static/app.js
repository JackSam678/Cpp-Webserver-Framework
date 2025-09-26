// 页面加载完成后执行
document.addEventListener('DOMContentLoaded', () => {
    // 获取服务器状态
    fetch('/api/status')
        .then(response => {
            if (!response.ok) {
                throw new Error('网络响应不正常');
            }
            return response.json();
        })
        .then(data => {
            const statusDiv = document.getElementById('server-status');
            
            // 更新状态信息
            statusDiv.innerHTML = `
                <div class="bg-gray-50 p-4 rounded-lg border border-gray-100 fade-in">
                    <div class="text-sm text-gray-500 mb-1">当前时间</div>
                    <div class="text-xl font-mono">${data.time}</div>
                </div>
                <div class="bg-gray-50 p-4 rounded-lg border border-gray-100 fade-in" style="animation-delay: 0.1s">
                    <div class="text-sm text-gray-500 mb-1">已处理请求</div>
                    <div class="text-xl font-mono">${data.requests}</div>
                </div>
                <div class="bg-gray-50 p-4 rounded-lg border border-gray-100 fade-in" style="animation-delay: 0.2s">
                    <div class="text-sm text-gray-500 mb-1">线程池大小</div>
                    <div class="text-xl font-mono">${data.threads}</div>
                </div>
            `;
        })
        .catch(error => {
            console.error('获取服务器状态失败:', error);
            document.getElementById('server-status').innerHTML = `
                <div class="text-red-500">无法获取服务器状态</div>
            `;
        });

    // 为导航链接添加活跃状态
    const currentPath = window.location.pathname;
    const navLinks = document.querySelectorAll('nav a');
    
    navLinks.forEach(link => {
        if (link.getAttribute('href') === currentPath) {
            link.classList.add('text-blue-200', 'font-medium');
        }
    });
});