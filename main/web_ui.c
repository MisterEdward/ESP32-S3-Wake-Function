#include "web_ui.h"

// HTML content with embedded CSS and JavaScript for a modern Apple-inspired design
const char *index_html = R"=====(
<!DOCTYPE html>
<html lang="en" class="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Remote Wake-Up Control</title>
    <style>
        :root {
            --apple-blue: #007aff;
            --apple-green: #34c759;
            --apple-red: #ff3b30;
            --apple-gray: #8e8e93;
            --apple-light-gray: #f2f2f7;
            --apple-purple: #af52de;
            
            /* GTA6 inspired colors */
            --gta-orange: #ff7e00;
            --gta-pink: #ff2d6d;
            --gta-purple: #9000ff;
            
            --bg-color: #f5f5f7;
            --card-bg: rgba(255, 255, 255, 0.8);
            --text-color: #1d1d1f;
            --text-secondary: #8e8e93;
            --card-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            --border-color: rgba(255, 255, 255, 0.2);
        }
        
        .dark {
            --bg-color: #000000;
            --card-bg: rgba(28, 28, 30, 0.7);
            --text-color: #ffffff;
            --text-secondary: #8e8e93;
            --card-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            --border-color: rgba(255, 255, 255, 0.1);
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            -webkit-font-smoothing: antialiased;
        }
        
        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            display: flex;
            flex-direction: column;
            min-height: 100vh;
            padding: 20px;
            transition: background-color 0.3s ease;
            position: relative;
            overflow-x: hidden;
        }
        
        /* Decorative shapes */
        .shape {
            position: absolute;
            border-radius: 50%;
            filter: blur(120px);
            z-index: -1;
            opacity: 0.4;
        }
        
        .shape-1 {
            background: linear-gradient(135deg, rgba(255, 126, 0, 0.8), rgba(255, 45, 109, 0.8));
            width: 350px;
            height: 350px;
            top: 10%;
            right: 30%;
            animation: drift1 30s infinite alternate ease-in-out;
        }
        
        .shape-2 {
            background: linear-gradient(135deg, rgba(255, 45, 109, 0.8), rgba(144, 0, 255, 0.8));
            width: 300px;
            height: 300px;
            bottom: 20%;
            left: 30%;
            animation: drift2 25s infinite alternate-reverse ease-in-out;
        }
        
        .shape-3 {
            background: linear-gradient(135deg, rgba(255, 126, 0, 0.8), rgba(144, 0, 255, 0.8));
            width: 250px;
            height: 250px;
            top: 70%;
            right: 10%;
            animation: drift3 28s infinite alternate ease-in-out;
        }
        
        @keyframes drift1 {
            0% {
                transform: translate(0, 0) scale(1);
            }
            50% {
                transform: translate(-15vw, 10vh) scale(1.2);
            }
            100% {
                transform: translate(15vw, -10vh) scale(0.9);
            }
        }
        
        @keyframes drift2 {
            0% {
                transform: translate(0, 0) scale(1);
            }
            50% {
                transform: translate(15vw, -10vh) scale(1.2);
            }
            100% {
                transform: translate(-15vw, 10vh) scale(0.9);
            }
        }
        
        @keyframes drift3 {
            0% {
                transform: translate(0, 0) scale(1);
            }
            50% {
                transform: translate(-10vw, -8vh) scale(1.1);
            }
            100% {
                transform: translate(8vw, 10vh) scale(0.9);
            }
        }
        
        .container {
            max-width: 500px;
            width: 100%;
            margin: 0 auto;
            background-color: var(--card-bg);
            border-radius: 18px;
            overflow: hidden;
            box-shadow: var(--card-shadow);
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            border: 1px solid var(--border-color);
            transition: all 0.3s ease;
            margin-bottom: 20px;
            position: relative;
            z-index: 1;
        }
        
        header {
            padding: 24px;
            text-align: center;
            border-bottom: 1px solid var(--border-color);
            position: relative;
        }
        
        .theme-toggle {
            position: absolute;
            top: 20px;
            right: 20px;
            cursor: pointer;
            font-size: 20px;
            z-index: 2;
        }
        
        h1 {
            font-size: 24px;
            font-weight: 600;
            margin-bottom: 8px;
            color: var(--text-color);
        }
        
        .subtitle {
            font-size: 16px;
            color: var(--text-secondary);
            font-weight: 400;
        }
        
        .content {
            padding: 24px;
            transition: max-height 0.8s cubic-bezier(0.25, 0.1, 0.25, 1);
            position: relative;
        }
        
        .status {
            display: flex;
            align-items: center;
            margin-bottom: 32px;
        }
        
        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        
        .connected {
            background-color: var(--apple-green);
        }
        
        .disconnected {
            background-color: var(--apple-red);
        }
        
        .status-text {
            font-size: 16px;
            font-weight: 500;
        }
        
        .button {
            display: block;
            width: 100%;
            padding: 16px;
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s ease;
            margin: 12px 0;
            box-shadow: 0 2px 6px rgba(0, 0, 0, 0.2);
        }
        
        .button-blue {
            background: linear-gradient(135deg, rgba(255, 126, 0, 0.7), rgba(255, 45, 109, 0.7));
        }
        
        .button-purple {
            background: linear-gradient(135deg, rgba(255, 45, 109, 0.7), rgba(144, 0, 255, 0.7));
        }
        
        .button-orange {
            background: linear-gradient(135deg, rgba(255, 159, 10, 0.7), rgba(245, 125, 0, 0.7));
        }
        
        .button-red {
            background: linear-gradient(135deg, rgba(255, 45, 45, 0.8), rgba(200, 10, 10, 0.8));
        }
        
        .button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        }
        
        .button-blue:hover {
            background: linear-gradient(135deg, rgba(255, 126, 0, 0.75), rgba(255, 45, 109, 0.75));
        }
        
        .button-purple:hover {
            background: linear-gradient(135deg, rgba(255, 45, 109, 0.75), rgba(144, 0, 255, 0.75));
        }
        
        .button-orange:hover {
            background: linear-gradient(135deg, rgba(255, 159, 10, 0.75), rgba(245, 125, 0, 0.75));
        }
        
        .button-red:hover {
            background: linear-gradient(135deg, rgba(255, 45, 45, 0.75), rgba(200, 10, 10, 0.75));
        }
        
        .button:active {
            transform: translateY(1px);
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
            opacity: 0.95;
        }
        
        .esp-restart {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background: rgba(28, 28, 30, 0.7);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 14px;
            color: var(--text-color);
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            box-shadow: var(--card-shadow);
            cursor: pointer;
            transition: all 0.2s ease;
            display: flex;
            align-items: center;
            gap: 5px;
            z-index: 100;
            outline: none;
            -webkit-tap-highlight-color: transparent;
        }
        
        .esp-restart:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
        }
        
        .esp-restart:active {
            transform: translateY(1px);
        }
        
        footer {
            text-align: center;
            padding: 24px;
            font-size: 14px;
            color: var(--text-secondary);
            margin-top: auto;
            position: relative;
            z-index: 1;
        }
        
        @media (max-width: 600px) {
            .container {
                border-radius: 12px;
            }
            .button {
                padding: 18px;
                font-size: 20px;
            }
        }
    </style>
</head>
<body>
    <!-- Decorative Shapes -->
    <div class="shape shape-1"></div>
    <div class="shape shape-2"></div>
    <div class="shape shape-3"></div>
    
    <div class="container">
        <header>
            <div class="theme-toggle" id="themeToggle">🌙</div>
            <h1>Remote Wake-Up</h1>
            <p class="subtitle">Wake up your computer with a single tap</p>
        </header>
        
        <div class="content">
            <div class="status">
                <div id="statusIndicator" class="status-indicator"></div>
                <span id="statusText" class="status-text">Checking connection...</span>
            </div>
            
            <button id="wakeButton" class="button button-blue">Wake on USB</button>
            <button id="wolButton" class="button button-purple">Wake on LAN</button>
            
            <hr style="border: none; border-top: 1px solid var(--border-color); margin: 24px 0;">
            
            <button id="shutdownButton" class="button button-orange">Shutdown PC</button>
            <button id="restartButton" class="button button-red" style="margin-bottom: 0;">Restart PC</button>
        </div>
    </div>
    
    <footer>
        ESP32 Remote Wake-Up Control | MAC: 70-85-C2-FA-D0-27
    </footer>
    
    <button id="espRestartButton" class="esp-restart">🔄 Restart ESP</button>
    
    <script>
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');
        const wakeButton = document.getElementById('wakeButton');
        const wolButton = document.getElementById('wolButton');
        const shutdownButton = document.getElementById('shutdownButton');
        const restartButton = document.getElementById('restartButton');
        const espRestartButton = document.getElementById('espRestartButton');
        const themeToggle = document.getElementById('themeToggle');
        const html = document.documentElement;
        const macAddress = '70-85-C2-FA-D0-27';

        function toggleTheme() {
            if (html.classList.contains('dark')) {
                html.classList.remove('dark');
                themeToggle.textContent = '☀️';
                localStorage.setItem('theme', 'light');
            } else {
                html.classList.add('dark');
                themeToggle.textContent = '🌙';
                localStorage.setItem('theme', 'dark');
            }
        }

        themeToggle.addEventListener('click', toggleTheme);

        const savedTheme = localStorage.getItem('theme') || 'dark';
        if (savedTheme === 'light') {
            html.classList.remove('dark');
            themeToggle.textContent = '☀️';
        } else {
            html.classList.add('dark');
            themeToggle.textContent = '🌙';
        }

        function checkConnection() {
            fetch('/wakeup', { method: 'HEAD' })
                .then(() => {
                    statusIndicator.classList.add('connected');
                    statusIndicator.classList.remove('disconnected');
                    statusText.textContent = '⌨️ Connected';
                })
                .catch(() => {
                    statusIndicator.classList.add('disconnected');
                    statusIndicator.classList.remove('connected');
                    statusText.textContent = 'Connection issue';
                });
        }

        checkConnection();
        
        // ESP restart button handler
        espRestartButton.addEventListener('click', () => {
            if (confirm('Are you sure you want to restart the ESP32 device? The connection will be lost temporarily.')) {
                espRestartButton.textContent = '🔄 Restarting...';
                espRestartButton.disabled = true;
                
                fetch('/esp-restart')
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('Failed to restart ESP');
                        }
                        // Show a countdown for reconnection
                        let countdown = 10;
                        statusText.textContent = `ESP restarting... Reconnecting in ${countdown}s`;
                        const intervalId = setInterval(() => {
                            countdown--;
                            if (countdown <= 0) {
                                clearInterval(intervalId);
                                window.location.reload();
                            } else {
                                statusText.textContent = `ESP restarting... Reconnecting in ${countdown}s`;
                            }
                        }, 1000);
                    })
                    .catch(error => {
                        console.error('Error:', error.message);
                        alert('Failed to restart ESP. Please try again.');
                        espRestartButton.textContent = '🔄 Restart ESP';
                        espRestartButton.disabled = false;
                    });
            }
        });

        wakeButton.addEventListener('click', () => {
            document.body.classList.add('loading');
            wakeButton.textContent = 'Sending...';
            wakeButton.disabled = true;

            fetch('/wakeup')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Failed to send wake signal');
                    }
                    return response.json();
                })
                .catch(error => {
                    console.error('Error:', error.message);
                })
                .finally(() => {
                    document.body.classList.remove('loading');
                    wakeButton.textContent = 'Wake on USB';
                    wakeButton.disabled = false;
                });
        });

        wolButton.addEventListener('click', () => {
            document.body.classList.add('loading');
            wolButton.textContent = 'Sending...';
            wolButton.disabled = true;

            fetch('/wol?mac=' + encodeURIComponent(macAddress))
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Failed to send WoL packet');
                    }
                    setTimeout(checkConnection, 5000);
                    return response.json();
                })
                .catch(error => {
                    console.error('Error:', error.message);
                })
                .finally(() => {
                    document.body.classList.remove('loading');
                    wolButton.textContent = 'Wake on LAN';
                    wolButton.disabled = false;
                });
        });

        // Shutdown PC button handler
        shutdownButton.addEventListener('click', () => {
            if (confirm("Are you sure you want to shutdown the computer? This may cause data loss if unsaved work is present.")) {
                shutdownButton.textContent = 'Shutting down...';
                shutdownButton.disabled = true;

                fetch('/shutdown')
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('Failed to send shutdown signal');
                        }
                        return response.json();
                    })
                    .catch(error => {
                        console.error('Error:', error.message);
                        alert(error.message);
                    })
                    .finally(() => {
                        shutdownButton.textContent = 'Shutdown PC';
                        shutdownButton.disabled = false;
                    });
            }
        });

        restartButton.addEventListener('click', () => {
            if (confirm("Are you sure you want to restart the computer? This may cause data loss if unsaved work is present.")) {
                document.body.classList.add('loading');
                restartButton.textContent = 'Restarting...';
                restartButton.disabled = true;

                fetch('/restart')
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('Failed to send restart signal');
                        }
                        return response.json();
                    })
                    .catch(error => {
                        console.error('Error:', error.message);
                    })
                    .finally(() => {
                        document.body.classList.remove('loading');
                        restartButton.textContent = 'Restart PC';
                        restartButton.disabled = false;
                    });
            }
        });
    </script>
</body>
)=====";
