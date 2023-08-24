## Barometer
A reimagining of a classic barometer using a BMP280 pressure sensor


![main_pic](https://github.com/bigcrimping/barometer/assets/74270551/fa05b8da-2620-4fb5-a163-f1ca3e115128)


The displays are both Dowty aircraft magnetic indicators which are similar to split-flap displays classically used in train stations


![barometer](https://github.com/bigcrimping/barometer/assets/74270551/299aa23d-fa96-4816-90d1-5d9c2e2891e8)



The display can show high/low/normal pressure and up/down arrows + "auto". The relays are overkill (the power supply is 28v and the displays when active take ~30mA) but I had them to hand.

The classical logic with a barometer is roughly:

1) Low pressure means bad weather
2) High pressure means good weather
3) Constant pressure usually means good weather
4) Rising air pressure is good
5) Lowering air pressure is bad

## Project Code

The code is here: https://github.com/bigcrimping/barometer/tree/main/code/pressure_meter

Fairly basic setup using the driver "BMx280MI" from Gregor Christandl, the code measures the pressure every 10 minutes and adds it to a circular buffer and uses the average to determine if the pressure is rising or falling. 

## Wiring

The wiring is as below.

PRESSURE_NORMAL is on pin 2
PRESSURE_HIGH is on pin 3
ARROW_UP is on pin 4
ARROW_DOWN is on pin 5
I2C SDA is on D18 and SCL is D19



![wiring](https://github.com/bigcrimping/barometer/assets/74270551/e367457f-473b-470f-b57f-5b39c9b3aa10)



## Mechanical Files

Mech files are here: https://github.com/bigcrimping/barometer/tree/main/mechanical

![3d](https://github.com/bigcrimping/barometer/assets/74270551/1d0ca40c-e71b-43a7-95e7-1bad9fb00361)


All mounting holes are M3 brass inserts with M3 x 7mm 

Fixing required 

6 x M3 brass inserts

6 x M3x7mm cap head bolts



