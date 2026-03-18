// DOM Elements
const connStatus = document.getElementById('connection-status');
const sysState = document.getElementById('sys-state');
const sysSpeed = document.getElementById('sys-speed');
const configForm = document.getElementById('config-form');

// Polling interval
setInterval(fetchStatus, 500);

async function fetchStatus() {
    try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error('Network response was not ok');
        const data = await res.json();
        
        connStatus.textContent = 'Online';
        connStatus.className = 'status-badge online';
        
        sysState.textContent = data.state;
        sysSpeed.textContent = data.speed;
        
        // Only update form if it's not currently focused (being edited)
        if (document.activeElement.tagName !== "INPUT") {
            document.getElementById('speedFinal').value = data.speedFinal;
            document.getElementById('delta').value = data.delta;
            document.getElementById('gap').value = data.gap;
        }
    } catch (e) {
        connStatus.textContent = 'Offline';
        connStatus.className = 'status-badge offline';
    }
}

// Config Submit
configForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const formData = new FormData();
    formData.append('speedFinal', document.getElementById('speedFinal').value);
    formData.append('delta', document.getElementById('delta').value);
    formData.append('gap', document.getElementById('gap').value);

    try {
        await fetch('/api/config', { method: 'POST', body: formData });
        alert('Configuración guardada exitosamente');
    } catch (e) {
        alert('Error guardando configuración');
    }
});

// Controls Logic (Pushbutton behavior)
function setupButton(btnId, startCmd, stopCmd) {
    const btn = document.getElementById(btnId);
    
    const startAction = () => {
        const formData = new FormData();
        formData.append('cmd', startCmd);
        fetch('/api/action', { method: 'POST', body: formData }).catch(console.error);
    };
    
    const stopAction = () => {
        const formData = new FormData();
        formData.append('cmd', stopCmd);
        fetch('/api/action', { method: 'POST', body: formData }).catch(console.error);
    };

    // Mouse events
    btn.addEventListener('mousedown', startAction);
    btn.addEventListener('mouseup', stopAction);
    btn.addEventListener('mouseleave', stopAction); // In case cursor leaves while holding

    // Touch events for mobile
    btn.addEventListener('touchstart', (e) => { e.preventDefault(); startAction(); });
    btn.addEventListener('touchend', (e) => { e.preventDefault(); stopAction(); });
    btn.addEventListener('touchcancel', (e) => { e.preventDefault(); stopAction(); });
}

setupButton('btn-left', 'left_start', 'left_stop');
setupButton('btn-right', 'right_start', 'right_stop');
setupButton('btn-stop', 'estop', 'estop_release');

// Initial load
fetchStatus();
