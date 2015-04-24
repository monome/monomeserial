# monomeserial

## PLEASE NOTE

monomeserial only works on monome grids made before 2012.

monomeserial was deprecated by the new serialosc server in 2011. if you still use any old monomeserial applications that need updating for serialosc, post to the forum.

list of monomeserial only applications: http://github.com/monome-community/collect-ms

!()[monomeserial_screenshot.png]

monomeserial is a routing application which converts the monome's native serial protocol to OSC or midi in addition to providing support for reorientation and multiple-unit spanning.

designed and written by brian crabtree, joe lake, daniel battaglia, steve duda, and kevin nelson.

source released under gnu gpl.

## 1. requirements

* os x: version 10.4 or above, universal binary.
* windows: xp or vistamonome device detection/removal is handled automatically.

configuration is per-device. each device is listed in the drop-down according to serial number. (ie: m40h0020 or m256-020)

## 2. midi

in this mode, button presses are statically mapped to note values. velocity 127 is sent for on, 0 for off. led values are identically mapped: non-zero velocities turn on an led, zero turns them off.

for example, on the 40h:

  0  1  2  3  4  5  6  7
  8  9  10 11 12 13 14 15
  ...
  65 57 58 59 60 61 62 63

the 256 maps notes 0-127 for channel 1 for the upper half and channel 2 for the lower half.

### 2a. cable orientation 

cable orientation should be set according to the physical orientation of the device. monomeserial will transform the matrix accordingly.

### 2b. ports and channels 

each device has an assignable midi input and output port. "to/from monomeserial" ports are created virtually. input and output channels are also assignable.

note: host address, host port, and listen port are osc specific and are not used in midi mode.

## 3. osc

OSC is a udp network protocol which is fast and flexible.

### 3a. addressing and ports

these settings affect all devices attached:

* /host address/ - destination IP address. 127.0.0.1 sends data inter-application, which will be the most common configuration.
* /host port/ - destination port
* /listen port/ - receiving port to listen on

the remaining settings are per-device.

### 3b. cable orientation

cable orientation should be set according to the physical orientation of the device. monomeserial will transform the matrix accordingly.

### 3c. prefix 

the prefix is the base of the all patterns sent and received.

the default is /40h, /256, /128, or /64 respectively

a press value is sent as:

  /40h/press 0 0 1

this could be changed to /mlr for example, then a press would be sent as:

  /mlr/press 0 0 1

the prefix also changes which incoming messages get matched. if the prefix is set to /mlr then

  /mlr/led 0 0 1

would change the led and

  /40h/led 0 0 1

would not.

this allows for context switching between multiple applications, or extended functionality within a single application.


### 3d. offsets

for using multiple units with the same prefix, offsetting allows values to be mapped across multiple devices.

for example, with two units side by side, the right unit can be set with a column offset of 8. this effectively creates a 16 x 8 grid.

to change an led on the right device:

  /40h/led 8 0 1

the right device also now sends out press messages with the y value shifted to the right by 8.


## known bugs

* mk devices have unexpected offsets when using cable orientations other than "left"
* autodetect is broken-- plugin in monome before launching monomeserial
* gs128 devices have their orientations rotated a bit. "left" is actually "top".
* frame is funky when offsets are used, but works ok at offset 0 0.  frame is broken for mk devices in windows.
* the windows installer still says version 0.2.1.5, but installed version says 0.3.0.0.  this bug doesn't have a parallel in os x, as there is no installer.