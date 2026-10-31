EESchema Schematic File Version 4
LIBS:4DigitIN2Display-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Wemos D1 mini 4-Digit Main Board IN2 Display Board"
Date "2018-10-28"
Rev ""
Comp "Ian Sparkes Nixie Clocks"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 950  900  0    70   ~ 0
Anode Control
$Comp
L 4DigitIN2Display-rescue:PC817-4DigitIN12-cache OK4
U 1 1 5696C7B0
P 2250 3400
F 0 "OK4" H 1975 3625 70  0000 L BNN
F 1 "EL817C" H 1975 3100 70  0000 L BNN
F 2 "NixieClockMaster:optocoupler-DIL04" H 2240 3190 70  0001 L TNN
F 3 "" H 2250 3400 60  0001 C CNN
F 4 "Value" H 2250 3400 60  0001 C CNN "Fieldname"
	1    2250 3400
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:PC817-4DigitIN12-cache OK3
U 1 1 5696C878
P 2250 2700
F 0 "OK3" H 1975 2925 70  0000 L BNN
F 1 "EL817C" H 1975 2400 70  0000 L BNN
F 2 "NixieClockMaster:optocoupler-DIL04" H 2240 2490 70  0001 L TNN
F 3 "" H 2250 2700 60  0001 C CNN
F 4 "Value" H 2250 2700 60  0001 C CNN "Fieldname"
	1    2250 2700
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:PC817-4DigitIN12-cache OK2
U 1 1 5696C940
P 2250 2000
F 0 "OK2" H 1975 2225 70  0000 L BNN
F 1 "EL817C" H 1975 1700 70  0000 L BNN
F 2 "NixieClockMaster:optocoupler-DIL04" H 2240 1790 70  0001 L TNN
F 3 "" H 2250 2000 60  0001 C CNN
F 4 "Value" H 2250 2000 60  0001 C CNN "Fieldname"
	1    2250 2000
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:PC817-4DigitIN12-cache OK1
U 1 1 5696CA08
P 2250 1300
F 0 "OK1" H 1975 1525 70  0000 L BNN
F 1 "EL817C" H 1975 1000 70  0000 L BNN
F 2 "NixieClockMaster:optocoupler-DIL04" H 2240 1090 70  0001 L TNN
F 3 "" H 2250 1300 60  0001 C CNN
F 4 "Value" H 2250 1300 60  0001 C CNN "Fieldname"
	1    2250 1300
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:Q_NPN_EBC-4DigitIN12Display-rescue Q1
U 1 1 5696D048
P 6750 2600
F 0 "Q1" H 6650 2350 70  0000 L BNN
F 1 "MPSA42" H 6350 2750 70  0000 L BNN
F 2 "NixieClockMaster:TO-92_inline_MPSA42" H 6740 2390 70  0001 L TNN
F 3 "" H 6750 2600 60  0001 C CNN
	1    6750 2600
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R2
U 1 1 5696D110
P 1600 1900
F 0 "R2" H 1450 1958 70  0000 L BNN
F 1 "1k" H 1450 1770 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 1590 1690 70  0001 L TNN
F 3 "" H 1600 1900 60  0001 C CNN
	1    1600 1900
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R3
U 1 1 5696D1D8
P 1600 2600
F 0 "R3" H 1450 2658 70  0000 L BNN
F 1 "1k" H 1450 2470 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 1590 2390 70  0001 L TNN
F 3 "" H 1600 2600 60  0001 C CNN
	1    1600 2600
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R4
U 1 1 5696D2A0
P 1600 3300
F 0 "R4" H 1450 3358 70  0000 L BNN
F 1 "1k" H 1450 3170 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 1590 3090 70  0001 L TNN
F 3 "" H 1600 3300 60  0001 C CNN
	1    1600 3300
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R6
U 1 1 5696D368
P 2900 2100
F 0 "R6" H 2750 2158 70  0000 L BNN
F 1 "3k" H 2750 1970 70  0000 L BNN
F 2 "NixieClockMaster:R-Long" H 2890 1890 70  0001 L TNN
F 3 "" H 2900 2100 60  0001 C CNN
	1    2900 2100
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R7
U 1 1 5696D430
P 2900 2800
F 0 "R7" H 2750 2858 70  0000 L BNN
F 1 "3k" H 2750 2670 70  0000 L BNN
F 2 "NixieClockMaster:R-Long" H 2890 2590 70  0001 L TNN
F 3 "" H 2900 2800 60  0001 C CNN
	1    2900 2800
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R8
U 1 1 5696D4F8
P 2900 3500
F 0 "R8" H 2750 3558 70  0000 L BNN
F 1 "3k" H 2750 3370 70  0000 L BNN
F 2 "NixieClockMaster:R-Long" H 2890 3290 70  0001 L TNN
F 3 "" H 2900 3500 60  0001 C CNN
	1    2900 3500
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R1
U 1 1 5696DB38
P 1600 1200
F 0 "R1" H 1450 1258 70  0000 L BNN
F 1 "1k" H 1450 1070 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 1590 990 70  0001 L TNN
F 3 "" H 1600 1200 60  0001 C CNN
	1    1600 1200
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R5
U 1 1 5696DD90
P 2900 1400
F 0 "R5" H 2750 1458 70  0000 L BNN
F 1 "3k" H 2750 1270 70  0000 L BNN
F 2 "NixieClockMaster:R-Long" H 2890 1190 70  0001 L TNN
F 3 "" H 2900 1400 60  0001 C CNN
	1    2900 1400
	-1   0    0    1   
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R10
U 1 1 5696DF20
P 6850 2150
F 0 "R10" H 6700 2208 70  0000 L BNN
F 1 "1k" H 6700 2020 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 6840 1940 70  0001 L TNN
F 3 "" H 6850 2150 60  0001 C CNN
	1    6850 2150
	0    -1   -1   0   
$EndComp
Text Label 900  1200 0    65   ~ 0
A1_I
Text Label 900  1900 0    65   ~ 0
A2_I
Text Label 900  2600 0    65   ~ 0
A3_I
Text Label 900  3300 0    65   ~ 0
A4_I
Text Label 1400 3300 2    20   ~ 0
ANODE1
Text Label 1400 2600 2    20   ~ 0
ANODE2
Text Label 1400 1900 2    20   ~ 0
ANODE3
Text Label 1400 1200 2    20   ~ 0
ANODE4
Text Label 1800 1400 0    20   ~ 0
GND
Text Label 1800 2100 0    20   ~ 0
GND
Text Label 1800 2800 0    20   ~ 0
GND
Text Label 1800 3500 0    20   ~ 0
GND
Text Label 2750 850  0    59   ~ 0
170V_I
Text Label 2750 1800 0    59   ~ 0
170V
Text Label 3450 3500 0    60   ~ 0
A4
Text Label 3450 2800 0    60   ~ 0
A3
Text Label 3450 2100 0    60   ~ 0
A2
Text Label 3450 1400 0    60   ~ 0
A1
$Comp
L 4DigitIN2Display-rescue:Conn_01x10-4DigitIN12-cache J1
U 1 1 594DD920
P 8900 5600
F 0 "J1" H 8900 6050 50  0000 C CNN
F 1 "Conn_01x10" V 8900 5600 50  0000 C CNN
F 2 "NixieClockMaster:Header_MA10-1_narrow" H 8900 4400 50  0001 C CNN
F 3 "" H 8900 4400 50  0001 C CNN
	1    8900 5600
	-1   0    0    1   
$EndComp
Text Label 1050 5700 0    60   ~ 0
C_1_I
Text Label 1050 5800 0    60   ~ 0
C_3_I
Text Label 1050 5900 0    60   ~ 0
C_5_I
Text Label 1050 6000 0    60   ~ 0
C_7_I
Text Label 1050 6100 0    60   ~ 0
C_9_I
Text Label 7600 5750 2    60   ~ 0
C_2_I
Text Label 7600 5850 2    60   ~ 0
C_4_I
Text Label 7600 5950 2    60   ~ 0
C_6_I
Text Label 7600 6050 2    60   ~ 0
C_8_I
Text Label 7600 6150 2    60   ~ 0
C_0_I
Text Label 10600 5800 2    65   ~ 0
T_LED1_I
Text Label 9450 5100 2    60   ~ 0
C_0_I
Text Label 9450 5300 2    60   ~ 0
C_2_I
Text Label 9450 5500 2    60   ~ 0
C_4_I
Text Label 9450 5700 2    60   ~ 0
C_6_I
Text Label 9450 5900 2    60   ~ 0
C_8_I
Text Label 10600 5100 2    60   ~ 0
A1_I
Text Label 10600 5300 2    60   ~ 0
A3_I
Text Label 9450 5200 2    60   ~ 0
C_1_I
Text Label 9450 5400 2    60   ~ 0
C_3_I
Text Label 9450 5600 2    60   ~ 0
C_5_I
Text Label 9450 5800 2    60   ~ 0
C_7_I
Text Label 9450 6000 2    60   ~ 0
C_9_I
Text Label 10600 5200 2    60   ~ 0
A2_I
Text Label 10600 5400 2    60   ~ 0
A4_I
Text Label 2250 4650 0    60   ~ 0
A1
Text Label 3850 4650 0    60   ~ 0
A2
Text Label 5450 4650 0    60   ~ 0
A3
Text Label 7050 4650 0    60   ~ 0
A4
Text Label 10600 5900 2    65   ~ 0
DLS_I
Text Label 10600 6000 2    60   ~ 0
GND_I
Text Label 10600 5500 2    60   ~ 0
170V_I
Text Notes 8800 4650 0    70   ~ 0
Connectors to main board
Wire Wire Line
	900  3300 1400 3300
Wire Wire Line
	900  2600 1400 2600
Wire Wire Line
	900  1900 1400 1900
Wire Wire Line
	900  1200 1400 1200
Wire Wire Line
	1950 1400 1800 1400
Wire Wire Line
	1950 2100 1800 2100
Wire Wire Line
	1800 2800 1950 2800
Wire Wire Line
	1650 3500 1800 3500
Wire Wire Line
	1800 1200 1950 1200
Wire Wire Line
	2550 1400 2700 1400
Wire Wire Line
	1800 1900 1950 1900
Wire Wire Line
	2550 2100 2700 2100
Wire Wire Line
	1800 2600 1950 2600
Wire Wire Line
	1800 3300 1950 3300
Wire Wire Line
	2550 2800 2700 2800
Wire Wire Line
	2550 3500 2700 3500
Wire Wire Line
	2750 1200 2550 1200
Wire Wire Line
	2750 1900 2550 1900
Wire Wire Line
	2750 850  2750 1200
Wire Wire Line
	2750 2600 2550 2600
Connection ~ 2750 1900
Wire Wire Line
	2750 3300 2550 3300
Connection ~ 2750 2600
Connection ~ 2750 1200
Wire Wire Line
	3100 1400 3450 1400
Wire Wire Line
	3100 2100 3450 2100
Wire Wire Line
	3100 2800 3450 2800
Wire Wire Line
	3100 3500 3450 3500
Wire Wire Line
	1050 5700 1450 5700
Connection ~ 1450 5700
Connection ~ 3050 5700
Connection ~ 4650 5700
Wire Wire Line
	1550 5750 3150 5750
Connection ~ 4750 5750
Connection ~ 3150 5750
Connection ~ 6350 5750
Wire Wire Line
	1050 5800 1650 5800
Connection ~ 4850 5800
Connection ~ 3250 5800
Connection ~ 1650 5800
Wire Wire Line
	1750 5850 3350 5850
Connection ~ 3350 5850
Connection ~ 4950 5850
Connection ~ 6550 5850
Wire Wire Line
	1050 5900 1850 5900
Connection ~ 5050 5900
Connection ~ 3450 5900
Connection ~ 1850 5900
Wire Wire Line
	1950 5950 3550 5950
Connection ~ 3550 5950
Connection ~ 5150 5950
Connection ~ 6750 5950
Wire Wire Line
	1050 6000 2050 6000
Connection ~ 5250 6000
Connection ~ 3650 6000
Connection ~ 2050 6000
Wire Wire Line
	2150 6050 3750 6050
Connection ~ 3750 6050
Connection ~ 5350 6050
Connection ~ 6950 6050
Wire Wire Line
	1050 6100 2250 6100
Connection ~ 5450 6100
Connection ~ 3850 6100
Connection ~ 2250 6100
Wire Wire Line
	2350 6150 3950 6150
Connection ~ 3950 6150
Connection ~ 5550 6150
Connection ~ 7150 6150
Wire Wire Line
	9100 5600 9450 5600
Wire Wire Line
	9450 5700 9100 5700
Wire Wire Line
	9450 5800 9100 5800
Wire Wire Line
	9450 5900 9100 5900
Wire Wire Line
	9450 6000 9100 6000
Wire Wire Line
	9100 5300 9450 5300
Wire Wire Line
	9100 5400 9450 5400
Wire Wire Line
	9100 5500 9450 5500
Wire Wire Line
	9100 5100 9450 5100
Wire Wire Line
	9100 5200 9450 5200
Wire Wire Line
	1950 4650 2250 4650
Wire Wire Line
	3550 4650 3850 4650
Wire Wire Line
	5150 4650 5450 4650
Wire Wire Line
	6750 4650 7050 4650
Wire Wire Line
	10200 5100 10600 5100
Wire Wire Line
	10200 5200 10600 5200
Wire Wire Line
	10200 5300 10600 5300
Wire Wire Line
	10200 5400 10600 5400
Wire Wire Line
	1800 1400 1800 2100
Connection ~ 1800 2800
Connection ~ 1800 2100
Wire Wire Line
	1650 3500 1650 3850
Connection ~ 1800 3500
Wire Wire Line
	10200 5600 10600 5600
Wire Wire Line
	10200 5700 10600 5700
Text Label 5700 2600 0    60   ~ 0
T_LED1_I
Wire Wire Line
	10200 6000 10600 6000
Wire Wire Line
	10200 5900 10600 5900
Wire Wire Line
	10200 5800 10600 5800
Text Label 10600 5700 2    65   ~ 0
T_LED2_I
Wire Wire Line
	6850 2350 6850 2400
Wire Wire Line
	6850 2800 6850 3200
$Comp
L 4DigitIN2Display-rescue:Q_NPN_EBC-4DigitIN12Display-rescue Q2
U 1 1 5BD4A87B
P 8350 2600
F 0 "Q2" H 8250 2350 70  0000 L BNN
F 1 "MPSA42" H 7950 2750 70  0000 L BNN
F 2 "NixieClockMaster:TO-92_inline_MPSA42" H 8340 2390 70  0001 L TNN
F 3 "" H 8350 2600 60  0001 C CNN
	1    8350 2600
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R12
U 1 1 5BD4A881
P 8450 2150
F 0 "R12" H 8300 2208 70  0000 L BNN
F 1 "1k" H 8300 2020 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 8440 1940 70  0001 L TNN
F 3 "" H 8450 2150 60  0001 C CNN
	1    8450 2150
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7700 2600 7400 2600
Text Label 7400 2600 0    60   ~ 0
T_LED2_I
Wire Wire Line
	8450 2350 8450 2400
Wire Wire Line
	8450 3200 8450 2800
Wire Wire Line
	6850 3200 7900 3200
Wire Wire Line
	7900 3200 7900 3500
Wire Wire Line
	7900 3500 8200 3500
Connection ~ 7900 3200
Text Label 8200 3500 0    60   ~ 0
GND_I
$Comp
L 4DigitIN2Display-rescue:Lamp_Neon-4DigitIN12-cache NE2
U 1 1 5BD4B3D7
P 8450 1750
F 0 "NE2" H 8475 1900 50  0000 L CNN
F 1 "Lamp_Neon" H 8475 1600 50  0000 L CNN
F 2 "NixieClockMaster:LED-5MM-Centred" V 8450 1850 50  0001 C CNN
F 3 "" V 8450 1850 50  0001 C CNN
	1    8450 1750
	1    0    0    -1  
$EndComp
$Comp
L 4DigitIN2Display-rescue:Lamp_Neon-4DigitIN12-cache NE1
U 1 1 5BD4B4BB
P 6850 1750
F 0 "NE1" H 6875 1900 50  0000 L CNN
F 1 "Lamp_Neon" H 6875 1600 50  0000 L CNN
F 2 "NixieClockMaster:LED-5MM-Centred" V 6850 1850 50  0001 C CNN
F 3 "" V 6850 1850 50  0001 C CNN
	1    6850 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	6850 1550 6850 1350
Wire Wire Line
	6850 1350 7900 1350
Wire Wire Line
	8450 1350 8450 1550
Wire Wire Line
	7900 1350 7900 1100
Wire Wire Line
	7900 1100 8850 1100
Connection ~ 7900 1350
Text Label 9350 1100 0    60   ~ 0
170V_I
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R9
U 1 1 5BD54E86
P 6300 2600
F 0 "R9" H 6150 2658 70  0000 L BNN
F 1 "4.7k" H 6150 2470 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 6290 2390 70  0001 L TNN
F 3 "" H 6300 2600 60  0001 C CNN
	1    6300 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	6100 2600 5700 2600
Wire Wire Line
	6500 2600 6550 2600
$Comp
L 4DigitIN2Display-rescue:RCL_R-EU_0207_2V-4DigitIN12Display-rescue R11
U 1 1 5BD555D9
P 7900 2600
F 0 "R11" H 7750 2658 70  0000 L BNN
F 1 "4.7k" H 7750 2470 70  0000 L BNN
F 2 "NixieClockMaster:R-0207_2V" H 7890 2390 70  0001 L TNN
F 3 "" H 7900 2600 60  0001 C CNN
	1    7900 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	8100 2600 8150 2600
Wire Wire Line
	9700 3200 9700 2900
Wire Wire Line
	9700 2500 9850 2500
Text Label 9850 2500 0    60   ~ 0
DLS_I
$Comp
L 4DigitIN2Display-rescue:R_PHOTO-4DigitIN12-cache LDR1
U 1 1 594E2512
P 9700 2750
F 0 "LDR1" H 9750 2800 50  0000 L CNN
F 1 "R_PHOTO" H 9750 2750 50  0000 L TNN
F 2 "NixieClockMaster:NEON-5MM-Centred" V 9750 2500 50  0001 L CNN
F 3 "" H 9700 2700 50  0001 C CNN
	1    9700 2750
	1    0    0    -1  
$EndComp
Connection ~ 8450 3200
Wire Wire Line
	1650 3850 1900 3850
Text Label 1900 3850 0    60   ~ 0
GND_I
$Comp
L 4DigitIN2Display-rescue:Conn_01x10-4DigitIN12-cache J2
U 1 1 5BD5A756
P 10000 5600
F 0 "J2" H 10000 6050 50  0000 C CNN
F 1 "Conn_01x10" V 10000 5600 50  0000 C CNN
F 2 "NixieClockMaster:Header_MA10-1_narrow" H 10000 4400 50  0001 C CNN
F 3 "" H 10000 4400 50  0001 C CNN
	1    10000 5600
	-1   0    0    1   
$EndComp
Text Label 10600 5600 2    60   ~ 0
VCC_I
Wire Wire Line
	10200 5500 10600 5500
$Comp
L 4DigitIN2Display-rescue:Jumper_NC_Dual-4DigitIN12Display-rescue JP1
U 1 1 5BD5AF1B
P 9150 1500
F 0 "JP1" H 9200 1400 50  0000 L CNN
F 1 "Jumper_NC_Dual" H 9150 1600 50  0000 C BNN
F 2 "NixieClockMaster:SOLDER-JUMPER_2-WAY" H 9150 1500 50  0001 C CNN
F 3 "" H 9150 1500 50  0001 C CNN
	1    9150 1500
	0    1    1    0   
$EndComp
Wire Wire Line
	9050 1500 8850 1500
Wire Wire Line
	8850 1500 8850 1100
Wire Wire Line
	9150 1250 9150 1100
Wire Wire Line
	9150 1100 9350 1100
Wire Wire Line
	9150 1750 9150 1900
Text Label 9900 1900 0    60   ~ 0
VCC_I
Text Label 7550 1350 2    20   ~ 0
SEP_VCC
Text Notes 6750 850  0    70   ~ 0
Separator neons/LEDs and LDR
Text Notes 4000 4400 0    70   ~ 0
Tubes
Wire Notes Line
	550  4100 11100 4100
Wire Notes Line
	4850 550  4850 4100
Wire Notes Line
	8050 4100 8050 6500
Wire Wire Line
	2750 1900 2750 2600
Wire Wire Line
	2750 2600 2750 3300
Wire Wire Line
	2750 1200 2750 1900
Wire Wire Line
	1450 5700 3050 5700
Wire Wire Line
	3050 5700 4650 5700
Wire Wire Line
	4650 5700 6250 5700
Wire Wire Line
	4750 5750 6350 5750
Wire Wire Line
	3150 5750 4750 5750
Wire Wire Line
	6350 5750 7600 5750
Wire Wire Line
	4850 5800 6450 5800
Wire Wire Line
	3250 5800 4850 5800
Wire Wire Line
	3350 5850 4950 5850
Wire Wire Line
	4950 5850 6550 5850
Wire Wire Line
	6550 5850 7600 5850
Wire Wire Line
	5050 5900 6650 5900
Wire Wire Line
	3450 5900 5050 5900
Wire Wire Line
	1850 5900 3450 5900
Wire Wire Line
	3550 5950 5150 5950
Wire Wire Line
	5150 5950 6750 5950
Wire Wire Line
	6750 5950 7600 5950
Wire Wire Line
	5250 6000 6850 6000
Wire Wire Line
	3650 6000 5250 6000
Wire Wire Line
	2050 6000 3650 6000
Wire Wire Line
	3750 6050 5350 6050
Wire Wire Line
	5350 6050 6950 6050
Wire Wire Line
	6950 6050 7600 6050
Wire Wire Line
	5450 6100 7050 6100
Wire Wire Line
	3850 6100 5450 6100
Wire Wire Line
	2250 6100 3850 6100
Wire Wire Line
	3950 6150 5550 6150
Wire Wire Line
	5550 6150 7150 6150
Wire Wire Line
	7150 6150 7600 6150
Wire Wire Line
	1800 2800 1800 3500
Wire Wire Line
	1800 2100 1800 2800
Wire Wire Line
	1800 3500 1950 3500
Wire Wire Line
	7900 3200 8450 3200
Wire Wire Line
	7900 1350 8450 1350
Wire Wire Line
	8450 3200 9700 3200
Wire Wire Line
	9700 2500 9700 2600
Wire Wire Line
	1650 5800 3250 5800
$Comp
L 4DigitIN2Display-rescue:IN-2-russian-nixies N1
U 1 1 5C03187E
P 1950 5100
F 0 "N1" V 1900 5250 50  0000 R CNN
F 1 "IN-2" V 1900 5050 50  0000 R CNN
F 2 "nixies-us:nixies-us-IN-2-DSUB" H 1950 5250 50  0001 C CNN
F 3 "" H 1950 5100 50  0001 C CNN
	1    1950 5100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1950 4650 1950 4800
Wire Wire Line
	1650 5500 1650 5800
Wire Wire Line
	1450 5500 1450 5700
Wire Wire Line
	1550 5500 1550 5750
Wire Wire Line
	1750 5500 1750 5850
Wire Wire Line
	1850 5500 1850 5900
Wire Wire Line
	1950 5500 1950 5950
Wire Wire Line
	2050 5500 2050 6000
Wire Wire Line
	2150 5500 2150 6050
Wire Wire Line
	2250 5500 2250 6100
Wire Wire Line
	2350 5500 2350 6150
$Comp
L 4DigitIN2Display-rescue:IN-2-russian-nixies N2
U 1 1 5C089F48
P 3550 5100
F 0 "N2" V 3500 5250 50  0000 R CNN
F 1 "IN-2" V 3500 5050 50  0000 R CNN
F 2 "nixies-us:nixies-us-IN-2-DSUB" H 3550 5250 50  0001 C CNN
F 3 "" H 3550 5100 50  0001 C CNN
	1    3550 5100
	0    -1   -1   0   
$EndComp
$Comp
L 4DigitIN2Display-rescue:IN-2-russian-nixies N3
U 1 1 5C08A087
P 5150 5100
F 0 "N3" V 5100 5250 50  0000 R CNN
F 1 "IN-2" V 5100 5050 50  0000 R CNN
F 2 "nixies-us:nixies-us-IN-2-DSUB" H 5150 5250 50  0001 C CNN
F 3 "" H 5150 5100 50  0001 C CNN
	1    5150 5100
	0    -1   -1   0   
$EndComp
$Comp
L 4DigitIN2Display-rescue:IN-2-russian-nixies N4
U 1 1 5C08A1AE
P 6750 5100
F 0 "N4" V 6700 5200 50  0000 R CNN
F 1 "IN-2" V 6700 5000 50  0000 R CNN
F 2 "nixies-us:nixies-us-IN-2-DSUB" H 6750 5250 50  0001 C CNN
F 3 "" H 6750 5100 50  0001 C CNN
	1    6750 5100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3550 4650 3550 4800
Wire Wire Line
	3050 5500 3050 5700
Wire Wire Line
	3150 5500 3150 5750
Wire Wire Line
	3250 5500 3250 5800
Wire Wire Line
	3350 5500 3350 5850
Wire Wire Line
	3450 5500 3450 5900
Wire Wire Line
	3550 5500 3550 5950
Wire Wire Line
	3650 5500 3650 6000
Wire Wire Line
	3750 5500 3750 6050
Wire Wire Line
	3850 5500 3850 6100
Wire Wire Line
	3950 5500 3950 6150
Wire Wire Line
	5150 4650 5150 4800
Wire Wire Line
	4650 5500 4650 5700
Wire Wire Line
	4750 5500 4750 5750
Wire Wire Line
	4850 5500 4850 5800
Wire Wire Line
	4950 5500 4950 5850
Wire Wire Line
	5050 5500 5050 5900
Wire Wire Line
	5150 5500 5150 5950
Wire Wire Line
	5250 5500 5250 6000
Wire Wire Line
	5350 5500 5350 6050
Wire Wire Line
	5450 5500 5450 6100
Wire Wire Line
	5550 5500 5550 6150
Wire Wire Line
	6750 4650 6750 4800
Wire Wire Line
	6250 5500 6250 5700
Wire Wire Line
	6350 5500 6350 5750
Wire Wire Line
	6450 5500 6450 5800
Wire Wire Line
	6550 5500 6550 5850
Wire Wire Line
	6650 5500 6650 5900
Wire Wire Line
	6750 5500 6750 5950
Wire Wire Line
	6850 5500 6850 6000
Wire Wire Line
	6950 5500 6950 6050
Wire Wire Line
	7050 5500 7050 6100
Wire Wire Line
	7150 5500 7150 6150
Wire Wire Line
	9150 1900 9900 1900
$EndSCHEMATC
