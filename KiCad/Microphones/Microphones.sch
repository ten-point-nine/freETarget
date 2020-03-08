EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_02x03_Top_Bottom MIC?
U 1 1 5E703D38
P 2950 2200
F 0 "MIC?" V 2954 2380 50  0000 L CNN
F 1 "Conn_02x03_Top_Bottom" V 3045 2380 50  0000 L CNN
F 2 "Digikey Pot:SPU01410LR6H-QB-7" H 2950 2200 50  0001 C CNN
F 3 "~" H 2950 2200 50  0001 C CNN
	1    2950 2200
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J?
U 1 1 5E706054
P 2950 3100
F 0 "J?" V 2822 2912 50  0000 R CNN
F 1 "Conn_01x03" V 2913 2912 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 2950 3100 50  0001 C CNN
F 3 "~" H 2950 3100 50  0001 C CNN
	1    2950 3100
	0    -1   1    0   
$EndComp
Wire Wire Line
	2850 2900 2850 2700
Wire Wire Line
	2850 2700 3050 2700
Wire Wire Line
	3050 2700 3050 2500
Wire Wire Line
	3050 2900 3550 2900
Wire Wire Line
	3550 2900 3550 1650
Wire Wire Line
	3550 1650 2950 1650
Wire Wire Line
	2950 1650 2950 2000
Wire Wire Line
	3050 2000 3300 2000
Wire Wire Line
	3300 2000 3300 2800
Wire Wire Line
	3300 2800 2950 2800
Wire Wire Line
	2950 2800 2950 2900
$EndSCHEMATC
