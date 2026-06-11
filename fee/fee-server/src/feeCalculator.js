const BASE_FEE = 1000;
const BASE_MINUTES = 30;
const EXTRA_FEE = 500;
const EXTRA_UNIT_MINUTES = 10;

/**
 * 주차 시간에 따라 요금을 계산한다.
 * @param {number} parkedMinutes - 주차한 시간
 * @returns {number} 계산된 주차 요금
 */
function calculateParkingFee(parkedMinutes) {
  if (parkedMinutes <= BASE_MINUTES) {
    return BASE_FEE;
  }

  const extraMinutes = parkedMinutes - BASE_MINUTES;
  const extraUnits = Math.ceil(extraMinutes / EXTRA_UNIT_MINUTES);

  return BASE_FEE + extraUnits * EXTRA_FEE;
}

module.exports = {
  calculateParkingFee,
};
