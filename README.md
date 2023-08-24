## Barometer
A reimagining of a classic barometer using a BMP280 pressure sensor

![main_pic](https://github.com/bigcrimping/barometer/assets/74270551/15842e96-7193-433c-9364-c8636e680af8)


The displays are both Dowty aircraft magnetic indicators which are similar to split-flap displays classically used in train stations

![barometer](https://github.com/bigcrimping/barometer/assets/74270551/f5106513-76e7-4a44-95ba-246bfbe6c3fe)

The display can show high/low/normal pressure and up/down arrows + "auto". The relays are overkill (the power supply is 28v and the displays when active take ~30mA) but I had them to hand.

The classical logic with a barometer is roughly:

1) Low pressure means bad weather
2) High pressure means good weather
3) Constant pressure usually means good weather
4) Rising air pressure is good
5) Lowering air pressure is bad

## Project Code

The code is here: (fill in once published)

Fairly basic setup using the driver "BMx280MI" from Gregor Christandl, the code measures the pressure every 10 minutes and adds it to a circular buffer and uses the average to determine if the pressure is rising or falling. 

## Wiring

The wiring is as below.

PRESSURE_NORMAL is on pin 2
PRESSURE_HIGH is on pin 3
ARROW_UP is on pin 4
ARROW_DOWN is on pin 5
I2C SDA is on D18 and SCL is D19

![image](https://github.com/bigcrimping/barometer/assets/74270551/70e9fb8e-a6ce-4eb6-9eeb-516b281252bf)


## Mechanical Files

Mech files are here: (fill in once published)

![3d](https://github.com/bigcrimping/barometer/assets/74270551/1addd655-9c70-4926-8692-edb90da42f17)

All mounting holes are M3 brass inserts with M3 x 7mm 

Fixing required 

6 x M3 brass inserts

6 x M3x7mm cap head bolts



