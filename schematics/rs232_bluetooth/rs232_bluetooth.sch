EESchema Schematic File Version 2
LIBS:power,device,conn,linear,regul,74xx,cmos4000,adc-dac,memory,xilinx,special,microcontrollers,dsp,microchip,analog_switches,motorola,texas,intel,audio,interface,digital-audio,philips,display,cypress,siliconi,contrib,valves,.\rs232_bluetooth.cache
EELAYER 24  0
EELAYER END
$Descr A4 11700 8267
Sheet 1 1
Title "Modu³ RS232 dla Bluetooth"
Date "17 sep 2008"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	3800 1350 3800 1650
Connection ~ 4350 1050
Wire Wire Line
	2900 1450 2900 1550
Connection ~ 3800 1550
Connection ~ 3800 1400
Wire Wire Line
	3800 1400 3250 1400
Wire Wire Line
	3250 1400 3250 1200
Wire Wire Line
	3250 1200 3400 1200
Connection ~ 2400 1000
Wire Wire Line
	3400 1000 2050 1000
Wire Wire Line
	4200 1050 4550 1050
Wire Wire Line
	5650 5200 5650 5050
Wire Wire Line
	5900 5150 5900 5050
Wire Wire Line
	5550 4050 5550 3850
Wire Wire Line
	5550 3850 6250 3850
Wire Wire Line
	6250 3950 6000 3950
Connection ~ 3950 2300
Wire Wire Line
	3350 2300 4150 2300
Wire Wire Line
	4150 3050 4150 3650
Wire Wire Line
	4150 3050 6250 3050
Connection ~ 3350 3850
Connection ~ 3750 3650
Wire Wire Line
	3750 3650 3750 2900
Wire Wire Line
	3350 3850 3350 2900
Wire Wire Line
	6250 2650 5800 2650
Wire Wire Line
	7000 1350 7000 1500
Wire Wire Line
	4500 3750 4500 3850
Wire Wire Line
	4500 3750 6250 3750
Wire Wire Line
	4150 3650 3200 3650
Wire Wire Line
	4400 3150 4400 3750
Wire Wire Line
	4400 3150 6250 3150
Connection ~ 2150 1200
Wire Wire Line
	2050 1200 2400 1200
Connection ~ 3350 2300
Wire Wire Line
	3350 2400 3350 2150
Connection ~ 3750 2300
Wire Wire Line
	3550 2300 3550 2400
Wire Wire Line
	8750 2650 9100 2650
Wire Wire Line
	9100 2650 9100 3150
Wire Wire Line
	9100 3150 9300 3150
Wire Wire Line
	8550 3050 8550 3650
Wire Wire Line
	8550 3050 7700 3050
Wire Wire Line
	7700 3650 8450 3650
Wire Wire Line
	8550 3650 9300 3650
Wire Wire Line
	1450 3100 1450 3050
Connection ~ 6600 900 
Wire Wire Line
	7000 950  7000 900 
Wire Wire Line
	7000 900  6350 900 
Connection ~ 8850 2650
Wire Wire Line
	8250 2550 8350 2550
Wire Wire Line
	8350 2650 8200 2650
Wire Wire Line
	8200 2650 8200 2750
Wire Wire Line
	8200 2750 7900 2750
Wire Wire Line
	7900 2750 7900 2650
Wire Wire Line
	7900 2650 7700 2650
Wire Wire Line
	5350 2850 6250 2850
Wire Wire Line
	6250 2750 5600 2750
Wire Wire Line
	6250 2550 6250 2250
Wire Wire Line
	7850 2550 7700 2550
Wire Wire Line
	6600 850  6600 900 
Wire Wire Line
	6350 900  6350 850 
Wire Wire Line
	1450 3050 1850 3050
Wire Wire Line
	1850 3850 1450 3850
Wire Wire Line
	1450 3850 1450 3750
Wire Wire Line
	7700 3750 9300 3750
Wire Wire Line
	9300 3450 8450 3450
Wire Wire Line
	8450 3450 8450 3650
Wire Wire Line
	9300 3550 8650 3550
Wire Wire Line
	8650 3550 8650 3150
Wire Wire Line
	8650 3150 7700 3150
Wire Wire Line
	8850 2800 8850 2050
Wire Wire Line
	8850 2050 8350 2050
Wire Wire Line
	8350 2050 8350 2550
Wire Wire Line
	2150 1350 2150 1200
Wire Wire Line
	3950 2400 3950 2300
Wire Wire Line
	3750 2300 3750 2400
Connection ~ 3550 2300
Wire Wire Line
	3200 3550 4500 3550
Wire Wire Line
	4400 3750 3200 3750
Wire Wire Line
	4500 3850 3200 3850
Wire Wire Line
	6250 3650 4500 3650
Wire Wire Line
	4500 3650 4500 3550
Wire Wire Line
	6250 2250 5800 2250
Wire Wire Line
	5350 2450 5600 2450
Wire Wire Line
	5600 2450 5600 2750
Wire Wire Line
	3550 2900 3550 3750
Connection ~ 3550 3750
Wire Wire Line
	3950 2900 3950 3550
Connection ~ 3950 3550
Wire Wire Line
	6250 2950 4150 2950
Wire Wire Line
	4150 2950 4150 2900
Wire Wire Line
	4150 2300 4150 2400
Wire Wire Line
	6000 3950 6000 4050
Wire Wire Line
	5450 5150 5450 5050
Wire Wire Line
	6100 5200 6100 5050
Wire Wire Line
	2150 900  2150 1000
Connection ~ 2150 1000
Wire Wire Line
	4550 1050 4550 850 
Wire Wire Line
	2900 1050 2900 1000
Connection ~ 2900 1000
Wire Wire Line
	4350 1450 4350 1550
Wire Wire Line
	4350 1550 2900 1550
Text Notes 7200 900  0    60   ~
Close to U1
$Comp
L CAPAPOL C7
U 1 1 48D15FAF
P 4350 1250
F 0 "C7" H 4400 1350 50  0000 L C
F 1 "2.2uF" H 4400 1150 50  0000 L C
	1    4350 1250
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C6
U 1 1 48D15F6D
P 2900 1250
F 0 "C6" H 2950 1350 50  0000 L C
F 1 "0.1uF" H 2950 1150 50  0000 L C
	1    2900 1250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 48D15E77
P 3800 1650
F 0 "#PWR01" H 3800 1650 30  0001 C C
F 1 "GND" H 3800 1580 30  0001 C C
	1    3800 1650
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR02
U 1 1 48D15E59
P 2150 900
F 0 "#PWR02" H 2150 990 20  0001 C C
F 1 "+5V" H 2150 990 30  0000 C C
	1    2150 900 
	1    0    0    -1  
$EndComp
$Comp
L LE33ABD U2
U 1 1 48D15DD2
P 3800 1100
F 0 "U2" H 3950 904 60  0000 C C
F 1 "LE33ABD" H 3800 1300 60  0000 C C
F 2 "SO8E" H 3800 1100 50  0000 C C
	1    3800 1100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 48D01DC7
P 6100 5200
F 0 "#PWR03" H 6100 5200 30  0001 C C
F 1 "GND" H 6100 5130 30  0001 C C
	1    6100 5200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 48D01DC1
P 5650 5200
F 0 "#PWR04" H 5650 5200 30  0001 C C
F 1 "GND" H 5650 5130 30  0001 C C
	1    5650 5200
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR05
U 1 1 48D01D7E
P 5900 5150
F 0 "#PWR05" H 5900 5250 30  0001 C C
F 1 "VCC" H 5900 5250 30  0000 C C
	1    5900 5150
	-1   0    0    1   
$EndComp
$Comp
L SWITCH_INV SW1
U 1 1 48D01D5A
P 5550 4550
F 0 "SW1" H 5350 4700 50  0000 C C
F 1 "SWITCH_INV" H 5400 4400 50  0000 C C
	1    5550 4550
	0    1    1    0   
$EndComp
$Comp
L SWITCH_INV SW2
U 1 1 48D01D34
P 6000 4550
F 0 "SW2" H 5800 4700 50  0000 C C
F 1 "SWITCH_INV" H 5850 4400 50  0000 C C
	1    6000 4550
	0    1    1    0   
$EndComp
$Comp
L R R5
U 1 1 48D01BC8
P 4150 2650
F 0 "R5" V 4230 2650 50  0000 C C
F 1 "10k" V 4150 2650 50  0000 C C
	1    4150 2650
	1    0    0    -1  
$EndComp
$Comp
L MAX3243ECAI+ U1
U 1 1 48AC7C74
P 7000 2400
F 0 "U1" H 7000 2400 60  0000 C C
F 1 "MAX3243ECAI+" V 7000 1550 60  0000 C C
F 2 "SSOP28" H 7000 600 50  0000 C C
	1    7000 2400
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG06
U 1 1 48ADB8CD
P 2400 1200
F 0 "#FLG06" H 2400 1470 30  0001 C C
F 1 "PWR_FLAG" H 2400 1430 30  0000 C C
	1    2400 1200
	-1   0    0    1   
$EndComp
$Comp
L PWR_FLAG #FLG07
U 1 1 48ADB8C2
P 2400 1000
F 0 "#FLG07" H 2400 1270 30  0001 C C
F 1 "PWR_FLAG" H 2400 1230 30  0000 C C
	1    2400 1000
	1    0    0    -1  
$EndComp
NoConn ~ 6250 4050
NoConn ~ 1850 3750
NoConn ~ 1850 3650
NoConn ~ 1850 3550
NoConn ~ 1850 3450
NoConn ~ 1850 3350
NoConn ~ 1850 3250
NoConn ~ 1850 3150
NoConn ~ 3200 3450
NoConn ~ 3200 3350
NoConn ~ 3200 3250
NoConn ~ 3200 3150
NoConn ~ 3200 3050
NoConn ~ 6250 3250
NoConn ~ 6250 3350
NoConn ~ 6250 3450
NoConn ~ 6250 3550
NoConn ~ 7700 3550
NoConn ~ 7700 3450
NoConn ~ 7700 3350
NoConn ~ 7700 2950
NoConn ~ 9300 3950
NoConn ~ 9300 3850
NoConn ~ 9300 3350
NoConn ~ 9300 3250
$Comp
L VCC #PWR08
U 1 1 48AD9A99
P 3350 2150
F 0 "#PWR08" H 3350 2250 30  0001 C C
F 1 "VCC" H 3350 2250 30  0000 C C
	1    3350 2150
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 48AD9A05
P 3950 2650
F 0 "R4" V 4030 2650 50  0000 C C
F 1 "10k" V 3950 2650 50  0000 C C
	1    3950 2650
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 48AD9A00
P 3750 2650
F 0 "R3" V 3830 2650 50  0000 C C
F 1 "10k" V 3750 2650 50  0000 C C
	1    3750 2650
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 48AD99FC
P 3550 2650
F 0 "R2" V 3630 2650 50  0000 C C
F 1 "10k" V 3550 2650 50  0000 C C
	1    3550 2650
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 48AD99ED
P 3350 2650
F 0 "R1" V 3430 2650 50  0000 C C
F 1 "10k" V 3350 2650 50  0000 C C
	1    3350 2650
	1    0    0    -1  
$EndComp
Text Notes 950  1100 0    60   ~
Zasilanie
$Comp
L +3,3V #PWR09
U 1 1 48AD9418
P 4550 850
F 0 "#PWR09" H 4550 810 30  0001 C C
F 1 "+3,3V" H 4550 960 30  0000 C C
	1    4550 850 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR010
U 1 1 48AD940A
P 2150 1350
F 0 "#PWR010" H 2150 1350 30  0001 C C
F 1 "GND" H 2150 1280 30  0001 C C
	1    2150 1350
	1    0    0    -1  
$EndComp
$Comp
L CONN_2 P3
U 1 1 48AD93FB
P 1700 1100
F 0 "P3" V 1650 1100 40  0000 C C
F 1 "CONN_2" V 1750 1100 40  0000 C C
F 2 "SIL-2" V 1700 1400 60  0000 C C
	1    1700 1100
	-1   0    0    1   
$EndComp
$Comp
L CONN_9 P1
U 1 1 48AD934C
P 2200 3450
F 0 "P1" V 2150 3450 60  0000 C C
F 1 "CONN_9" V 2250 3450 60  0000 C C
F 2 "SIL9" V 2200 4050 60  0000 C C
	1    2200 3450
	1    0    0    -1  
$EndComp
$Comp
L CONN_9 P2
U 1 1 48AD924E
P 2850 3450
F 0 "P2" V 2800 3450 60  0000 C C
F 1 "CONN_9" V 2900 3450 60  0000 C C
F 2 "SIL9" V 2850 2850 60  0000 C C
	1    2850 3450
	-1   0    0    1   
$EndComp
Text Notes 9800 2850 0    60   ~
DTE
Text Notes 9950 3200 0    60   ~
GND
Text Notes 9950 3300 0    60   ~
RING
Text Notes 9950 3400 0    60   ~
DTR
Text Notes 9950 3900 0    60   ~
DSR
Text Notes 9950 4000 0    60   ~
DCD
Text Notes 9950 3600 0    60   ~
TxD
Text Notes 9950 3500 0    60   ~
CTS
Text Notes 9950 3700 0    60   ~
RTS
Text Notes 9950 3800 0    60   ~
RxD
$Comp
L DB9 J1
U 1 1 48AC9206
P 9750 3550
F 0 "J1" H 9750 4100 70  0000 C C
F 1 "DB9" H 9750 3000 70  0000 C C
F 2 "DB9M_CI" H 9680 4170 60  0000 C C
	1    9750 3550
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR011
U 1 1 48AC8142
P 1450 3100
F 0 "#PWR011" H 1450 3100 30  0001 C C
F 1 "GND" H 1450 3030 30  0001 C C
	1    1450 3100
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR012
U 1 1 48AC8130
P 1450 3750
F 0 "#PWR012" H 1450 3850 30  0001 C C
F 1 "VCC" H 1450 3850 30  0000 C C
	1    1450 3750
	1    0    0    -1  
$EndComp
Text Notes 2300 3100 0    60   ~
GND
Text Notes 2750 3550 2    60   ~
CTS
Text Notes 2750 3650 2    60   ~
RTS
Text Notes 2750 3750 2    60   ~
TX
Text Notes 2750 3850 2    60   ~
RX
$Comp
L GND #PWR013
U 1 1 48AC7ED6
P 7000 1500
F 0 "#PWR013" H 7000 1500 30  0001 C C
F 1 "GND" H 7000 1430 30  0001 C C
	1    7000 1500
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C3
U 1 1 48AC7DE6
P 7000 1150
F 0 "C3" H 7050 1250 50  0000 L C
F 1 "0.1uF" H 7050 1050 50  0000 L C
F 2 "SM0603" H 6700 1150 60  0000 C C
	1    7000 1150
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR014
U 1 1 48AC7DE0
P 6600 850
F 0 "#PWR014" H 6600 950 30  0001 C C
F 1 "VCC" H 6600 950 30  0000 C C
	1    6600 850 
	1    0    0    -1  
$EndComp
$Comp
L +3,3V #PWR015
U 1 1 48AC7DD6
P 6350 850
F 0 "#PWR015" H 6350 810 30  0001 C C
F 1 "+3,3V" H 6350 960 30  0000 C C
	1    6350 850 
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR016
U 1 1 48AC7CE1
P 5450 5150
F 0 "#PWR016" H 5450 5250 30  0001 C C
F 1 "VCC" H 5450 5250 30  0000 C C
	1    5450 5150
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR017
U 1 1 48AC7BC1
P 8850 2800
F 0 "#PWR017" H 8850 2800 30  0001 C C
F 1 "GND" H 8850 2730 30  0001 C C
	1    8850 2800
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C5
U 1 1 48AC7AFD
P 8550 2650
F 0 "C5" H 8600 2750 50  0000 L C
F 1 "0.1uF" H 8600 2550 50  0000 L C
F 2 "SM0603" H 8900 2650 60  0000 C C
	1    8550 2650
	0    1    -1   0   
$EndComp
$Comp
L CAPAPOL C4
U 1 1 48AC7AF8
P 8050 2550
F 0 "C4" H 8100 2650 50  0000 L C
F 1 "0.1uF" H 8100 2450 50  0000 L C
F 2 "SM0603" H 8400 2550 60  0000 C C
	1    8050 2550
	0    -1   -1   0   
$EndComp
$Comp
L CAPAPOL C1
U 1 1 48AC79C4
P 5350 2650
F 0 "C1" H 5200 2750 50  0000 L C
F 1 "0.1uF" H 4950 2550 50  0000 L C
F 2 "SM0603" H 5050 2650 60  0000 C C
	1    5350 2650
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C2
U 1 1 48AC78D4
P 5800 2450
F 0 "C2" H 5850 2550 50  0000 L C
F 1 "0.1uF" H 5850 2350 50  0000 L C
F 2 "SM0603" H 5550 2550 60  0000 C C
	1    5800 2450
	1    0    0    -1  
$EndComp
$EndSCHEMATC
