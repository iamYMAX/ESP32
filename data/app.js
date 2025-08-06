document.addEventListener('DOMContentLoaded', () => {
    // --- Элементы управления GPIO ---
    const controlsContainer = document.querySelector('.controls');
    const gpioPins = [
        { pin: 2, name: 'Built-in LED' },
        { pin: 18, name: 'GPIO 18' },
        { pin: 19, name: 'GPIO 19' },
        { pin: 12, name: 'GPIO 12' }
    ];

    // --- Элементы управления генератором ДПКВ ---
    const rpmSlider = document.getElementById('rpm-slider');
    const rpmValueSpan = document.getElementById('rpm-value');
    const patternSelect = document.getElementById('pattern-select');

    // --- Элементы управления зажиганием ---
    const dwellSlider = document.getElementById('dwell-time-input');
    const dwellValueSpan = document.getElementById('dwell-value');
    const angleSlider = document.getElementById('ignition-angle-input');
    const angleValueSpan = document.getElementById('angle-value');

    // --- Элементы управления реле ---
    const relayModeSelect = document.getElementById('relay-mode-select');
    const pwmControlSection = document.getElementById('pwm-control-section');
    const pwmSlider = document.getElementById('pwm-duty-slider');
    const pwmValueSpan = document.getElementById('pwm-value');

    // --- Элементы управления дисплеем ---
    const nextScreenBtn = document.getElementById('next-screen-btn');

    // --- Элементы управления форсунками ---
    const injectorsContainer = document.getElementById('injectors-container');
    const cleanInjectorsBtn = document.getElementById('clean-injectors-btn');
    const NUM_INJECTORS = 4;


    // --- Инициализация ---
    // Создаем карточки GPIO
    gpioPins.forEach(createGpioCard);
    // Создаем карточки управления форсунками
    for (let i = 0; i < NUM_INJECTORS; i++) {
        createInjectorCard(i);
    }


    // --- API-функции ---
    async function sendCommand(url) {
        try {
            const response = await fetch(url);
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            console.log(`Command sent: ${url}`);
        } catch (error) {
            console.error(`Failed to send command: ${url}`, error);
        }
    }

    async function updateStatus() {
        try {
            const response = await fetch('/status');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const status = await response.json();

            // GPIO
            if (status.gpio) {
                document.querySelectorAll('.switch input[type="checkbox"]').forEach(checkbox => {
                    const pin = checkbox.getAttribute('data-pin');
                    if (status.gpio.hasOwnProperty(pin)) checkbox.checked = status.gpio[pin];
                });
            }
            // Crank Generator
            if (status.generator) {
                rpmSlider.value = status.generator.rpm;
                rpmValueSpan.textContent = status.generator.rpm;
                patternSelect.value = status.generator.pattern;
            }
            // Ignition
            if (status.ignition) {
                dwellSlider.value = status.ignition.dwell;
                dwellValueSpan.textContent = parseFloat(status.ignition.dwell).toFixed(1);
                angleSlider.value = status.ignition.angle;
                angleValueSpan.textContent = status.ignition.angle;
            }
            // Relay
            if (status.relay) {
                relayModeSelect.value = status.relay.mode;
                pwmSlider.value = status.relay.pwm_duty;
                pwmValueSpan.textContent = status.relay.pwm_duty;
                pwmControlSection.style.display = status.relay.mode === 'pwm' ? 'block' : 'none';
            }
            // Injectors
            if (status.injectors) {
                status.injectors.forEach((inj, index) => {
                    document.querySelector(`.injector-enable[data-injector="${index}"]`).checked = inj.enabled;
                    const angleSlider = document.querySelector(`.injector-angle[data-injector="${index}"]`);
                    angleSlider.value = inj.angle;
                    document.getElementById(`inj-angle-val-${index}`).textContent = inj.angle;
                    const durationSlider = document.querySelector(`.injector-duration[data-injector="${index}"]`);
                    durationSlider.value = inj.duration;
                    document.getElementById(`inj-duration-val-${index}`).textContent = parseFloat(inj.duration).toFixed(1);
                });
            }

        } catch (error) {
            console.error("Failed to fetch status:", error);
        }
    }


    // --- Обработчики событий ---
    // GPIO
    controlsContainer.addEventListener('change', e => {
        if (e.target.matches('.switch input[type="checkbox"]')) {
            sendCommand(`/toggle?pin=${e.target.dataset.pin}&state=${e.target.checked ? 1 : 0}`);
        }
    });

    // Crank Generator
    rpmSlider.addEventListener('input', () => { rpmValueSpan.textContent = rpmSlider.value; });
    rpmSlider.addEventListener('change', () => { sendCommand(`/set_rpm?value=${rpmSlider.value}`); });
    patternSelect.addEventListener('change', () => { sendCommand(`/set_pattern?pattern=${patternSelect.value}`); });

    // Ignition
    dwellSlider.addEventListener('input', () => { dwellValueSpan.textContent = parseFloat(dwellSlider.value).toFixed(1); });
    dwellSlider.addEventListener('change', () => { sendCommand(`/set_ignition_params?dwell=${dwellSlider.value}`); });
    angleSlider.addEventListener('input', () => { angleValueSpan.textContent = angleSlider.value; });
    angleSlider.addEventListener('change', () => { sendCommand(`/set_ignition_params?angle=${angleSlider.value}`); });

    // Relay
    relayModeSelect.addEventListener('change', () => {
        const mode = relayModeSelect.value;
        pwmControlSection.style.display = mode === 'pwm' ? 'block' : 'none';
        sendCommand(`/set_relay_mode?mode=${mode}`);
    });
    pwmSlider.addEventListener('input', () => { pwmValueSpan.textContent = pwmSlider.value; });
    pwmSlider.addEventListener('change', () => { sendCommand(`/set_relay_mode?mode=pwm&value=${pwmSlider.value}`); });

    // Display
    nextScreenBtn.addEventListener('click', () => { sendCommand('/next_screen'); });

    // Injectors
    cleanInjectorsBtn.addEventListener('click', () => { sendCommand('/start_injector_cleaning'); });

    injectorsContainer.addEventListener('input', e => {
        if (!e.target.dataset.injector) return;
        const index = e.target.dataset.injector;
        if (e.target.matches('.injector-angle')) {
            document.getElementById(`inj-angle-val-${index}`).textContent = e.target.value;
        } else if (e.target.matches('.injector-duration')) {
            document.getElementById(`inj-duration-val-${index}`).textContent = parseFloat(e.target.value).toFixed(1);
        }
    });

    injectorsContainer.addEventListener('change', e => {
        if (!e.target.dataset.injector) return;
        const index = e.target.dataset.injector;
        const enabled = document.querySelector(`.injector-enable[data-injector="${index}"]`).checked;
        const angle = document.querySelector(`.injector-angle[data-injector="${index}"]`).value;
        const duration = document.querySelector(`.injector-duration[data-injector="${index}"]`).value;

        sendCommand(`/set_injector_params?injector=${index}&enabled=${enabled}&angle=${angle}&duration=${duration}`);
    });


    // --- Вспомогательные функции ---
    function createGpioCard(pinInfo) {
        const card = document.createElement('div');
        card.className = 'control-card';
        const title = document.createElement('h2');
        title.textContent = pinInfo.name;
        const switchLabel = document.createElement('label');
        switchLabel.className = 'switch';
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.dataset.pin = pinInfo.pin;
        const slider = document.createElement('span');
        slider.className = 'slider';
        switchLabel.appendChild(checkbox);
        switchLabel.appendChild(slider);
        card.appendChild(title);
        card.appendChild(switchLabel);
        controlsContainer.appendChild(card);
    }

    function createInjectorCard(index) {
        const injectorDiv = document.createElement('div');
        injectorDiv.className = 'injector-control-group';
        injectorDiv.innerHTML = `
            <div class="injector-title">
                <h4>Форсунка ${index + 1}</h4>
                <label class="switch">
                    <input type="checkbox" data-injector="${index}" class="injector-enable">
                    <span class="slider"></span>
                </label>
            </div>
            <div class="generator-control">
                <label>Угол открытия (°ДПКВ): <span id="inj-angle-val-${index}">0</span></label>
                <input type="range" min="0" max="720" value="0" class="rpm-slider injector-angle" data-injector="${index}">
            </div>
            <div class="generator-control">
                <label>Длительность (мс): <span id="inj-duration-val-${index}">0.0</span></label>
                <input type="range" min="0" max="20" value="0" step="0.1" class="rpm-slider injector-duration" data-injector="${index}">
            </div>
        `;
        injectorsContainer.appendChild(injectorDiv);
    }

    // --- Первый запуск ---
    updateStatus();

    // --- WebSocket and Chart ---
    const chartCtx = document.getElementById('signalChart').getContext('2d');
    const signalChart = new Chart(chartCtx, {
        type: 'line',
        data: {
            labels: Array.from(Array(100).keys()),
            datasets: [{
                label: 'Pin 5 Signal',
                data: [],
                borderColor: 'rgb(75, 192, 192)',
                tension: 0.1,
                pointRadius: 0
            }]
        },
        options: {
            animation: false,
            scales: {
                y: {
                    min: -0.1,
                    max: 1.1,
                    ticks: {
                        stepSize: 1
                    }
                },
                x: {
                    display: false
                }
            }
        }
    });

    const socket = new WebSocket(`ws://${window.location.hostname}/ws`);
    socket.binaryType = 'arraybuffer';

    socket.onopen = () => console.log('WebSocket connection opened');
    socket.onclose = () => console.log('WebSocket connection closed');
    socket.onmessage = (event) => {
        const data = new Uint8Array(event.data);
        const chartData = signalChart.data.datasets[0].data;

        for (let i = 0; i < data.length; i++) {
            chartData.push(data[i]);
        }

        while (chartData.length > 100) {
            chartData.shift();
        }

        signalChart.update();
    };
});
