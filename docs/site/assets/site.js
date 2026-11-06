// Optional enhancements; the documentation and navigation also work without JS.
(() => {
  const experiment = document.querySelector('.experiment');
  if (experiment) {
    const baseline = 0x2d;
    const hex = value => `0x${value.toString(16).toUpperCase().padStart(2, '0')}`;
    const buttons = experiment.querySelectorAll('[data-bit]');
    const selectBit = bit => {
      const mask = 1 << bit;
      const result = baseline ^ mask;
      buttons.forEach(button => {
        const position = Number(button.dataset.bit);
        const selected = position === bit;
        button.classList.toggle('selected', selected);
        button.setAttribute('aria-pressed', String(selected));
        button.textContent = String((result >> position) & 1);
      });
      experiment.querySelector('[data-selected-bit]').textContent = String(bit);
      experiment.querySelector('[data-fault-hex]').textContent = hex(result);
      experiment.querySelector('[data-fault-decimal]').textContent = String(result);
      experiment.querySelector('[data-mask]').textContent = hex(mask);
      experiment.querySelector('[data-result]').textContent = hex(result);
    };
    buttons.forEach(button => button.addEventListener('click', () => selectBit(Number(button.dataset.bit))));
  }

  // Clipboard access is available on HTTPS and localhost. Omit the controls
  // where the browser cannot copy, leaving every command selectable normally.
  if (navigator.clipboard && window.isSecureContext) {
    const status = document.createElement('span');
    status.className = 'visually-hidden';
    status.setAttribute('role', 'status');
    document.body.append(status);
    document.querySelectorAll('pre > code').forEach(code => {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'copy-button';
      button.textContent = 'Copy';
      button.setAttribute('aria-label', 'Copy code to clipboard');
      button.addEventListener('click', async () => {
        try {
          await navigator.clipboard.writeText(code.textContent);
          button.textContent = 'Copied';
          button.setAttribute('aria-label', 'Code copied to clipboard');
          status.textContent = 'Code copied to clipboard.';
        } catch {
          button.textContent = 'Select text to copy';
          button.setAttribute('aria-label', 'Select the code text to copy');
          status.textContent = 'Clipboard unavailable. Select the code text to copy.';
        }
        setTimeout(() => {
          button.textContent = 'Copy';
          button.setAttribute('aria-label', 'Copy code to clipboard');
        }, 2000);
      });
      code.parentElement.prepend(button);
    });
  }
})();
