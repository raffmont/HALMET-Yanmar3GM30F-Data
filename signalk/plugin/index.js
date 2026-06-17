'use strict'

const PLUGIN_ID = 'signalk-halmet-yanmar3gm30f-data'

const DEFAULTS = {
  engineId: 'main',
  fuelTankId: 'main',
  fuelCapacityLiters: 30,
  staleAfterSeconds: 10,
  enableDerivedTankRemaining: true,
  enableDerivedDiagnostics: true
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
  alternatorTemperature: 'electrical.alternators.main.temperature'
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
