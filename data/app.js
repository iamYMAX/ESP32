document.addEventListener('DOMContentLoaded', () => {
    const controlsContainer = document.querySelector('.controls');
    // Пины, которые мы хотим контролировать (соответствуют main.cpp)
    const pins = [
        { pin: 2, name: 'Built-in LED' },
        { pin: 18, name: 'GPIO 18' },
        { pin: 19, name: 'GPIO 19' },
        { pin: 21, name: 'GPIO 21' }
    ];

    // Функция для создания карточки управления
    function createControlCard(pinInfo) {
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

    // Создаем карточки для всех пинов
    pins.forEach(createControlCard);

    // Функция для получения и обновления состояния пинов
    async function updatePinStates() {
        try {
            const response = await fetch('/status');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const states = await response.json();
            document.querySelectorAll('.switch input[type="checkbox"]').forEach(checkbox => {
                const pin = checkbox.getAttribute('data-pin');
                if (states.hasOwnProperty(pin)) {
                    checkbox.checked = states[pin];
                }
            });
        } catch (error) {
            console.error("Failed to fetch pin states:", error);
        }
    }

    // Функция для отправки команды на переключение пина
    async function togglePin(pin, state) {
        try {
            const response = await fetch(`/toggle?pin=${pin}&state=${state ? 1 : 0}`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            console.log(`Pin ${pin} toggled to ${state}`);
        } catch (error) {
            console.error(`Failed to toggle pin ${pin}:`, error);
            // Если не удалось отправить, откатываем состояние чекбокса
            updatePinStates();
        }
    }

    // Добавляем обработчики событий на переключатели
    controlsContainer.addEventListener('change', (event) => {
        const checkbox = event.target;
        if (checkbox.matches('.switch input[type="checkbox"]')) {
            const pin = checkbox.getAttribute('data-pin');
            const state = checkbox.checked;
            togglePin(pin, state);
        }
    });

    // Запрашиваем начальное состояние при загрузке страницы
    updatePinStates();
});
