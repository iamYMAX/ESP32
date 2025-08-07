document.addEventListener('DOMContentLoaded', () => {
    console.log('ECU Data Page Loaded');

    const addWidgetBtn = document.getElementById('add-widget-btn');
    const pidSelect = document.getElementById('pid-select');
    const widgetsContainer = document.getElementById('data-widgets-container');

    const socket = new WebSocket(`ws://${window.location.hostname}/ws`);

    socket.onopen = () => {
        console.log('WebSocket connection opened');
    };

    socket.onclose = () => {
        console.log('WebSocket connection closed');
    };

    socket.onmessage = (event) => {
        try {
            const message = JSON.parse(event.data);
            if (message.type === 'ecu_data') {
                updateWidget(message.pid, message.value);
            }
        } catch (e) {
            // Ignore non-JSON messages, like the signal graph data
        }
    };

    function updateWidget(pid, value) {
        const widget = widgetsContainer.querySelector(`.widget[data-pid="${pid}"]`);
        if (widget) {
            const valueEl = widget.querySelector('.value');
            if (valueEl) {
                valueEl.textContent = value;
            }
        }
    }

    const pidConfig = {
        'rpm': { title: 'Обороты двигателя', unit: 'RPM' },
        'speed': { title: 'Скорость', unit: 'км/ч' }
    };

    addWidgetBtn.addEventListener('click', () => {
        const selectedPid = pidSelect.value;
        createWidget(selectedPid);
    });

    widgetsContainer.addEventListener('click', (event) => {
        if (event.target.classList.contains('remove-btn')) {
            const widget = event.target.closest('.widget');
            if (widget) {
                widget.remove();
            }
        }
    });

    function createWidget(pid) {
        // Prevent duplicate widgets
        if (widgetsContainer.querySelector(`.widget[data-pid="${pid}"]`)) {
            console.log(`Widget for ${pid} already exists.`);
            return;
        }

        const config = pidConfig[pid];
        if (!config) {
            console.error(`Invalid PID: ${pid}`);
            return;
        }

        const widgetEl = document.createElement('div');
        widgetEl.className = 'widget';
        widgetEl.dataset.pid = pid;
        widgetEl.innerHTML = `
            <button class="remove-btn">&times;</button>
            <h3>${config.title}</h3>
            <p><span class="value">...</span> ${config.unit}</p>
        `;
        widgetsContainer.appendChild(widgetEl);
    }
});
