const loginView = document.getElementById('loginView');
const dashboardView = document.getElementById('dashboardView');
const loginForm = document.getElementById('loginForm');
const loginError = document.getElementById('loginError');
const connectionStatus = document.getElementById('connectionStatus');
const themeToggleButton = document.getElementById('themeToggleButton');
const logoutButton = document.getElementById('logoutButton');
const occupiedCount = document.getElementById('occupiedCount');
const emptyCount = document.getElementById('emptyCount');
const totalCount = document.getElementById('totalCount');
const lastUpdated = document.getElementById('lastUpdated');
const slotList = document.getElementById('slotList');
const slotLiveBadge = document.getElementById('slotLiveBadge');
const currentUser = document.getElementById('currentUser');

let stream = null;
const THEME_STORAGE_KEY = 'smart-car-park-theme';
const BASE_FEE = 1000;
const BASE_MINUTES = 30;
const EXTRA_FEE = 500;
const EXTRA_UNIT_MINUTES = 10;

function formatDateTime(isoString) {
  if (!isoString) {
    return '--';
  }

  const date = new Date(isoString);
  const dateText = new Intl.DateTimeFormat('ko-KR', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
  }).format(date);
  const timeText = new Intl.DateTimeFormat('ko-KR', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  }).format(date);

  return `${dateText} ${timeText}`;
}

function setStatus(text, state) {
  connectionStatus.textContent = text;
  connectionStatus.className = `status-pill ${state}`;
  connectionStatus.dataset.label = text;
  slotLiveBadge.textContent = state === 'ok' ? 'LIVE CONNECTED' : 'RECONNECTING';
  slotLiveBadge.className = `slot-live-badge ${state}`;
}

function applyTheme(theme) {
  const isDark = theme === 'dark';
  document.body.classList.toggle('dark-theme', isDark);
  themeToggleButton.textContent = isDark ? 'Light' : 'Dark';
}

function toggleTheme() {
  const nextTheme = document.body.classList.contains('dark-theme') ? 'light' : 'dark';
  localStorage.setItem(THEME_STORAGE_KEY, nextTheme);
  applyTheme(nextTheme);
}

function normalizeSlotStatus(status) {
  if (status === '빈칸') {
    return '비어있음';
  }

  return status;
}

function formatTimeOnly(isoString) {
  if (!isoString) {
    return '-';
  }

  const date = new Date(isoString);
  return new Intl.DateTimeFormat('ko-KR', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false,
  }).format(date);
}

function formatDurationClock(totalSeconds) {
  if (!totalSeconds || totalSeconds < 1) {
    return '-';
  }

  const safeSeconds = Math.max(0, Math.floor(totalSeconds));
  const hours = String(Math.floor(safeSeconds / 3600)).padStart(2, '0');
  const minutes = String(Math.floor((safeSeconds % 3600) / 60)).padStart(2, '0');
  const seconds = String(safeSeconds % 60).padStart(2, '0');
  return `${hours}:${minutes}:${seconds}`;
}

function buildSlotStateBadge(status) {
  if (status === '주차중') {
    return '<span class="slot-status-badge busy">OCCUPIED</span>';
  }

  if (status === '출차대기') {
    return '<span class="slot-status-badge waiting">EXIT WAIT</span>';
  }

  return '<span class="slot-status-badge empty">EMPTY</span>';
}

function calculateAnimatedFee(slot) {
  if (!slot.enteredAt || !slot.parkedSeconds) {
    return null;
  }

  const baseSeconds = BASE_MINUTES * 60;
  const extraUnitSeconds = EXTRA_UNIT_MINUTES * 60;
  const parkedSeconds = Math.max(1, Math.floor(slot.parkedSeconds));

  if (parkedSeconds <= baseSeconds) {
    return Math.min(
      BASE_FEE,
      Math.max(1, Math.round((parkedSeconds / baseSeconds) * BASE_FEE))
    );
  }

  const extraSeconds = parkedSeconds - baseSeconds;
  const completedExtraUnits = Math.floor(extraSeconds / extraUnitSeconds);
  const progressSeconds = extraSeconds % extraUnitSeconds;

  return (
    BASE_FEE +
    completedExtraUnits * EXTRA_FEE +
    Math.round((progressSeconds / extraUnitSeconds) * EXTRA_FEE)
  );
}

function renderSlots(state) {
  slotList.innerHTML = '';

  state.slots.forEach((slot) => {
    const item = document.createElement('div');
    const statusText = normalizeSlotStatus(slot.status);
    const animatedFee = calculateAnimatedFee(slot);
    const feeText = animatedFee === null ? '-' : `${animatedFee.toLocaleString('ko-KR')}원`;
    item.className = 'slot-table-row';

    item.innerHTML = `
      <span class="slot-id-cell">주차칸 ${slot.slotId}</span>
      <span class="slot-status-cell" title="${statusText}">${buildSlotStateBadge(statusText)}</span>
      <span>${formatTimeOnly(slot.enteredAt)}</span>
      <span>${formatDurationClock(slot.parkedSeconds)}</span>
      <span class="slot-fee-cell">${feeText}</span>
    `;

    slotList.appendChild(item);
  });
}

function renderState(state) {
  occupiedCount.textContent = String(state.occupiedSlots);
  emptyCount.textContent = String(state.emptySlots);
  totalCount.textContent = String(state.totalSlots);
  lastUpdated.textContent = formatDateTime(state.now);
  renderSlots(state);
}

async function loadState() {
  const response = await fetch('/api/admin/state', {
    credentials: 'include',
  });

  if (!response.ok) {
    throw new Error('state load failed');
  }

  const payload = await response.json();
  renderState(payload.state);
}

async function loadMe() {
  const response = await fetch('/api/admin/me', {
    credentials: 'include',
  });

  if (!response.ok) {
    return null;
  }

  const payload = await response.json();
  return payload.user;
}

function closeStream() {
  if (stream) {
    stream.close();
    stream = null;
  }
}

function connectStream() {
  closeStream();
  stream = new EventSource('/api/admin/stream', { withCredentials: true });

  stream.onopen = () => {
    setStatus('Online', 'ok');
  };

  stream.onmessage = (event) => {
    renderState(JSON.parse(event.data));
  };

  stream.onerror = () => {
    setStatus('재연결 대기', 'warn');
  };
}

function showDashboard(user) {
  loginView.classList.add('hidden');
  dashboardView.classList.remove('hidden');
  currentUser.textContent = `${user.displayName} (${user.username})`;
  setStatus('연결 대기', 'warn');
}

function showLogin() {
  dashboardView.classList.add('hidden');
  loginView.classList.remove('hidden');
  currentUser.textContent = 'Guest';
}

loginForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  loginError.textContent = '';

  const formData = new FormData(loginForm);
  const username = String(formData.get('username') || '').trim();
  const password = String(formData.get('password') || '').trim();

  const response = await fetch('/api/admin/login', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    credentials: 'include',
    body: JSON.stringify({ username, password }),
  });

  if (!response.ok) {
    loginError.textContent = '아이디 또는 비밀번호가 올바르지 않습니다.';
    return;
  }

  const payload = await response.json();
  showDashboard(payload.user);
  await loadState();
  connectStream();
});

logoutButton.addEventListener('click', async () => {
  await fetch('/api/admin/logout', {
    method: 'POST',
    credentials: 'include',
  });

  closeStream();
  showLogin();
});

themeToggleButton.addEventListener('click', toggleTheme);

window.addEventListener('beforeunload', closeStream);

(async () => {
  applyTheme(localStorage.getItem(THEME_STORAGE_KEY) || 'light');

  try {
    const user = await loadMe();
    if (!user) {
      showLogin();
      return;
    }

    showDashboard(user);
    await loadState();
    connectStream();
  } catch (error) {
    showLogin();
    loginError.textContent = '서버 연결을 확인할 수 없습니다.';
  }
})();
