# Ciel NES emulator

Ciel is a free and open-source NES emulator written in C++.
Its purpose is to emulate the Nintendo Entertainment System with higher-than-average accuracy.

# Current Version

The latest released version is 0.1.0.

Version 0.1.0 is lacking audio output.
The cycle-accurate 2A03 core supports all official instructions.
The PPU core is fully-functional, though nowhere near cycle-accurate.
The APU has yet to be emulated.
Ciel supports NROM and AxROM games in version 0.1.0.

# To-Dos

## 2A03 core:
* Add unofficial instructions
* Improve accuracy

## PPU core:
* Make PPU core cycle-accurate

## APU core:
* Add APU core

## Mappers:
* Add more mappers

## Misc:
* Add debugger
* Add UI

# How to run games with Ciel

To run games with Ceil, pass a ROM path as a command-line argument.

## Controls:
* X key => A
* Y key => B
* Arrow keys => UP/DOWN/LEFT/RIGHT
* Keypad Enter => Start
* Backspace => Select
