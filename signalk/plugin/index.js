'use strict'

const PLUGIN_ID = 'signalk-halmet-yanmar3gm30f-data'

const DEFAULTS = {
  engineId: 'main',
  fuelTankId: 'main',
  fuelCapacityLiters: 30,
  staleAfterSeconds: 10,
  enableDerivedTankRemaining: true,
  enableDerivedDiagnostics: true,
  vibrationSensor: '',
  vibrationMountLocation: '',
  vibrationAxis: 'z',
  vibrationMaxFrequencyHz: 250
}

const PATHS = {
  revolutions: (engineId) => `propulsion.${engineId}.revolutions`,
  state: (engineId) => `propulsion.${engineId}.state`,
  coolantTemperature: (engineId) => `propulsion.${engineId}.coolantTemperature`,
  oilPressure: (engineId) => `propulsion.${engineId}.oilPressure`,
  alternatorVoltage: (engineId) => `propulsion.${engineId}.alternatorVoltage`,
  fuelLevel: (tankId) => `tanks.fuel.${tankId}.currentLevel`,
  fuelRemaining: (tankId) => `tanks.fuel.${tankId}.remaining`,
  engineRoomTemperature: 'environment.inside.engineRoom.temperature',
  exhaustTemperature: (engineId) => `propulsion.${engineId}.exhaustTemperature`,
  transmissionOilTemperature: (engineId) =>
    `propulsion.${engineId}.transmission.oilTemperature`,
  alternatorTemperature: 'electrical.alternators.main.temperature',
  vibrationAccelerationRms: 'halmet.yanmar3gm30f.vibration.accelerationRms',
  vibrationPeakAcceleration: 'halmet.yanmar3gm30f.vibration.peakAcceleration',
  vibrationPeakFrequency: 'halmet.yanmar3gm30f.vibration.peakFrequency',
  vibrationCrankOrderAmplitude: 'halmet.yanmar3gm30f.vibration.crankOrderAmplitude',
  vibrationFiringOrderAmplitude: 'halmet.yanmar3gm30f.vibration.firingOrderAmplitude',
  vibrationSampleRate: 'halmet.yanmar3gm30f.vibration.sampleRate',
  vibrationFftSize: 'halmet.yanmar3gm30f.vibration.fftSize',
  vibrationStatus: 'halmet.yanmar3gm30f.vibration.status',
  vibrationSpectrum: 'halmet.yanmar3gm30f.vibration.spectrum'
}

function nowIso() {
  return new Date().toISOString()
}

function finiteNumber(value) {
  return typeof value === 'number' && Number.isFinite(value)
}

function getSelfPath(app, path) {
  try {
    if (typeof app.getSelfPath === 'function') {
      return app.getSelfPath(path)
    }
  } catch (e) {
    return undefined
  }
  return undefined
}

function getValue(app, path) {
  const node = getSelfPath(app, path)
  if (node && Object.prototype.hasOwnProperty.call(node, 'value')) {
    return node.value
  }
  return undefined
}

function parseObject(value) {
  if (value && typeof value === 'object' && !Array.isArray(value)) {
    return value
  }
  if (typeof value === 'string' && value.trim() !== '') {
    try {
      const parsed = JSON.parse(value)
      return parsed && typeof parsed === 'object' && !Array.isArray(parsed)
        ? parsed
        : null
    } catch (e) {
      return null
    }
  }
  return null
}

function finiteNumbers(values) {
  return Array.isArray(values) ? values.filter(finiteNumber) : []
}

function makeDelta(values) {
  return {
    updates: [
      {
        source: { label: PLUGIN_ID },
        timestamp: nowIso(),
        values
      }
    ]
  }
}

function sendValues(app, values) {
  if (values.length === 0) {
    return
  }
  if (typeof app.handleMessage === 'function') {
    app.handleMessage(PLUGIN_ID, makeDelta(values))
  }
}

function readStatus(app, settings) {
  const engineId = settings.engineId || DEFAULTS.engineId
  const tankId = settings.fuelTankId || DEFAULTS.fuelTankId
  const paths = {
    revolutions: PATHS.revolutions(engineId),
    state: PATHS.state(engineId),
    coolantTemperature: PATHS.coolantTemperature(engineId),
    oilPressure: PATHS.oilPressure(engineId),
    alternatorVoltage: PATHS.alternatorVoltage(engineId),
    fuelLevel: PATHS.fuelLevel(tankId),
    fuelRemaining: PATHS.fuelRemaining(tankId),
    engineRoomTemperature: PATHS.engineRoomTemperature,
    exhaustTemperature: PATHS.exhaustTemperature(engineId),
    transmissionOilTemperature: PATHS.transmissionOilTemperature(engineId),
    alternatorTemperature: PATHS.alternatorTemperature
  }

  const values = {}
  for (const [key, path] of Object.entries(paths)) {
    values[key] = getValue(app, path)
  }

  const rpm = finiteNumber(values.revolutions) ? values.revolutions * 60 : null
  const fuelPercent = finiteNumber(values.fuelLevel)
    ? values.fuelLevel * 100
    : null

  return {
    pluginId: PLUGIN_ID,
    timestamp: nowIso(),
    paths,
    values,
    display: {
      rpm,
      fuelPercent,
      coolantCelsius: finiteNumber(values.coolantTemperature)
        ? values.coolantTemperature - 273.15
        : null,
      engineRoomCelsius: finiteNumber(values.engineRoomTemperature)
        ? values.engineRoomTemperature - 273.15
        : null,
      oilPressureBar: finiteNumber(values.oilPressure)
        ? values.oilPressure / 100000
        : null
    }
  }
}

function readVibration(app, settings) {
  const engineId = settings.engineId || DEFAULTS.engineId
  const revolutions = getValue(app, PATHS.revolutions(engineId))
  const rpm = finiteNumber(revolutions) ? revolutions * 60 : null
  const sampleRate = getValue(app, PATHS.vibrationSampleRate)
  const fftSize = getValue(app, PATHS.vibrationFftSize)
  const spectrum = parseObject(getValue(app, PATHS.vibrationSpectrum))
  const spectrumSampleRate = spectrum ? spectrum.sampleRateHz : null
  const spectrumFftSize = spectrum ? spectrum.fftSize : null
  const effectiveSampleRate = finiteNumber(sampleRate) ? sampleRate : spectrumSampleRate
  const effectiveFftSize = finiteNumber(fftSize) ? fftSize : spectrumFftSize
  const binWidth =
    finiteNumber(effectiveSampleRate) && finiteNumber(effectiveFftSize) && effectiveFftSize > 0
      ? effectiveSampleRate / effectiveFftSize
      : spectrum && finiteNumber(spectrum.binWidthHz)
        ? spectrum.binWidthHz
        : null
  const magnitudes = spectrum ? finiteNumbers(spectrum.magnitudesMps2) : []
  const spectrumFrequencies = spectrum ? finiteNumbers(spectrum.frequenciesHz) : []
  const frequencies =
    spectrumFrequencies.length > 0
      ? spectrumFrequencies
      : binWidth
        ? magnitudes.map((_, index) => (index + 1) * binWidth)
        : []
  const maxFrequency =
    spectrum && finiteNumber(spectrum.maxFrequencyHz)
      ? spectrum.maxFrequencyHz
      : finiteNumber(settings.vibrationMaxFrequencyHz)
        ? settings.vibrationMaxFrequencyHz
        : DEFAULTS.vibrationMaxFrequencyHz

  const metrics = {
    accelerationRmsMps2: getValue(app, PATHS.vibrationAccelerationRms),
    peakAccelerationMps2: getValue(app, PATHS.vibrationPeakAcceleration),
    peakFrequencyHz: getValue(app, PATHS.vibrationPeakFrequency),
    crankOrderAmplitudeMps2: getValue(app, PATHS.vibrationCrankOrderAmplitude),
    firingOrderAmplitudeMps2: getValue(app, PATHS.vibrationFiringOrderAmplitude),
    status: getValue(app, PATHS.vibrationStatus)
  }

  return {
    pluginId: PLUGIN_ID,
    timestamp: nowIso(),
    engineId,
    paths: {
      accelerationRms: PATHS.vibrationAccelerationRms,
      peakAcceleration: PATHS.vibrationPeakAcceleration,
      peakFrequency: PATHS.vibrationPeakFrequency,
      crankOrderAmplitude: PATHS.vibrationCrankOrderAmplitude,
      firingOrderAmplitude: PATHS.vibrationFiringOrderAmplitude,
      sampleRate: PATHS.vibrationSampleRate,
      fftSize: PATHS.vibrationFftSize,
      status: PATHS.vibrationStatus,
      spectrum: PATHS.vibrationSpectrum
    },
    vibration: {
      available:
        Object.values(metrics).some((value) => value !== undefined) || magnitudes.length > 0,
      sensor: settings.vibrationSensor || null,
      mountLocation: settings.vibrationMountLocation || null,
      axis: (spectrum && spectrum.axis) || settings.vibrationAxis || DEFAULTS.vibrationAxis,
      sampleRateHz: finiteNumber(effectiveSampleRate) ? effectiveSampleRate : null,
      fftSize: finiteNumber(effectiveFftSize) ? effectiveFftSize : null,
      binWidthHz: finiteNumber(binWidth) ? binWidth : null,
      maxFrequencyHz: maxFrequency,
      rpm,
      crankFrequencyHz: finiteNumber(rpm) ? rpm / 60 : null,
      firingFrequencyHz: finiteNumber(rpm) ? 1.5 * rpm / 60 : null,
      ...metrics,
      frequenciesHz: frequencies,
      magnitudesMps2: magnitudes,
      topPeaks: Array.isArray(spectrum && spectrum.topPeaks) ? spectrum.topPeaks : []
    }
  }
}

module.exports = function pluginFactory(app) {
  let interval = null
  let lastDerived = {}

  const plugin = {
    id: PLUGIN_ID,
    name: 'HALMET Yanmar 3GM30F Data',
    description:
      'Web GUI, derived tank data, and diagnostics for HALMET Yanmar 3GM30F Signal K streams.',

    schema: () => ({
      type: 'object',
      required: [],
      properties: {
        engineId: {
          type: 'string',
          title: 'Engine id',
          default: DEFAULTS.engineId,
          description: 'Signal K propulsion id, for example main in propulsion.main.*'
        },
        fuelTankId: {
          type: 'string',
          title: 'Fuel tank id',
          default: DEFAULTS.fuelTankId,
          description: 'Signal K fuel tank id, for example main in tanks.fuel.main.*'
        },
        fuelCapacityLiters: {
          type: 'number',
          title: 'Fuel capacity, liters',
          default: DEFAULTS.fuelCapacityLiters,
          minimum: 0,
          description:
            'Used to derive tanks.fuel.<id>.remaining from currentLevel. Set 0 to disable remaining calculation.'
        },
        staleAfterSeconds: {
          type: 'number',
          title: 'Diagnostic stale threshold, seconds',
          default: DEFAULTS.staleAfterSeconds,
          minimum: 1
        },
        enableDerivedTankRemaining: {
          type: 'boolean',
          title: 'Publish derived fuel remaining',
          default: DEFAULTS.enableDerivedTankRemaining
        },
        enableDerivedDiagnostics: {
          type: 'boolean',
          title: 'Enable diagnostics API',
          default: DEFAULTS.enableDerivedDiagnostics
        },
        vibrationSensor: {
          type: 'string',
          title: 'Vibration sensor label',
          default: DEFAULTS.vibrationSensor,
          description: 'Optional display label, for example ICM-42688-P or MPU-6050.'
        },
        vibrationMountLocation: {
          type: 'string',
          title: 'Vibration sensor mount location',
          default: DEFAULTS.vibrationMountLocation,
          description:
            'Optional display note, for example engine block side, middle cylinder area.'
        },
        vibrationAxis: {
          type: 'string',
          title: 'Primary vibration axis',
          default: DEFAULTS.vibrationAxis,
          enum: ['x', 'y', 'z', 'magnitude']
        },
        vibrationMaxFrequencyHz: {
          type: 'number',
          title: 'Vibration graph max frequency, Hz',
          default: DEFAULTS.vibrationMaxFrequencyHz,
          minimum: 1
        }
      }
    }),

    start: (configuration) => {
      const settings = { ...DEFAULTS, ...(configuration || {}) }
      const engineId = settings.engineId || DEFAULTS.engineId
      const tankId = settings.fuelTankId || DEFAULTS.fuelTankId

      const publishDerivedValues = () => {
        const values = []

        if (settings.enableDerivedTankRemaining) {
          const level = getValue(app, PATHS.fuelLevel(tankId))
          const capacityLiters = Number(settings.fuelCapacityLiters)
          if (finiteNumber(level) && capacityLiters > 0) {
            const remainingM3 = (Math.max(0, Math.min(1, level)) * capacityLiters) / 1000
            const rounded = Math.round(remainingM3 * 1000000) / 1000000
            if (lastDerived.fuelRemaining !== rounded) {
              lastDerived.fuelRemaining = rounded
              values.push({ path: PATHS.fuelRemaining(tankId), value: rounded })
            }
          }
        }

        const revolutions = getValue(app, PATHS.revolutions(engineId))
        const currentState = getValue(app, PATHS.state(engineId))
        if (finiteNumber(revolutions)) {
          const derivedState = revolutions > 1 / 60 ? 'started' : 'stopped'
          if (currentState !== derivedState && lastDerived.state !== derivedState) {
            lastDerived.state = derivedState
            values.push({ path: PATHS.state(engineId), value: derivedState })
          }
        }

        sendValues(app, values)
      }

      interval = setInterval(publishDerivedValues, 1000)

      plugin._settings = settings

      if (app.debug) {
        app.debug(`${PLUGIN_ID} started`)
      }
    },

    registerWithRouter: (router) => {
      const handler = (req, res) => {
        const settings = plugin._settings || DEFAULTS
        const status = readStatus(app, settings)
        res.json({
          ...status,
          derived: {
            fuelRemainingEnabled: Boolean(settings.enableDerivedTankRemaining),
            fuelCapacityLiters: Number(settings.fuelCapacityLiters),
            lastDerived
          },
          diagnostics: {
            dataAvailable: Object.values(status.values).some((value) => value !== undefined),
            rpmAvailable: finiteNumber(status.values.revolutions),
            fuelAvailable: finiteNumber(status.values.fuelLevel),
            temperatureAvailable:
              finiteNumber(status.values.coolantTemperature) ||
              finiteNumber(status.values.engineRoomTemperature)
          }
        })
      }

      router.get('/status', handler)
      router.get('/diagnostics', handler)
      router.get('/vibration', (req, res) => {
        const settings = plugin._settings || DEFAULTS
        res.json(readVibration(app, settings))
      })
      router.get('/spectrum', (req, res) => {
        const settings = plugin._settings || DEFAULTS
        res.json(readVibration(app, settings))
      })
    },

    getOpenApi: () => ({
      openapi: '3.0.0',
      info: {
        title: 'HALMET Yanmar 3GM30F Data Plugin API',
        version: '0.1.0'
      },
      paths: {
        '/status': {
          get: {
            summary: 'Return live Signal K engine values and derived display values',
            responses: { '200': { description: 'Current plugin status' } }
          }
        },
        '/diagnostics': {
          get: {
            summary: 'Return diagnostic booleans for the configured data stream',
            responses: { '200': { description: 'Current plugin diagnostics' } }
          }
        },
        '/vibration': {
          get: {
            summary: 'Return latest HALMET vibration metrics and reduced FFT spectrum',
            responses: { '200': { description: 'Current vibration data' } }
          }
        },
        '/spectrum': {
          get: {
            summary: 'Alias for /vibration for spectrum graph clients',
            responses: { '200': { description: 'Current vibration spectrum data' } }
          }
        }
      }
    }),

    stop: () => {
      if (interval) {
        clearInterval(interval)
        interval = null
      }
      lastDerived = {}
      if (app.debug) {
        app.debug(`${PLUGIN_ID} stopped`)
      }
    }
  }

  return plugin
}
