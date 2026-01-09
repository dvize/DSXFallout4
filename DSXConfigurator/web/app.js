let config = [];
let metadata = {};
let currentEditIndex = -1;

document.addEventListener('DOMContentLoaded', async () => {
    await loadMetadata();
    await loadConfig();
    setupEventListeners();
});

async function loadMetadata() {
    try {
        const res = await fetch('/api/meta');
        metadata = await res.json();
        populateDropdowns();
    } catch (e) {
        console.error("Failed to load metadata", e);
        alert("Failed to load application metadata.");
    }
}

async function loadConfig() {
    const loading = document.getElementById('loading');
    loading.style.display = 'block';
    try {
        const res = await fetch('/api/config');
        config = await res.json();
        renderList();
    } catch (e) {
        console.error("Failed to load config", e);
        alert("Failed to load configuration.");
    } finally {
        loading.style.display = 'none';
    }
}

function populateDropdowns() {
    // Categories
    const catSelect = document.getElementById('entry-category');
    metadata.Categories.forEach(cat => {
        const opt = document.createElement('option');
        opt.value = cat;
        opt.textContent = cat;
        catSelect.appendChild(opt);
    });

    // Trigger Types
    const typeSelect = document.getElementById('entry-type');
    metadata.TriggerTypes.forEach(type => {
        const opt = document.createElement('option');
        opt.value = type.Value;
        opt.textContent = `${type.Value} - ${type.Name}`;
        typeSelect.appendChild(opt);
    });

    // Custom Trigger Modes
    const customSelect = document.getElementById('entry-custom-mode');
    metadata.CustomTriggerModes.forEach(mode => {
        const opt = document.createElement('option');
        opt.value = mode.Value;
        opt.textContent = `${mode.Value} - ${mode.Name}`;
        customSelect.appendChild(opt);
    });

    // Player LED
    const ledSelect = document.getElementById('entry-player-led');
    metadata.PlayerLEDModes.forEach(mode => {
        const opt = document.createElement('option');
        opt.value = mode.Value;
        opt.textContent = mode.Name;
        ledSelect.appendChild(opt);
    });

    // Mic LED
    const micSelect = document.getElementById('entry-mic-led');
    metadata.MicLEDModes.forEach(mode => {
        const opt = document.createElement('option');
        opt.value = mode.Value;
        opt.textContent = mode.Name;
        micSelect.appendChild(opt);
    });
}

function renderList() {
    const list = document.getElementById('config-list');
    list.innerHTML = '';

    config.forEach((entry, index) => {
        const isProtected = metadata.ProtectedNames[entry.Name];
        const card = document.createElement('div');
        card.className = `config-card ${isProtected ? 'protected' : ''}`;
        card.onclick = () => openEditModal(index);

        const typeName = metadata.TriggerTypes.find(t => t.Value === entry.TriggerType)?.Name || 'Unknown';

        card.innerHTML = `
            <div class="card-header">
                <span class="card-title">${entry.Name}</span>
                <span class="card-badge">${entry.Category}</span>
            </div>
            <div class="card-details">
                Side: ${entry.TriggerSide === 1 ? 'Left' : 'Right'} | Type: ${typeName}
            </div>
            <div class="card-desc">
                ${entry.Description || ''}
            </div>
        `;
        list.appendChild(card);
    });
}

function setupEventListeners() {
    document.getElementById('add-btn').onclick = () => openEditModal(-1);
    document.getElementById('save-btn').onclick = saveConfig;

    document.querySelector('.close').onclick = closeModal;
    window.onclick = (e) => {
        if (e.target == document.getElementById('edit-modal')) closeModal();
    };

    document.getElementById('edit-form').onsubmit = (e) => {
        e.preventDefault();
        applyChanges();
    };

    document.getElementById('delete-btn').onclick = deleteEntry;

    // Dynamic updates
    document.getElementById('entry-type').onchange = updateTriggerParamsUI;
    document.getElementById('entry-custom-mode').onchange = updateTriggerParamsUI;
}

function openEditModal(index) {
    currentEditIndex = index;
    const modal = document.getElementById('edit-modal');
    const title = document.getElementById('modal-title');
    const deleteBtn = document.getElementById('delete-btn');
    const form = document.getElementById('edit-form');

    modal.classList.remove('hidden');

    if (index === -1) {
        // New Entry
        title.textContent = "Add New Entry";
        deleteBtn.classList.add('hidden');
        form.reset();
        // Set defaults
        document.getElementById('entry-controller').value = 0;
        document.getElementById('entry-side').value = 2; // Right default
        document.getElementById('entry-type').value = 0;
        document.getElementById('entry-custom-mode').value = 0;
        document.getElementById('entry-player-led').value = 5; // AllOff
        document.getElementById('entry-mic-led').value = 2; // Off
        document.getElementById('entry-threshold').value = 0;
        document.getElementById('rgb-r').value = 0;
        document.getElementById('rgb-g').value = 0;
        document.getElementById('rgb-b').value = 0;
    } else {
        // Edit Entry
        const entry = config[index];
        title.textContent = `Edit: ${entry.Name}`;

        // Check protection
        if (metadata.ProtectedNames[entry.Name]) {
            deleteBtn.classList.add('hidden');
        } else {
            deleteBtn.classList.remove('hidden');
        }

        document.getElementById('entry-name').value = entry.Name;
        document.getElementById('entry-description').value = entry.Description || '';
        document.getElementById('entry-formid').value = entry.CustomFormID || '';
        document.getElementById('entry-category').value = entry.Category;
        document.getElementById('entry-controller').value = entry.ControllerIndex;
        document.getElementById('entry-side').value = entry.TriggerSide;
        document.getElementById('entry-type').value = entry.TriggerType;
        document.getElementById('entry-custom-mode').value = entry.customTriggerMode;
        document.getElementById('entry-player-led').value = entry.playerLEDNewRev;
        document.getElementById('entry-mic-led').value = entry.MicLEDMode;
        document.getElementById('entry-threshold').value = entry.TriggerThreshold;

        const rgb = entry.RGBUpdate || [0, 0, 0];
        document.getElementById('rgb-r').value = rgb[0];
        document.getElementById('rgb-g').value = rgb[1];
        document.getElementById('rgb-b').value = rgb[2];

        // Store params for UI update
        form.dataset.params = JSON.stringify(entry.TriggerParams || [0, 0, 0, 0]);
    }

    updateTriggerParamsUI();
}

function closeModal() {
    document.getElementById('edit-modal').classList.add('hidden');
}

function updateTriggerParamsUI() {
    const typeVal = parseInt(document.getElementById('entry-type').value);
    const customModeVal = parseInt(document.getElementById('entry-custom-mode').value);
    const container = document.getElementById('trigger-params-container');
    const typeDesc = document.getElementById('type-desc');
    const customGroup = document.getElementById('custom-mode-group');
    const customDesc = document.getElementById('custom-mode-desc');

    container.innerHTML = '';

    const typeMeta = metadata.TriggerTypes.find(t => t.Value === typeVal);
    if (!typeMeta) return;

    typeDesc.textContent = typeMeta.Description;

    // Show/Hide Custom Mode selector
    if (typeVal === 12) { // CustomTriggerValue
        customGroup.classList.remove('hidden');
        const customMeta = metadata.CustomTriggerModes.find(m => m.Value === customModeVal);
        customDesc.textContent = customMeta ? customMeta.Description : '';
    } else {
        customGroup.classList.add('hidden');
    }

    // Determine param labels
    let paramLabels = typeMeta.Params || ["Param 1", "Param 2", "Param 3", "Param 4"];

    // Get current values
    let currentParams = [0, 0, 0, 0];

    // Use dataset if available (from openEditModal)
    const form = document.getElementById('edit-form');
    if (form.dataset.params) {
        try {
            const stored = JSON.parse(form.dataset.params);
            if (currentEditIndex !== -1 && config[currentEditIndex].TriggerType === typeVal) {
                currentParams = stored;
            }
        } catch (e) { }
    }

    if (typeMeta.Params) {
        typeMeta.Params.forEach((label, i) => {
            const div = document.createElement('div');
            div.className = 'form-group half';
            div.innerHTML = `
                <label>${label}:</label>
                <input type="number" class="trigger-param" data-index="${i}" value="${currentParams[i] || 0}">
            `;
            container.appendChild(div);
        });
    } else {
        container.innerHTML = '<div class="form-group"><small>No parameters for this trigger type.</small></div>';
    }
}

function applyChanges() {
    const name = document.getElementById('entry-name').value;
    if (!name) {
        alert("Name is required");
        return;
    }

    // Collect values
    const entry = {
        Name: name,
        Description: document.getElementById('entry-description').value,
        CustomFormID: document.getElementById('entry-formid').value,
        Category: document.getElementById('entry-category').value,
        ControllerIndex: parseInt(document.getElementById('entry-controller').value),
        TriggerSide: parseInt(document.getElementById('entry-side').value),
        TriggerType: parseInt(document.getElementById('entry-type').value),
        customTriggerMode: parseInt(document.getElementById('entry-custom-mode').value),
        TriggerParams: [0, 0, 0, 0],
        RGBUpdate: [
            parseInt(document.getElementById('rgb-r').value) || 0,
            parseInt(document.getElementById('rgb-g').value) || 0,
            parseInt(document.getElementById('rgb-b').value) || 0
        ],
        PlayerLED: [false, false, false, false, false], // Legacy, just default to false
        playerLEDNewRev: parseInt(document.getElementById('entry-player-led').value),
        MicLEDMode: parseInt(document.getElementById('entry-mic-led').value),
        TriggerThreshold: parseInt(document.getElementById('entry-threshold').value)
    };

    // Collect params
    document.querySelectorAll('.trigger-param').forEach(input => {
        const idx = parseInt(input.dataset.index);
        entry.TriggerParams[idx] = parseInt(input.value) || 0;
    });

    if (currentEditIndex === -1) {
        config.push(entry);
    } else {
        config[currentEditIndex] = entry;
    }

    closeModal();
    renderList();
}

function deleteEntry() {
    if (currentEditIndex === -1) return;
    if (!confirm("Are you sure you want to delete this entry?")) return;

    config.splice(currentEditIndex, 1);
    closeModal();
    renderList();
}

async function saveConfig() {
    const btn = document.getElementById('save-btn');
    const originalText = btn.textContent;
    btn.textContent = "Saving...";
    btn.disabled = true;

    try {
        const res = await fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });

        if (res.ok) {
            alert("Configuration saved successfully!");
        } else {
            alert("Failed to save configuration.");
        }
    } catch (e) {
        console.error("Save failed", e);
        alert("Error saving configuration.");
    } finally {
        btn.textContent = originalText;
        btn.disabled = false;
    }
}
