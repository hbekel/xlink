EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:userport
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "xlink parport cable"
Date "24 sep 2014"
Rev "1"
Comp "Henning Bekel"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L DB25 J2
U 1 1 54229979
P 9000 4900
F 0 "J2" H 9050 6250 70  0000 C CNN
F 1 "PARPORT" H 8950 3550 70  0000 C CNN
F 2 "" H 9000 4900 60  0000 C CNN
F 3 "" H 9000 4900 60  0000 C CNN
	1    9000 4900
	1    0    0    -1  
$EndComp
$Comp
L USERPORT J1
U 1 1 542299C8
P 2050 4950
F 0 "J1" H 2050 5600 60  0000 C CNN
F 1 "USERPORT" H 2050 4200 60  0000 C CNN
F 2 "~" H 2100 5000 60  0000 C CNN
F 3 "~" H 2100 5000 60  0000 C CNN
	1    2050 4950
	1    0    0    -1  
$EndComp
Text Label 2750 4600 0    60   ~ 0
D0
Text Label 2750 4700 0    60   ~ 0
D1
Text Label 2750 4800 0    60   ~ 0
D2
Text Label 2750 4900 0    60   ~ 0
D3
Text Label 2750 5000 0    60   ~ 0
D4
Text Label 2750 5100 0    60   ~ 0
D5
Text Label 2750 5200 0    60   ~ 0
D6
Text Label 2750 5300 0    60   ~ 0
D7
Text Label 2750 5400 0    60   ~ 0
BUSY
Text Label 2750 4500 0    60   ~ 0
STROBE
NoConn ~ 1500 4700
NoConn ~ 1500 4800
NoConn ~ 1500 4900
NoConn ~ 1500 5000
NoConn ~ 1500 5100
NoConn ~ 1500 5200
NoConn ~ 1500 5300
NoConn ~ 1500 5400
$Comp
L GND #PWR01
U 1 1 54229ABD
P 1500 4400
F 0 "#PWR01" H 1500 4400 30  0001 C CNN
F 1 "GND" H 1500 4330 30  0001 C CNN
F 2 "" H 1500 4400 60  0000 C CNN
F 3 "" H 1500 4400 60  0000 C CNN
	1    1500 4400
	0    1    1    0   
$EndComp
$Comp
L GND #PWR02
U 1 1 54229ACA
P 1500 5500
F 0 "#PWR02" H 1500 5500 30  0001 C CNN
F 1 "GND" H 1500 5430 30  0001 C CNN
F 2 "" H 1500 5500 60  0000 C CNN
F 3 "" H 1500 5500 60  0000 C CNN
	1    1500 5500
	0    1    1    0   
$EndComp
$Comp
L GND #PWR03
U 1 1 54229AD0
P 2650 5500
F 0 "#PWR03" H 2650 5500 30  0001 C CNN
F 1 "GND" H 2650 5430 30  0001 C CNN
F 2 "" H 2650 5500 60  0000 C CNN
F 3 "" H 2650 5500 60  0000 C CNN
	1    2650 5500
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR04
U 1 1 54229AD6
P 2650 4400
F 0 "#PWR04" H 2650 4400 30  0001 C CNN
F 1 "GND" H 2650 4330 30  0001 C CNN
F 2 "" H 2650 4400 60  0000 C CNN
F 3 "" H 2650 4400 60  0000 C CNN
	1    2650 4400
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR05
U 1 1 54229ADC
P 8550 5200
F 0 "#PWR05" H 8550 5200 30  0001 C CNN
F 1 "GND" H 8550 5130 30  0001 C CNN
F 2 "" H 8550 5200 60  0000 C CNN
F 3 "" H 8550 5200 60  0000 C CNN
	1    8550 5200
	0    1    1    0   
$EndComp
$Comp
L GND #PWR06
U 1 1 54229AE2
P 8550 5000
F 0 "#PWR06" H 8550 5000 30  0001 C CNN
F 1 "GND" H 8550 4930 30  0001 C CNN
F 2 "" H 8550 5000 60  0000 C CNN
F 3 "" H 8550 5000 60  0000 C CNN
	1    8550 5000
	0    1    1    0   
$EndComp
$Comp
L GND #PWR07
U 1 1 54229AE8
P 8550 4800
F 0 "#PWR07" H 8550 4800 30  0001 C CNN
F 1 "GND" H 8550 4730 30  0001 C CNN
F 2 "" H 8550 4800 60  0000 C CNN
F 3 "" H 8550 4800 60  0000 C CNN
	1    8550 4800
	0    1    1    0   
$EndComp
$Comp
L GND #PWR08
U 1 1 54229AEE
P 8550 4600
F 0 "#PWR08" H 8550 4600 30  0001 C CNN
F 1 "GND" H 8550 4530 30  0001 C CNN
F 2 "" H 8550 4600 60  0000 C CNN
F 3 "" H 8550 4600 60  0000 C CNN
	1    8550 4600
	0    1    1    0   
$EndComp
$Comp
L GND #PWR09
U 1 1 54229AF4
P 8550 4400
F 0 "#PWR09" H 8550 4400 30  0001 C CNN
F 1 "GND" H 8550 4330 30  0001 C CNN
F 2 "" H 8550 4400 60  0000 C CNN
F 3 "" H 8550 4400 60  0000 C CNN
	1    8550 4400
	0    1    1    0   
$EndComp
$Comp
L GND #PWR010
U 1 1 54229AFA
P 8550 4200
F 0 "#PWR010" H 8550 4200 30  0001 C CNN
F 1 "GND" H 8550 4130 30  0001 C CNN
F 2 "" H 8550 4200 60  0000 C CNN
F 3 "" H 8550 4200 60  0000 C CNN
	1    8550 4200
	0    1    1    0   
$EndComp
$Comp
L GND #PWR011
U 1 1 54229B00
P 8550 4000
F 0 "#PWR011" H 8550 4000 30  0001 C CNN
F 1 "GND" H 8550 3930 30  0001 C CNN
F 2 "" H 8550 4000 60  0000 C CNN
F 3 "" H 8550 4000 60  0000 C CNN
	1    8550 4000
	0    1    1    0   
$EndComp
$Comp
L GND #PWR012
U 1 1 54229B06
P 8550 3800
F 0 "#PWR012" H 8550 3800 30  0001 C CNN
F 1 "GND" H 8550 3730 30  0001 C CNN
F 2 "" H 8550 3800 60  0000 C CNN
F 3 "" H 8550 3800 60  0000 C CNN
	1    8550 3800
	0    1    1    0   
$EndComp
Text Notes 700  700  0    60   ~ 0
This is a simple 8-bit parallel transfer cable connecting\nthe PC Parallel Port to the C64 User port.\n\n       --------------------------------\n       PC Parallel Port    |      C64 User Port\n       --------------------------------\n       GND    (18-25)  <->    GND    (1, 12, A, N)\n       D0     (2)        <->    PB0    (C) \n       D1     (3)        <->    PB1    (D) \n       D2     (4)        <->    PB2    (E) \n       D3     (5)        <->    PB3    (F) \n       D4     (6)        <->    PB4    (H) \n       D5     (7)        <->    PB5    (J) \n       D6     (8)        <->    PB6    (K) \n       D7     (9)        <->    PB7    (L) \n       BUSY   (11)       <--    PA2    (M) \n       STROBE (1)        -->    /FLAG2 (B) \n\nA shielded cable is recommended. The cable shield should be connected\nto shield ground on both sides. A 0.1uF capacitor between shield and\nground at the c64 side can also help to reduce noise. Improper\nshielding will most likely cause transfer errors.\n\nCAUTION: There is no protection of any kind. The parallel port pins\nare directly connected to port B of CIA2. Always power down both the\nPC and C64 before inserting/removing the cable. Also turn off the C64\nduring boot, reboot or shutdown of the PC.
Text Label 1400 4600 2    60   ~ 0
RESET
$Comp
L R R2
U 1 1 54229B85
P 5200 5100
F 0 "R2" V 5280 5100 40  0000 C CNN
F 1 "10k" V 5207 5101 40  0000 C CNN
F 2 "~" V 5130 5100 30  0000 C CNN
F 3 "~" H 5200 5100 30  0000 C CNN
	1    5200 5100
	0    -1   -1   0   
$EndComp
$Comp
L BC547 Q1
U 1 1 54229B9E
P 5650 5100
F 0 "Q1" H 5650 4951 40  0000 R CNN
F 1 "BC547" H 5650 5250 40  0000 R CNN
F 2 "TO92" H 5550 5202 29  0000 C CNN
F 3 "~" H 5650 5100 60  0000 C CNN
	1    5650 5100
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 54229BC5
P 5750 4650
F 0 "R3" V 5830 4650 40  0000 C CNN
F 1 "1k5" V 5757 4651 40  0000 C CNN
F 2 "~" V 5680 4650 30  0000 C CNN
F 3 "~" H 5750 4650 30  0000 C CNN
	1    5750 4650
	1    0    0    -1  
$EndComp
Text Label 1400 4500 2    60   ~ 0
VCC
$Comp
L R R1
U 1 1 54229C27
P 4950 4850
F 0 "R1" V 5030 4850 40  0000 C CNN
F 1 "10k" V 4957 4851 40  0000 C CNN
F 2 "~" V 4880 4850 30  0000 C CNN
F 3 "~" H 4950 4850 30  0000 C CNN
	1    4950 4850
	1    0    0    -1  
$EndComp
Wire Wire Line
	4950 5100 4700 5100
Text Label 4700 5100 2    60   ~ 0
INIT
Wire Wire Line
	4950 4600 4950 4350
Wire Wire Line
	5750 4350 5750 4400
Text Label 5750 4350 0    60   ~ 0
VCC
$Comp
L GND #PWR013
U 1 1 54229C60
P 5750 5450
F 0 "#PWR013" H 5750 5450 30  0001 C CNN
F 1 "GND" H 5750 5380 30  0001 C CNN
F 2 "" H 5750 5450 60  0000 C CNN
F 3 "" H 5750 5450 60  0000 C CNN
	1    5750 5450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5750 5300 5750 5450
$Comp
L BC547 Q2
U 1 1 54229C76
P 6350 4900
F 0 "Q2" H 6350 4751 40  0000 R CNN
F 1 "BC547" H 6350 5050 40  0000 R CNN
F 2 "TO92" H 6250 5002 29  0000 C CNN
F 3 "~" H 6350 4900 60  0000 C CNN
	1    6350 4900
	1    0    0    -1  
$EndComp
Wire Wire Line
	5750 4900 6150 4900
Wire Wire Line
	6450 4700 6450 4350
Text Label 6450 4350 0    60   ~ 0
RESET
Wire Wire Line
	6450 5100 6450 5450
$Comp
L GND #PWR014
U 1 1 54229CA2
P 6450 5450
F 0 "#PWR014" H 6450 5450 30  0001 C CNN
F 1 "GND" H 6450 5380 30  0001 C CNN
F 2 "" H 6450 5450 60  0000 C CNN
F 3 "" H 6450 5450 60  0000 C CNN
	1    6450 5450
	1    0    0    -1  
$EndComp
Text Label 4950 4350 0    60   ~ 0
VCC
Text Label 8450 5900 2    60   ~ 0
D0
Text Label 8450 5700 2    60   ~ 0
D1
Text Label 8450 5500 2    60   ~ 0
D2
Text Label 8450 5300 2    60   ~ 0
D3
Text Label 8450 5100 2    60   ~ 0
D4
Text Label 8450 4900 2    60   ~ 0
D5
Text Label 8450 4700 2    60   ~ 0
D6
Text Label 8450 4500 2    60   ~ 0
D7
Text Label 8450 6100 2    60   ~ 0
STROBE
Text Label 8450 4100 2    60   ~ 0
BUSY
Text Label 8450 5600 2    60   ~ 0
INIT
NoConn ~ 8550 3700
NoConn ~ 8550 3900
NoConn ~ 8550 4300
NoConn ~ 8550 5400
NoConn ~ 8550 5800
NoConn ~ 8550 6000
Wire Wire Line
	1400 4500 1500 4500
Wire Wire Line
	1400 4600 1500 4600
Wire Wire Line
	2650 4500 2750 4500
Wire Wire Line
	2650 4600 2750 4600
Wire Wire Line
	2650 4700 2750 4700
Wire Wire Line
	2650 4800 2750 4800
Wire Wire Line
	2650 4900 2750 4900
Wire Wire Line
	2650 5000 2750 5000
Wire Wire Line
	2650 5100 2750 5100
Wire Wire Line
	2650 5200 2750 5200
Wire Wire Line
	2650 5300 2750 5300
Wire Wire Line
	2650 5400 2750 5400
Wire Wire Line
	8450 4100 8550 4100
Wire Wire Line
	8450 4500 8550 4500
Wire Wire Line
	8450 4700 8550 4700
Wire Wire Line
	8450 4900 8550 4900
Wire Wire Line
	8450 5100 8550 5100
Wire Wire Line
	8450 5300 8550 5300
Wire Wire Line
	8450 5500 8550 5500
Wire Wire Line
	8450 5600 8550 5600
Wire Wire Line
	8450 5700 8550 5700
Wire Wire Line
	8450 5900 8550 5900
Wire Wire Line
	8450 6100 8550 6100
$Comp
L PWR_FLAG #FLG015
U 1 1 5422A1C2
P 10700 6950
F 0 "#FLG015" H 10700 7045 30  0001 C CNN
F 1 "PWR_FLAG" H 10700 7130 30  0000 C CNN
F 2 "" H 10700 6950 60  0000 C CNN
F 3 "" H 10700 6950 60  0000 C CNN
	1    10700 6950
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR016
U 1 1 5422A1D1
P 10700 6950
F 0 "#PWR016" H 10700 6950 30  0001 C CNN
F 1 "GND" H 10700 6880 30  0001 C CNN
F 2 "" H 10700 6950 60  0000 C CNN
F 3 "" H 10700 6950 60  0000 C CNN
	1    10700 6950
	1    0    0    -1  
$EndComp
$EndSCHEMATC
