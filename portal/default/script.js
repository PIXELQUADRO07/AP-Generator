document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('loginForm');
    const connectBtn = document.getElementById('connectBtn');
    const terms = document.getElementById('acceptTerms');

    if (terms && connectBtn) {
        terms.addEventListener('change', () => {
            connectBtn.disabled = !terms.checked;
            connectBtn.style.opacity = terms.checked ? '1' : '0.5';
        });
    }

    if (form) {
        form.addEventListener('submit', (e) => {
            connectBtn.disabled = true;
            connectBtn.innerText = 'Connessione in corso...';
        });
    }
});
