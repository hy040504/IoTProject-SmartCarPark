const express = require('express');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const { calculateParkingFee } = require('./feeCalculator');

const app = express();
const port = Number(process.env.PORT || 3000);
const gateSerialPath = process.env.GATE_SERIAL_PORT || 'COM3';
const slotSerialPath = process.env.SLOT_SERIAL_PORT || 'COM5';
const serialBaudRate = Number(process.env.SERIAL_BAUD_RATE || 9600);
const slotCount = 2;

const parkingSessions = new Map();
const slotOccupied = new Map();
const pendingExitSlots = [];

/**
 * 요청에서 주차 슬롯 ID를 가져온다.
 * @param {import('express').Request} req - Express 요청 객체
 * @returns {string} 주차 슬롯 ID
 */
function getSlotId(req) {
  return String(req.query.slot || '1');
}

/**
 * 빈 주차칸 수를 계산한다.
 * @returns {number} 현재 비어 있는 주차칸 수
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
 * 차량 입차 시간을 저장한다.
 * @param {string} slotId - 주차 슬롯 ID
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
 * 차량 출차 대기 상태를 저장한다.
 * @param {string} slotId - 주차 슬롯 ID
 * @returns {void} 반환값 없음
 */
function markVehicleVacated(slotId) {
  slotOccupied.set(slotId, false);

  if (parkingSessions.has(slotId) && !pendingExitSlots.includes(slotId)) {
    pendingExitSlots.push(slotId);
  }
}

/**
 * 차량 출차 요금을 계산하고 주차 기록을 정리한다.
 * @param {string} slotId - 주차 슬롯 ID
 * @returns {{slotId: string, enteredAt: string, exitedAt: string, parkedMinutes: number, fee: number} | null} 계산된 요금 정보
 */
function checkoutVehicle(slotId) {
  const session = parkingSessions.get(slotId);

  if (!session) {
    return null;
  }

  const exitedAt = new Date();
  const parkedMinutes = Math.max(1, Math.ceil((exitedAt - session.enteredAt) / 60000));
  const fee = calculateParkingFee(parkedMinutes);

  parkingSessions.delete(slotId);
  slotOccupied.set(slotId, false);

  return {
    slotId,
    enteredAt: session.enteredAt.toISOString(),
    exitedAt: exitedAt.toISOString(),
    parkedMinutes,
    fee,
  };
}

/**
 * 현재 주차 중인 차량 목록을 반환한다.
 * @returns {Array<{slotId: string, enteredAt: string}>} 주차 중인 차량 목록
 */
function getActiveSessions() {
  return Array.from(parkingSessions.entries()).map(([slotId, session]) => ({
    slotId,
    enteredAt: session.enteredAt.toISOString(),
  }));
}

/**
 * Serial 포트를 열고 한 줄 단위 파서를 연결한다.
 * @param {string} path - 열 Serial 포트 이름
 * @param {(line: string) => void} onLine - 수신 라인 처리 함수
 * @returns {{port: SerialPort, parser: ReadlineParser, path: string}} Serial 연결 정보
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

/**
 * Uno 1 차단기 보드에 한 줄 명령을 보낸다.
 * @param {string} line - 전송할 명령 문자열
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
 * Uno 1에 최신 빈자리 수를 전달한다.
 * @returns {void} 반환값 없음
 */
function notifyEmptySlots() {
  sendGateLine(`EMPTY,${countEmptySlots()}`);
}

/**
 * Serial 이벤트에서 주차칸 번호를 추출한다.
 * @param {string[]} parts - 쉼표로 분리한 Serial 이벤트
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
 * Uno 2 주차칸 보드에서 받은 이벤트를 처리한다.
 * @param {string} line - Uno 2가 전송한 Serial 라인
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
    console.log(`[slot] entry slot ${slotId}`);
    return;
  }

  if (eventName === 'VACATED') {
    markVehicleVacated(slotId);
    notifyEmptySlots();
    console.log(`[slot] vacated slot ${slotId}`);
  }
}

/**
 * 출구 차단기 통과 이벤트를 출차 확정으로 처리한다.
 * @returns {{slotId: string, fee: number} | null} 출차 처리 결과
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

  sendGateLine(`FEE,${receipt.slotId},${receipt.fee}`);
  notifyEmptySlots();

  return {
    slotId: receipt.slotId,
    fee: receipt.fee,
  };
}

/**
 * Uno 1 차단기 보드에서 받은 이벤트를 처리한다.
 * @param {string} line - Uno 1이 전송한 Serial 라인
 * @returns {void} 반환값 없음
 */
function handleGateLine(line) {
  if (line !== 'BARRIER_EXIT') {
    return;
  }

  const result = finalizePendingExit();

  if (result) {
    console.log(`[gate] exit slot ${result.slotId}, fee ${result.fee}`);
    return;
  }

  sendGateLine('FEE,0,0');
  console.log('[gate] exit detected without pending slot');
}

/**
 * 입차 요청을 처리하고 주차 시작 시간을 저장한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleVehicleEntry(req, res) {
  const slotId = getSlotId(req);
  const entry = registerVehicleEntry(slotId);

  notifyEmptySlots();

  res.json({
    ok: true,
    message: '차량 입차가 등록되었습니다.',
    entry,
  });
}

/**
 * 출차 요청을 처리하고 주차 요금을 계산한다.
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

  res.json({
    ok: true,
    receipt,
  });
}

/**
 * 현재 주차 중인 차량 목록 요청을 처리한다.
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
 * Serial 연결 상태 요청을 처리한다.
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
    baudRate: serialBaudRate,
  });
}

/**
 * 서버 시작 상태를 콘솔에 출력한다.
 * @returns {void} 반환값 없음
 */
function handleServerStarted() {
  console.log(`Parking fee server listening on http://localhost:${port}`);
  console.log(`Gate serial: ${gateSerialPath}`);
  console.log(`Slot serial: ${slotSerialPath}`);
}

gateConnection = createSerialConnection(gateSerialPath, handleGateLine);
slotConnection = createSerialConnection(slotSerialPath, handleSlotLine);

app.get('/parking/entry', handleVehicleEntry);
app.get('/parking/exit', handleVehicleExit);
app.get('/parking/sessions', handleActiveSessions);
app.get('/serial/status', handleSerialStatus);

app.listen(port, handleServerStarted);
