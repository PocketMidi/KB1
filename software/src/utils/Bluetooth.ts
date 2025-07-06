/// <reference types="web-bluetooth" />

import { BTECharacteristics } from '@/objects/Constants.ts';
import {
  InterpolationType,
  LeverFunctionMode,
  LeverPushFunctionMode,
  LeverPushSettings,
  LeverSettings,
  ScaleSettings,
  ScaleType, Settings,
  SettingsType,
  TouchFunctionMode,
  TouchSettings,
  ValueMode,
} from '@/types/interfaces.ts';

let device: any;
const characteristics = {};
const settings:Settings = {};

export default class Bluetooth {

  static async connect(): Promise<boolean> {
    try {
      const navigator: any = window.navigator;
      if(navigator && navigator.bluetooth) {
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
            return true;
          }
        }
      }
    } catch (err: any) {
      console.error(`Bluetooth connection error: ${err}`, 'error');
    }
    return false;
  }

  static disconnect(): void {
    if (device && device.gatt.connected) {
      console.log('Disconnecting from device...');
      device.removeEventListener('gattserverdisconnected', this.handleDisconnect);
      device.gatt.disconnect();
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
                settings.lever1 = this.parseRawData(rawData, SettingsType.LEVER) as LeverSettings;
                break;
              }
              case BTECharacteristics.Lever2: {
                settings.lever2 = this.parseRawData(rawData, SettingsType.LEVER) as LeverSettings;
                break;
              }
              case BTECharacteristics.LeverPush1: {
                settings.leverPush1 = this.parseRawData(rawData, SettingsType.LEVERPUSH) as LeverPushSettings;
                break;
              }
              case BTECharacteristics.LeverPush2: {
                settings.leverPush2 = this.parseRawData(rawData, SettingsType.LEVERPUSH) as LeverPushSettings;
                break;
              }
              case BTECharacteristics.Touch: {
                settings.touch = this.parseRawData(rawData, SettingsType.TOUCH) as TouchSettings;
                break;
              }
              case BTECharacteristics.Scale: {
                settings.scale = this.parseRawData(rawData, SettingsType.SCALE) as ScaleSettings;
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

  static parseRawData(data: DataView, settingsType: SettingsType): LeverSettings | LeverPushSettings | TouchSettings | ScaleSettings | null {
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

  static handleDisconnect(evt: Event) {
    console.log(evt);
  }
}

