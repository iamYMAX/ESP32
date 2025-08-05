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


    // --- Инициализация ---
    // Создаем карточки GPIO
    gpioPins.forEach(createGpioCard);

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

    // --- Первый запуск ---
    updateStatus();
});
