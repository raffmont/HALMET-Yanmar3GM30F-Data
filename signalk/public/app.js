'use strict'

const pluginId = 'signalk-halmet-yanmar3gm30f-data'
const statusUrl = `/plugins/${pluginId}/status`
const vibrationUrl = `/plugins/${pluginId}/vibration`

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

function drawSpectrum(vibration) {
  const canvas = document.getElementById('spectrum')
  if (!canvas) {
    return
  }
  const ctx = canvas.getContext('2d')
  const width = canvas.width
  const height = canvas.height
  ctx.clearRect(0, 0, width, height)
  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, width, height)
  ctx.strokeStyle = '#d9e2ec'
  ctx.lineWidth = 1
  for (let i = 0; i <= 4; i += 1) {
    const y = 24 + (height - 48) * (i / 4)
    ctx.beginPath()
    ctx.moveTo(48, y)
    ctx.lineTo(width - 18, y)
    ctx.stroke()
  }

  const frequencies = vibration.frequenciesHz || []
  const magnitudes = vibration.magnitudesMps2 || []
  const points = frequencies
    .map((frequency, index) => ({ frequency, magnitude: magnitudes[index] }))
    .filter(
      (point) =>
        typeof point.frequency === 'number' &&
        Number.isFinite(point.frequency) &&
        typeof point.magnitude === 'number' &&
        Number.isFinite(point.magnitude)
    )
  const maxFrequency = vibration.maxFrequencyHz || Math.max(...points.map((p) => p.frequency), 1)
  const maxMagnitude = Math.max(...points.map((p) => p.magnitude), 0)

  ctx.fillStyle = '#52606d'
  ctx.font = '14px system-ui, sans-serif'
  ctx.fillText('Frequency (Hz)', width - 132, height - 10)
  ctx.save()
  ctx.translate(16, 170)
  ctx.rotate(-Math.PI / 2)
  ctx.fillText('m/s2', 0, 0)
  ctx.restore()

  if (!vibration.available || points.length === 0 || maxMagnitude <= 0) {
    ctx.fillStyle = '#829ab1'
    ctx.font = '18px system-ui, sans-serif'
    ctx.fillText('No vibration spectrum available', 56, 58)
    return
  }

  const plotLeft = 48
  const plotRight = width - 18
  const plotTop = 18
  const plotBottom = height - 36
  ctx.strokeStyle = '#1f6f8b'
  ctx.lineWidth = 2
  ctx.beginPath()
  points.forEach((point, index) => {
    const x = plotLeft + (point.frequency / maxFrequency) * (plotRight - plotLeft)
    const y = plotBottom - (point.magnitude / maxMagnitude) * (plotBottom - plotTop)
    if (index === 0) {
      ctx.moveTo(x, y)
    } else {
      ctx.lineTo(x, y)
    }
  })
  ctx.stroke()
}

async function refresh() {
  try {
    const [statusResponse, vibrationResponse] = await Promise.all([
      fetch(statusUrl, { cache: 'no-store' }),
      fetch(vibrationUrl, { cache: 'no-store' })
    ])
    const data = await statusResponse.json()
    const vibrationData = await vibrationResponse.json()
    const v = data.values || {}
    const d = data.display || {}
    const vibration = vibrationData.vibration || {}

    setText('rpm', fmt(d.rpm, ' rpm', 0))
    setText('state', v.state || '--')
    setText('fuel', fmt(d.fuelPercent, '%', 0))
    setText(
      'remaining',
      fmt(typeof v.fuelRemaining === 'number' ? v.fuelRemaining * 1000 : null, ' L', 1)
    )
    setText('coolant', fmt(d.coolantCelsius, ' °C', 1))
    setText('oil', fmt(d.oilPressureBar, ' bar', 2))
    setText('alt', fmt(v.alternatorVoltage, ' V', 1))
    setText('engine-room', fmt(d.engineRoomCelsius, ' °C', 1))
    setText('vibration-status', vibration.status || (vibration.available ? 'available' : 'unavailable'))
    setText('vibration-rms', fmt(vibration.accelerationRmsMps2, ' m/s2', 2))
    setText('vibration-peak', fmt(vibration.peakAccelerationMps2, ' m/s2', 2))
    setText('vibration-frequency', fmt(vibration.peakFrequencyHz, ' Hz', 1))
    setText('vibration-crank', fmt(vibration.crankOrderAmplitudeMps2, ' m/s2', 2))
    setText('vibration-firing', fmt(vibration.firingOrderAmplitudeMps2, ' m/s2', 2))
    drawSpectrum(vibration)
    setText('diagnostics', JSON.stringify(data.diagnostics, null, 2))
  } catch (error) {
    setText('diagnostics', `Unable to read ${statusUrl}\n${error.message}`)
  }
}

refresh()
setInterval(refresh, 1000)
