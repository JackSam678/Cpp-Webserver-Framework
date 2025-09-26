// 表单验证
document.addEventListener('DOMContentLoaded', () => {
    const form = document.querySelector('form');
    
    form.addEventListener('submit', (e) => {
        let isValid = true;
        
        // 验证姓名
        const nameInput = document.getElementById('name');
        if (nameInput.value.trim() === '') {
            showError(nameInput, '请输入您的姓名');
            isValid = false;
        } else {
            clearError(nameInput);
        }
        
        // 验证邮箱
        const emailInput = document.getElementById('email');
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!emailRegex.test(emailInput.value)) {
            showError(emailInput, '请输入有效的邮箱地址');
            isValid = false;
        } else {
            clearError(emailInput);
        }
        
        // 如果表单无效，阻止提交
        if (!isValid) {
            e.preventDefault();
        }
    });
    
    // 显示错误信息
    function showError(input, message) {
        clearError(input);
        
        const errorElement = document.createElement('div');
        errorElement.className = 'text-red-500 text-sm mt-1';
        errorElement.textContent = message;
        errorElement.dataset.error = 'true';
        
        input.parentNode.appendChild(errorElement);
        input.classList.add('border-red-500');
    }
    
    // 清除错误信息
    function clearError(input) {
        input.classList.remove('border-red-500');
        
        const errorElement = input.parentNode.querySelector('[data-error="true"]');
        if (errorElement) {
            errorElement.remove();
        }
    }

    // 为当前页面导航链接添加活跃状态
    const navLinks = document.querySelectorAll('nav a');
    navLinks.forEach(link => {
        if (link.getAttribute('href') === '/form.html') {
            link.classList.add('text-blue-200', 'font-medium');
        }
    });
});