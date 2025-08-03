document.addEventListener('DOMContentLoaded', () => {
    // --- Элементы управления GPIO ---
    const controlsContainer = document.querySelector('.controls');
    const gpioPins = [
        { pin: 2, name: 'Built-in LED' },
        { pin: 18, name: 'GPIO 18' },
        { pin: 19, name: 'GPIO 19' },
        { pin: 21, name: 'GPIO 21' }
    ];

    // --- Элементы управления генератором ---
    const rpmSlider = document.getElementById('rpm-slider');
    const rpmValueSpan = document.getElementById('rpm-value');
    const patternSelect = document.getElementById('pattern-select');

    // Функция для создания карточки управления GPIO
    function createGpioCard(pinInfo) {
        const card = document.createElement('div');
        card.className = 'control-card';
        const title = document.createElement('h2');
        title.textContent = pinInfo.name;
        const switchLabel = document.createElement('label');
        switchLabel.className = 'switch';
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.setAttribute('data-pin', pinInfo.pin);
        const slider = document.createElement('span');
        slider.className = 'slider';
        switchLabel.appendChild(checkbox);
        switchLabel.appendChild(slider);
        card.appendChild(title);
        card.appendChild(switchLabel);
        controlsContainer.appendChild(card);
    }

    // Создаем карточки для всех пинов GPIO
    gpioPins.forEach(createGpioCard);

    // --- API-функции ---

    // Универсальная функция для отправки команд
    async function sendCommand(url) {
        try {
            const response = await fetch(url);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            console.log(`Command sent: ${url}`);
        } catch (error) {
            console.error(`Failed to send command: ${url}`, error);
        }
    }

    // Обновление всего состояния с сервера
    async function updateStatus() {
        try {
            const response = await fetch('/status');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);

            const status = await response.json();

            // Обновляем GPIO
            document.querySelectorAll('.switch input[type="checkbox"]').forEach(checkbox => {
                const pin = checkbox.getAttribute('data-pin');
                if (status.gpio && status.gpio.hasOwnProperty(pin)) {
                    checkbox.checked = status.gpio[pin];
                }
            });

            // Обновляем генератор
            if (status.generator) {
                rpmSlider.value = status.generator.rpm;
                rpmValueSpan.textContent = status.generator.rpm;
                patternSelect.value = status.generator.pattern;
            }

        } catch (error) {
            console.error("Failed to fetch status:", error);
        }
    }

    // --- Обработчики событий ---

    // Обработчик для GPIO
    controlsContainer.addEventListener('change', (event) => {
        const checkbox = event.target;
        if (checkbox.matches('.switch input[type="checkbox"]')) {
            const pin = checkbox.getAttribute('data-pin');
            const state = checkbox.checked;
            sendCommand(`/toggle?pin=${pin}&state=${state ? 1 : 0}`);
        }
    });

    // Обработчики для генератора
    rpmSlider.addEventListener('input', () => {
        rpmValueSpan.textContent = rpmSlider.value;
    });

    rpmSlider.addEventListener('change', () => {
        sendCommand(`/set_rpm?value=${rpmSlider.value}`);
    });

    patternSelect.addEventListener('change', () => {
        sendCommand(`/set_pattern?pattern=${patternSelect.value}`);
    });


    // Запрашиваем начальное состояние при загрузке страницы
    updateStatus();
});
