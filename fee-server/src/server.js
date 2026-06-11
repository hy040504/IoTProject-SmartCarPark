const express = require('express');
const { calculateParkingFee } = require('./feeCalculator');

const app = express();
const port = Number(process.env.PORT || 3000);
const parkingSessions = new Map();

/**
 * 요청에서 주차 슬롯 ID를 가져온다.
 * @param {import('express').Request} req - Express 요청 객체
 * @returns {string} 주차 슬롯 ID
 */
function getSlotId(req) {
  return String(req.query.slot || '1');
}

/**
 * 차량 입차 시간을 저장한다.
 * @param {string} slotId - 주차 슬롯 ID
 * @returns {{slotId: string, enteredAt: string}} 저장된 입차 정보
 */
function registerVehicleEntry(slotId) {
  const enteredAt = new Date();

  parkingSessions.set(slotId, {
    enteredAt,
  });

  return {
    slotId,
    enteredAt: enteredAt.toISOString(),
  };
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
 * 입차 요청을 처리하고 주차 시작 시간을 저장한다.
 * @param {import('express').Request} req - Express 요청 객체
 * @param {import('express').Response} res - Express 응답 객체
 * @returns {void} 반환값 없음
 */
function handleVehicleEntry(req, res) {
  const slotId = getSlotId(req);
  const entry = registerVehicleEntry(slotId);

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
  });
}

/**
 * 서버 시작 상태를 콘솔에 출력한다.
 * @returns {void} 반환값 없음
 */
function handleServerStarted() {
  console.log(`Parking fee server listening on http://localhost:${port}`);
}

app.get('/parking/entry', handleVehicleEntry);
app.get('/parking/exit', handleVehicleExit);
app.get('/parking/sessions', handleActiveSessions);

app.listen(port, handleServerStarted);
