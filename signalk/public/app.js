'use strict'

const pluginId = 'signalk-halmet-yanmar3gm30f-data'
const statusUrl = `/plugins/${pluginId}/status`

function fmt(value, suffix, digits = 1) {
  return typeof value === 'number' && Number.isFinite(value)
    ? `${value.toFixed(digits)}${suffix}`
    : '--'
}

function setText(id, value) {
  const element = document.getElementById(id)
  if (element) {
    element.textContent = value
  }
}

async function refresh() {
  try {
    const response = await fetch(statusUrl, { cache: 'no-store' })
    const data = await response.json()
    const v = data.values || {}
    const d = data.display || {}

    setText('rpm', fmt(d.rpm, ' rpm', 0))
    setText('state', v.state || '--')
    setText('fuel', fmt(d.fuelPercent, '%', 0))
    setText('remaining', fmt(v.fuelRemaining ? v.fuelRemaining * 1000 : null, ' L', 1))
    setText('coolant', fmt(d.coolantCelsius, ' °C', 1))
    setText('oil', fmt(d.oilPressureBar, ' bar', 2))
    setText('alt', fmt(v.alternatorVoltage, ' V', 1))
    setText('engine-room', fmt(d.engineRoomCelsius, ' °C', 1))
    setText('diagnostics', JSON.stringify(data.diagnostics, null, 2))
  } catch (error) {
    setText('diagnostics', `Unable to read ${statusUrl}\n${error.message}`)
  }
}

refresh()
setInterval(refresh, 1000)
