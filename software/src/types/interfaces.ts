//----------------------------------
// Enums
//----------------------------------
export enum LeverFunctionMode {
  INTERPOLATED,
  PEAK_AND_DECAY,
  INCREMENTAL,
}

export enum LeverPushFunctionMode {
  INTERPOLATED,
  PEAK_AND_DECAY,
  STATIC,
  RESET,
}

export enum TouchFunctionMode {
  HOLD,
  TOGGLE,
  CONTINUOUS,
}

export enum InterpolationType {
  LINEAR,
  EXPONENTIAL,
  LOGARITHMIC
}

export enum ValueMode {
  UNIPOLAR,
  BIPOLAR
}

export enum ScaleType {
  CHROMATIC,
  MAJOR,
  MINOR,
  HARMONIC_MINOR,
  MELODIC_MINOR_ASC,
  PENTATONIC_MAJOR,
  PENTATONIC_MINOR,
  BLUES,
  DORIAN,
  PHRYGIAN,
  LYDIAN,
  MIXOLYDIAN,
  LOCRIAN
}

export enum SettingsType {
  LEVER,
  LEVERPUSH,
  TOUCH,
  SCALE,
}

//----------------------------------
// Interfaces
//----------------------------------
export interface Settings {
  lever1?: LeverSettings,
  lever2?: LeverSettings,
  leverPush1?: LeverPushSettings,
  leverPush2?: LeverPushSettings,
  touch?: TouchSettings,
  scale?: ScaleSettings,
}

export interface LeverSettings {
  ccNumber: number;
  minCCValue: number;
  maxCCValue: number;
  stepSize: number;
  functionMode: LeverFunctionMode;
  valueMode: ValueMode;
  onsetTime: number;
  offsetTime: number;
  onsetType: InterpolationType;
  offsetType: InterpolationType;
}

export interface LeverPushSettings {
  ccNumber: number;
  minCCValue: number;
  maxCCValue: number;
  functionMode: LeverPushFunctionMode;
  onsetTime: number;
  offsetTime: number;
  onsetType: InterpolationType;
  offsetType: InterpolationType;
}

export interface TouchSettings {
  ccNumber: number;
  minCCValue: number;
  maxCCValue: number;
  functionMode: TouchFunctionMode;
}

export interface ScaleSettings {
  scaleType: ScaleType;
  rootNote: number;
}
