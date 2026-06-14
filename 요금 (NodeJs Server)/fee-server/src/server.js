const express = require('express');
const path = require('path');
const crypto = require('crypto');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const { calculateParkingFee } = require('./feeCalculator');

const app = express();
app.use(express.json());
const port = Number(process.env.PORT || 3000);
const gateSerialPath = process.env.GATE_SERIAL_PORT || 'COM3';
const slotSerialPath = process.env.SLOT_SERIAL_PORT || 'COM5';
const lcdSerialPath = process.env.LCD_SERIAL_PORT || 'COM7';
const serialBaudRate = Number(process.env.SERIAL_BAUD_RATE || 9600);
const slotCount = 2;
const adminUiDir = path.resolve(__dirname, '..', '..', '..', '관리자 페이지 (NodeJs Server)');
const adminSessionCookieName = 'admin_session';
const adminAccounts = new Map([
  ['admin1', { password: '1234', displayName: 'Admin 1' }],
  ['admin2', { password: '1234', displayName: 'Admin 2' }],
]);

const parkingSessions = new Map();
const slotOccupied = new Map();
const pendingExitSlots = [];
const adminSessions = new Map();
const adminStreamClients = new Set();

/**
 * 요청에서 주차칸 번호를 읽는다.
 * @param {import('express').Request} req - Express 요청 객체
 * @returns {string} 주차칸 번호
 */
function getSlotId(req) {
  return String(req.query.slot || '1');
}

/**
 * 현재 비어 있는 주차칸 수를 계산한다.
 * @returns {number} 빈 주차칸 수
 */
function countEmptySlots() {
  let occupiedCount = 0;

  for (let slotId = 1; slotId <= slotCount; slotId++) {
    if (slotOccupied.get(String(slotId))) {
      occupiedCount += 1;
    }
  }

  return slotCount - occupiedCount;
}

/**
 * 차량 입차 정보를 저장한다.
 * @param {string} slotId - 주차칸 번호
 * @returns {{slotId: string, enteredAt: string}} 저장된 입차 정보
 */
function registerVehicleEntry(slotId) {
  const enteredAt = new Date();

  slotOccupied.set(slotId, true);
  parkingSessions.set(slotId, {
    enteredAt,
  });

  return {
    slotId,
    enteredAt: enteredAt.toISOString(),
  };
}

/**
 * 차량이 주차칸을 비웠음을 기록한다.
 * @param {string} slotId - 주차칸 번호
 * @returns {void} 반환값 없음
 */
function markVehicleVacated(slotId) {
  slotOccupied.set(slotId, false);

  if (parkingSessions.has(slotId) && !pendingExitSlots.includes(slotId)) {
    pendingExitSlots.push(slotId);
  }
}

/**
 * 차량 출차 요금을 계산하고 기록을 정리한다.
 * @param {string} slotId - 주차칸 번호
 * @returns {{slotId: string, enteredAt: string, exitedAt: string, parkedSeconds: number, parkedMinutes: number, fee: number} | null} 계산 결과
 */
function checkoutVehicle(slotId) {
  const session = parkingSessions.get(slotId);

  if (!session) {
    return null;
  }

  const exitedAt = new Date();
  const parkedSeconds = Math.max(1, Math.ceil((exitedAt - session.enteredAt) / 1000));
  const parkedMinutes = Math.max(1, Math.ceil(parkedSeconds / 60));
  const fee = calculateParkingFee(parkedMinutes);

  parkingSessions.delete(slotId);
  slotOccupied.set(slotId, false);

  return {
    slotId,
    enteredAt: session.enteredAt.toISOString(),
    exitedAt: exitedAt.toISOString(),
    parkedSeconds,
    parkedMinutes,
    fee,
  };
}

/**
 * LCD에 보낼 시각을 HH:MM:SS 형식으로 만든다.
 * @param {Date} date - 표시할 시각
 * @returns {string} LCD 표시용 시각
 */
function formatLcdTime(date) {
  const hours = String(date.getHours()).padStart(2, '0');
  const minutes = String(date.getMinutes()).padStart(2, '0');
  const seconds = String(date.getSeconds()).padStart(2, '0');

  return `${hours}:${minutes}:${seconds}`;
}

/**
 * LCD에 보낼 날짜를 YYYY-MM-DD 형식으로 만든다.
 * @param {Date} date - 표시할 날짜
 * @returns {string} LCD 표시용 날짜
 */
function formatLcdDate(date) {
  const year = String(date.getFullYear());
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');

  return `${year}-${month}-${day}`;
}

/**
 * 현재 주차 중인 차량 목록을 반환한다.
 * @returns {Array<{slotId: string, enteredAt: string}>} 주차 세션 목록
 */
function getActiveSessions() {
  return Array.from(parkingSessions.entries()).map(([slotId, session]) => ({
    slotId,
    enteredAt: session.enteredAt.toISOString(),
  }));
}

/**
 * 주차 시간을 사람이 읽기 쉬운 문자열로 바꾼다.
 * @param {number} seconds - 경과 초
 * @returns {string} 표시 문자열
 */
function formatDurationLabel(seconds) {
  const safeSeconds = Math.max(0, Math.floor(seconds));
  const hours = Math.floor(safeSeconds / 3600);
  const minutes = Math.floor((safeSeconds % 3600) / 60);
  const restSeconds = safeSeconds % 60;

  return `${hours}h ${minutes}m ${restSeconds}s`;
}

/**
 * 쿠키 문자열을 객체로 바꾼다.
 * @param {string | undefined} cookieHeader - 요청 쿠키 헤더
 * @returns {Record<string, string>} 쿠키 맵
 */
function parseCookies(cookieHeader) {
  const cookies = {};

  if (!cookieHeader) {
    return cookies;
  }

  for (const chunk of cookieHeader.split(';')) {
    const index = chunk.indexOf('=');
    if (index < 0) {
      continue;
    }

    const key = chunk.slice(0, index).trim();
    const value = chunk.slice(index + 1).trim();
    if (key) {
      cookies[key] = decodeURIComponent(value);
    }
  }

  return cookies;
}

/**
 * 관리자 세션을 확인한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @returns {{username: string, displayName: string} | null} 인증된 사용자
 */
function getAdminUser(req) {
  const cookies = parseCookies(req.headers.cookie);
  const token = cookies[adminSessionCookieName];

  if (!token) {
    return null;
  }

  return adminSessions.get(token) || null;
}

/**
 * 관리자 인증이 필요한 요청을 검사한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @param {import('express').NextFunction} next - 다음 미들웨어
 * @returns {void} 반환값 없음
 */
function requireAdminAuth(req, res, next) {
  const user = getAdminUser(req);

  if (!user) {
    res.status(401).json({
      ok: false,
      message: '관리자 로그인이 필요합니다.',
    });
    return;
  }

  req.adminUser = user;
  next();
}

/**
 * 관리자 세션 토큰을 만든다.
 * @param {string} username - 계정 아이디
 * @returns {string} 세션 토큰
 */
function createAdminSession(username) {
  const account = adminAccounts.get(username);
  const token = crypto.randomUUID();

  adminSessions.set(token, {
    username,
    displayName: account ? account.displayName : username,
  });

  return token;
}

/**
 * 관리자 세션 쿠키를 설정한다.
 * @param {import('express').Response} res - Express 응답 객체
 * @param {string} token - 세션 토큰
 * @returns {void} 반환값 없음
 */
function setAdminSessionCookie(res, token) {
  res.setHeader('Set-Cookie', [
    `${adminSessionCookieName}=${encodeURIComponent(token)}; HttpOnly; Path=/; SameSite=Lax`,
  ]);
}

/**
 * 관리자 세션 쿠키를 삭제한다.
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function clearAdminSessionCookie(res) {
  res.setHeader('Set-Cookie', [
    `${adminSessionCookieName}=; HttpOnly; Path=/; SameSite=Lax; Max-Age=0`,
  ]);
}

/**
 * 관리자 페이지에 보낼 실시간 상태를 만든다.
 * @returns {{now: string, totalSlots: number, occupiedSlots: number, emptySlots: number, slots: Array<object>, pendingExitSlots: string[]}} 상태 객체
 */
function buildAdminState() {
  const now = new Date();
  const slots = [];

  for (let index = 1; index <= slotCount; index++) {
    const slotId = String(index);
    const session = parkingSessions.get(slotId) || null;
    const occupied = Boolean(slotOccupied.get(slotId));
    const pendingExit = pendingExitSlots.includes(slotId);
    const enteredAt = session ? session.enteredAt : null;
    const parkedSeconds = enteredAt ? Math.max(1, Math.ceil((now - enteredAt) / 1000)) : 0;
    const parkedMinutes = enteredAt ? Math.max(1, Math.ceil(parkedSeconds / 60)) : 0;
    const feePreview = enteredAt ? calculateParkingFee(parkedMinutes) : 0;

    slots.push({
      slotId,
      occupied,
      pendingExit,
      status: occupied ? '주차중' : pendingExit ? '출차대기' : enteredAt ? '출차대기' : '비어있음',
      enteredAt: enteredAt ? enteredAt.toISOString() : null,
      parkedSeconds,
      parkedMinutes,
      parkedLabel: enteredAt ? formatDurationLabel(parkedSeconds) : '0h 0m 0s',
      feePreview,
    });
  }

  return {
    now: now.toISOString(),
    totalSlots: slotCount,
    occupiedSlots: slotCount - countEmptySlots(),
    emptySlots: countEmptySlots(),
    slots,
    pendingExitSlots: [...pendingExitSlots],
  };
}

/**
 * 관리자 SSE 클라이언트에게 상태를 전송한다.
 * @returns {void} 반환값 없음
 */
function broadcastAdminState() {
  const payload = JSON.stringify(buildAdminState());

  for (const client of [...adminStreamClients]) {
    try {
      client.res.write(`data: ${payload}\n\n`);
    } catch (error) {
      adminStreamClients.delete(client);
    }
  }
}

/**
 * 관리자 SSE 연결을 추가한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleAdminStream(req, res) {
  if (!getAdminUser(req)) {
    res.status(401).json({
      ok: false,
      message: '관리자 로그인이 필요합니다.',
    });
    return;
  }

  res.setHeader('Content-Type', 'text/event-stream; charset=utf-8');
  res.setHeader('Cache-Control', 'no-cache, no-transform');
  res.setHeader('Connection', 'keep-alive');
  res.flushHeaders?.();

  const client = { res };
  adminStreamClients.add(client);
  res.write(`data: ${JSON.stringify(buildAdminState())}\n\n`);

  req.on('close', () => {
    adminStreamClients.delete(client);
  });
}

/**
 * 시리얼 포트를 열고 줄 단위로 읽는 연결을 만든다.
 * @param {string} path - 시리얼 포트 경로
 * @param {(line: string) => void} onLine - 수신 라인 처리 함수
 * @returns {{port: SerialPort, parser: ReadlineParser, path: string}} 연결 객체
 */
function createSerialConnection(path, onLine) {
  const serialPort = new SerialPort({
    path,
    baudRate: serialBaudRate,
    autoOpen: false,
  });
  const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

  serialPort.on('open', () => {
    console.log(`[serial] opened ${path} at ${serialBaudRate}`);
  });

  serialPort.on('error', (error) => {
    console.error(`[serial] ${path} error: ${error.message}`);
  });

  parser.on('data', (line) => {
    onLine(String(line).trim());
  });

  serialPort.open((error) => {
    if (error) {
      console.error(`[serial] ${path} open failed: ${error.message}`);
    }
  });

  return {
    port: serialPort,
    parser,
    path,
  };
}

let gateConnection = null;
let slotConnection = null;
let lcdConnection = null;

/**
 * Uno 1로 문자열을 보낸다.
 * @param {string} line - 전송할 문자열
 * @returns {boolean} 전송 성공 여부
 */
function sendGateLine(line) {
  if (!gateConnection || !gateConnection.port.isOpen) {
    return false;
  }

  gateConnection.port.write(`${line}\n`);
  return true;
}

/**
 * Uno 3 출구 LCD로 문자열을 보낸다.
 * @param {string} line - 전송할 문자열
 * @returns {boolean} 전송 성공 여부
 */
function sendLcdLine(line) {
  if (!lcdConnection || !lcdConnection.port.isOpen) {
    return false;
  }

  lcdConnection.port.write(`${line}\n`);
  return true;
}

/**
 * Uno 1에 빈자리 수를 전달한다.
 * @returns {void} 반환값 없음
 */
function notifyEmptySlots() {
  const emptySlots = countEmptySlots();
  sendGateLine(`EMPTY,${emptySlots}`);
  console.log(`[gate] empty slots ${emptySlots}`);
}

/**
 * Uno 3 출구 LCD에 현재 주차 현황을 전달한다.
 * @returns {void} 반환값 없음
 */
function notifyDisplayStatus() {
  const occupiedSlots = slotCount - countEmptySlots();
  const now = new Date();
  sendLcdLine(`LCD_STATUS,${occupiedSlots},${slotCount},${formatLcdDate(now)},${formatLcdTime(now)}`);
}

/**
 * 시리얼 이벤트에서 주차칸 번호를 꺼낸다.
 * @param {string[]} parts - 콤마로 분리한 이벤트 조각
 * @returns {string | null} 주차칸 번호
 */
function parseSlotEvent(parts) {
  const slotId = String(parts[1] || '').trim();

  if (!slotId) {
    return null;
  }

  return slotId;
}

/**
 * Uno 2의 주차칸 이벤트를 처리한다.
 * @param {string} line - 수신한 한 줄
 * @returns {void} 반환값 없음
 */
function handleSlotLine(line) {
  if (!line || line.startsWith('Slot ') || line.startsWith('Send event:')) {
    return;
  }

  const parts = line.split(',');
  const eventName = parts[0];
  const slotId = parseSlotEvent(parts);

  if (!slotId) {
    return;
  }

  if (eventName === 'ENTRY') {
    registerVehicleEntry(slotId);
    notifyEmptySlots();
    notifyDisplayStatus();
    broadcastAdminState();
    console.log(`[slot] entry slot ${slotId}`);
    return;
  }

  if (eventName === 'VACATED') {
    markVehicleVacated(slotId);
    notifyEmptySlots();
    notifyDisplayStatus();
    broadcastAdminState();
    console.log(`[slot] vacated slot ${slotId}`);
  }
}

/**
 * 대기 중인 출차를 확정하고 요금을 계산한다.
 * @returns {{slotId: string, fee: number} | null} 출차 결과
 */
function finalizePendingExit() {
  const slotId = pendingExitSlots.shift();

  if (!slotId) {
    return null;
  }

  const receipt = checkoutVehicle(slotId);

  if (!receipt) {
    return null;
  }

  notifyEmptySlots();
  notifyDisplayStatus();
  broadcastAdminState();
  sendLcdLine(
    `LCD_EXIT_FEE,${receipt.slotId},${receipt.fee},${receipt.parkedSeconds},${formatLcdTime(new Date(receipt.exitedAt))}`
  );

  return {
    slotId: receipt.slotId,
    fee: receipt.fee,
  };
}

/**
 * Uno 1의 차단기 이벤트를 처리한다.
 * @param {string} line - 수신한 한 줄
 * @returns {void} 반환값 없음
 */
function handleGateLine(line) {
  if (!line) {
    return;
  }

  if (line.startsWith('ENTRANCE_DETECTED')) {
    console.log(`[gate] entrance detected: ${line}`);
    if (countEmptySlots() === 0) {
      const occupiedSlots = slotCount - countEmptySlots();
      sendLcdLine(`LCD_FULL_WARNING,4000,${occupiedSlots},${slotCount}`);
      console.log('[lcd] parking full warning 4000ms');
    }
    return;
  }

  if (line.startsWith('Entrance Light:') || line.startsWith('Exit Light:')) {
    console.log(`[gate] sensor ${line}`);
    return;
  }

  if (line !== 'BARRIER_EXIT') {
    return;
  }

  const result = finalizePendingExit();

  if (result) {
    console.log(`[gate] exit slot ${result.slotId}, fee ${result.fee}`);
    return;
  }

  sendLcdLine(`LCD_EXIT_FEE,0,0,0,${formatLcdTime(new Date())}`);
  console.log('[gate] exit detected without pending slot');
}

/**
 * 입차 요청을 처리한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleVehicleEntry(req, res) {
  const slotId = getSlotId(req);
  const entry = registerVehicleEntry(slotId);

  notifyEmptySlots();
  notifyDisplayStatus();
  broadcastAdminState();

  res.json({
    ok: true,
    message: '차량 입차가 기록되었습니다.',
    entry,
  });
}

/**
 * 출차 요청을 처리하고 요금을 계산한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleVehicleExit(req, res) {
  const slotId = getSlotId(req);
  const receipt = checkoutVehicle(slotId);

  if (!receipt) {
    res.status(404).json({
      ok: false,
      message: '입차 기록이 없습니다.',
    });
    return;
  }

  notifyEmptySlots();
  notifyDisplayStatus();
  broadcastAdminState();

  res.json({
    ok: true,
    receipt,
  });
}

/**
 * 현재 세션 목록을 반환한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleActiveSessions(req, res) {
  res.json({
    ok: true,
    sessions: getActiveSessions(),
    occupied: Object.fromEntries(slotOccupied),
    pendingExitSlots,
    emptySlots: countEmptySlots(),
  });
}

/**
 * 시리얼 연결 상태를 반환한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleSerialStatus(req, res) {
  res.json({
    ok: true,
    gate: {
      path: gateSerialPath,
      open: Boolean(gateConnection && gateConnection.port.isOpen),
    },
    slots: {
      path: slotSerialPath,
      open: Boolean(slotConnection && slotConnection.port.isOpen),
    },
    lcd: {
      path: lcdSerialPath,
      open: Boolean(lcdConnection && lcdConnection.port.isOpen),
    },
    baudRate: serialBaudRate,
  });
}

/**
 * 관리자 로그인 요청을 처리한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleAdminLogin(req, res) {
  const username = String(req.body?.username || '').trim();
  const password = String(req.body?.password || '').trim();
  const account = adminAccounts.get(username);

  if (!account || account.password !== password) {
    res.status(401).json({
      ok: false,
      message: '아이디 또는 비밀번호가 올바르지 않습니다.',
    });
    return;
  }

  const token = createAdminSession(username);
  setAdminSessionCookie(res, token);

  res.json({
    ok: true,
    user: {
      username,
      displayName: account.displayName,
    },
  });
}

/**
 * 관리자 로그아웃 요청을 처리한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleAdminLogout(req, res) {
  const cookies = parseCookies(req.headers.cookie);
  const token = cookies[adminSessionCookieName];

  if (token) {
    adminSessions.delete(token);
  }

  clearAdminSessionCookie(res);
  res.json({ ok: true });
}

/**
 * 현재 로그인한 관리자 정보를 반환한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleAdminMe(req, res) {
  const user = getAdminUser(req);

  if (!user) {
    res.status(401).json({
      ok: false,
      message: '로그인이 필요합니다.',
    });
    return;
  }

  res.json({
    ok: true,
    user,
  });
}

/**
 * 관리자 페이지용 현재 상태를 반환한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleAdminState(req, res) {
  if (!getAdminUser(req)) {
    res.status(401).json({
      ok: false,
      message: '로그인이 필요합니다.',
    });
    return;
  }

  res.json({
    ok: true,
    state: buildAdminState(),
  });
}

/**
 * 서버 시작 로그를 출력한다.
 * @returns {void} 반환값 없음
 */
function handleServerStarted() {
  console.log(`Parking fee server listening on http://localhost:${port}`);
  console.log(`Gate serial: ${gateSerialPath}`);
  console.log(`Slot serial: ${slotSerialPath}`);
  console.log(`LCD serial: ${lcdSerialPath}`);
  console.log(`Admin page: http://localhost:${port}/admin`);
}

gateConnection = createSerialConnection(gateSerialPath, handleGateLine);
slotConnection = createSerialConnection(slotSerialPath, handleSlotLine);
lcdConnection = createSerialConnection(lcdSerialPath, () => {});

gateConnection.port.on('open', notifyEmptySlots);
lcdConnection.port.on('open', notifyDisplayStatus);

setTimeout(() => {
  notifyEmptySlots();
  notifyDisplayStatus();
}, 1500);

setInterval(() => {
  notifyDisplayStatus();
  broadcastAdminState();
}, 1000);

app.use('/admin', express.static(adminUiDir));
app.get('/admin', (req, res) => {
  res.sendFile(path.join(adminUiDir, 'index.html'));
});
app.get('/', (req, res) => {
  res.redirect('/admin');
});
app.post('/api/admin/login', handleAdminLogin);
app.post('/api/admin/logout', handleAdminLogout);
app.get('/api/admin/me', handleAdminMe);
app.get('/api/admin/state', handleAdminState);
app.get('/api/admin/stream', handleAdminStream);
app.get('/parking/entry', handleVehicleEntry);
app.get('/parking/exit', handleVehicleExit);
app.get('/parking/sessions', handleActiveSessions);
app.get('/serial/status', handleSerialStatus);

app.listen(port, handleServerStarted);
