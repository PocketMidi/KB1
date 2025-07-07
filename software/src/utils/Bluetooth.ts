/// <reference types="web-bluetooth" />

import { BTECharacteristics } from '@/objects/Constants.ts';
import {
  InterpolationType,
  LeverFunctionMode,
  LeverPushFunctionMode,
  LeverPushSettings,
  LeverSettings,
  ScaleSettings,
  ScaleType,
  Settings,
  SettingsType,
  TouchFunctionMode,
  TouchSettings,
  ValueMode,
} from '@/types/interfaces.ts';

let device: any;
let isConnected: boolean;
const characteristics = {};
const settings: Settings = {};

export default class Bluetooth {
  static get connected() {
    return isConnected;
  }

  static async connect(): Promise<boolean> {
    try {
      const navigator: any = window.navigator;
      if (navigator && navigator.bluetooth) {
        console.log('Requesting Bluetooth device...');
        device = await navigator?.bluetooth.requestDevice({
          filters: [{ services: [BTECharacteristics.Service] }],
          optionalServices: [BTECharacteristics.Service],
        });

        if (device) {
          console.log(`Connecting to GATT server on "${device.name}"...`);
          device.addEventListener('gattserverdisconnected', this.handleDisconnect);

          const server = await device.gatt.connect();
          console.log('Connected to GATT server.');

          if (server) {
            const service = await server.getPrimaryService(BTECharacteristics.Service);
            console.log('Getting characteristics...');
            const keys = Object.keys(BTECharacteristics);
            for (let i = 0, n = keys.length; i < n; i += 1) {
              const key = keys[i];
              if (BTECharacteristics[key] !== BTECharacteristics.Service) {
                characteristics[key] = await service.getCharacteristic(BTECharacteristics[key]);
              }
            }
            isConnected = true;
            return true;
          }
        }
      }
    } catch (err: any) {
      console.error(`Bluetooth connection error: ${err}`, 'error');
    }
    isConnected = false;
    return false;
  }

  static disconnect(): void {
    if (device && device?.gatt.connected) {
      console.log('Disconnecting from device...');
      device.removeEventListener('gattserverdisconnected', this.handleDisconnect);
      device.gatt.disconnect();
      isConnected = false;
    }
  }

  static async getSettings(): Promise<Settings> {
    const keys = Object.keys(BTECharacteristics);
    for (let i = 0, n = keys.length; i < n; i += 1) {
      const key = keys[i];
      if (BTECharacteristics[key] !== BTECharacteristics.Service) {
        const characteristic = characteristics[key];
        if (characteristic) {
          try {
            const rawData: DataView = await characteristic.readValue();
            switch (BTECharacteristics[key]) {
              case BTECharacteristics.Lever1: {
                settings.lever1 = this.parseDataView(rawData, SettingsType.LEVER) as LeverSettings;
                break;
              }
              case BTECharacteristics.Lever2: {
                settings.lever2 = this.parseDataView(rawData, SettingsType.LEVER) as LeverSettings;
                break;
              }
              case BTECharacteristics.LeverPush1: {
                settings.leverPush1 = this.parseDataView(rawData, SettingsType.LEVERPUSH) as LeverPushSettings;
                break;
              }
              case BTECharacteristics.LeverPush2: {
                settings.leverPush2 = this.parseDataView(rawData, SettingsType.LEVERPUSH) as LeverPushSettings;
                break;
              }
              case BTECharacteristics.Touch: {
                settings.touch = this.parseDataView(rawData, SettingsType.TOUCH) as TouchSettings;
                break;
              }
              case BTECharacteristics.Scale: {
                settings.scale = this.parseDataView(rawData, SettingsType.SCALE) as ScaleSettings;
                break;
              }
              default: {
                console.warn(`Unhandled characteristic: BTECharacteristics.${key}`);
              }
            }
          } catch (err: any) {
            console.error(`Error reading ${key} (${characteristic.uuid.substring(4, 8)}): ${err}`, 'error');
          }
        }
      }
    }
    return settings;
  }

  static async writeSettings(settings: Settings): Promise<boolean> {
    const keys = Object.keys(BTECharacteristics);
    for (let i = 0, n = keys.length; i < n; i += 1) {
      const key = keys[i];
      if (BTECharacteristics[key] !== BTECharacteristics.Service) {
        const characteristic = characteristics[key];
        if (characteristic) {
          try {
            let dataView: DataView | null = null;
            switch (BTECharacteristics[key]) {
              case BTECharacteristics.Lever1: {
                dataView = this.createDataView(settings.lever1 as LeverSettings, SettingsType.LEVER);
                break;
              }
              case BTECharacteristics.Lever2: {
                dataView = this.createDataView(settings.lever2 as LeverSettings, SettingsType.LEVER);
                break;
              }
              case BTECharacteristics.LeverPush1: {
                dataView = this.createDataView(settings.leverPush1 as LeverPushSettings, SettingsType.LEVERPUSH);
                break;
              }
              case BTECharacteristics.LeverPush2: {
                dataView = this.createDataView(settings.leverPush1 as LeverPushSettings, SettingsType.LEVERPUSH);
                break;
              }
              case BTECharacteristics.Touch: {
                dataView = this.createDataView(settings.touch as TouchSettings, SettingsType.TOUCH);
                break;
              }
              case BTECharacteristics.Scale: {
                dataView = this.createDataView(settings.scale as ScaleSettings, SettingsType.SCALE);
                break;
              }
              default: {
                console.warn(`Unhandled setting type for writing: ${key}`);
              }
            }
            if (dataView) {
              await characteristic.writeValue(dataView);
            }
          } catch (err: any) {
            console.error(`Error writing ${key} (${characteristic.uuid.substring(4, 8)}): ${err}`, 'error');
            return false;
          }
        }
      }
    }
    return true;
  }

  static parseDataView(
    data: DataView,
    settingsType: SettingsType,
  ): LeverSettings | LeverPushSettings | TouchSettings | ScaleSettings | null {
    switch (settingsType) {
      case SettingsType.LEVER:
        return {
          ccNumber: data.getInt32(0, true),
          minCCValue: data.getInt32(4, true),
          maxCCValue: data.getInt32(8, true),
          stepSize: data.getInt32(12, true),
          functionMode: data.getInt32(16, true) as LeverFunctionMode,
          valueMode: data.getInt32(20, true) as ValueMode,
          onsetTime: data.getUint32(24, true),
          offsetTime: data.getUint32(28, true),
          onsetType: data.getInt32(32, true) as InterpolationType,
          offsetType: data.getInt32(36, true) as InterpolationType,
        } as LeverSettings;
      case SettingsType.LEVERPUSH:
        return {
          ccNumber: data.getInt32(0, true),
          minCCValue: data.getInt32(4, true),
          maxCCValue: data.getInt32(8, true),
          functionMode: data.getInt32(12, true) as LeverPushFunctionMode,
          onsetTime: data.getUint32(16, true),
          offsetTime: data.getUint32(20, true),
          onsetType: data.getInt32(24, true) as InterpolationType,
          offsetType: data.getInt32(28, true) as InterpolationType,
        } as LeverPushSettings;
      case SettingsType.TOUCH:
        return {
          ccNumber: data.getInt32(0, true),
          minCCValue: data.getInt32(4, true),
          maxCCValue: data.getInt32(8, true),
          functionMode: data.getInt32(12, true) as TouchFunctionMode,
        } as TouchSettings;
      case SettingsType.SCALE:
        return {
          scaleType: data.getInt32(0, true) as ScaleType,
          rootNote: data.getInt32(4, true),
        } as ScaleSettings;
      default:
        console.warn('Unknown settings type for parsing raw data.');
        return null;
    }
  }

  static createDataView(
    settings: LeverSettings | LeverPushSettings | TouchSettings | ScaleSettings,
    settingsType: SettingsType,
  ): DataView | null {
    let buffer: ArrayBuffer;
    let dataView: DataView;

    switch (settingsType) {
      case SettingsType.LEVER:
        buffer = new ArrayBuffer(40);
        dataView = new DataView(buffer);
        const leverSettings = settings as LeverSettings;
        dataView.setInt32(0, leverSettings.ccNumber, true);
        dataView.setInt32(4, leverSettings.minCCValue, true);
        dataView.setInt32(8, leverSettings.maxCCValue, true);
        dataView.setInt32(12, leverSettings.stepSize, true);
        dataView.setInt32(16, leverSettings.functionMode, true);
        dataView.setInt32(20, leverSettings.valueMode, true);
        dataView.setUint32(24, leverSettings.onsetTime, true);
        dataView.setUint32(28, leverSettings.offsetTime, true);
        dataView.setInt32(32, leverSettings.onsetType, true);
        dataView.setInt32(36, leverSettings.offsetType, true);
        return dataView;
      case SettingsType.LEVERPUSH:
        buffer = new ArrayBuffer(32);
        dataView = new DataView(buffer);
        const leverPushSettings = settings as LeverPushSettings;
        dataView.setInt32(0, leverPushSettings.ccNumber, true);
        dataView.setInt32(4, leverPushSettings.minCCValue, true);
        dataView.setInt32(8, leverPushSettings.maxCCValue, true);
        dataView.setInt32(12, leverPushSettings.functionMode, true);
        dataView.setUint32(16, leverPushSettings.onsetTime, true);
        dataView.setUint32(20, leverPushSettings.offsetTime, true);
        dataView.setInt32(24, leverPushSettings.onsetType, true);
        dataView.setInt32(28, leverPushSettings.offsetType, true);
        return dataView;
      case SettingsType.TOUCH:
        buffer = new ArrayBuffer(16);
        dataView = new DataView(buffer);
        const touchSettings = settings as TouchSettings;
        dataView.setInt32(0, touchSettings.ccNumber, true);
        dataView.setInt32(4, touchSettings.minCCValue, true);
        dataView.setInt32(8, touchSettings.maxCCValue, true);
        dataView.setInt32(12, touchSettings.functionMode, true);
        return dataView;
      case SettingsType.SCALE:
        buffer = new ArrayBuffer(8);
        dataView = new DataView(buffer);
        const scaleSettings = settings as ScaleSettings;
        dataView.setInt32(0, scaleSettings.scaleType, true);
        dataView.setInt32(4, scaleSettings.rootNote, true);
        return dataView;
      default:
        console.warn('Unknown settings type for creating DataView.');
        return null;
    }
  }

  static handleDisconnect(evt: Event) {
    console.log(evt);
  }
}
