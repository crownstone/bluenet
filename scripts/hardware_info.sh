#!/bin/sh

echo "The factory information registers (FICR) can be found online"
# https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fficr.html
echo "The chip versions as well"
# https://infocenter.nordicsemi.com/topic/comp_matrix_nrf52832/COMP/nrf52832/ic_revision_overview.html
echo
echo "Crownstone has IC chip revision 2. The QFN48 package, with 512 kB FLASH and 64 kB RAM denoted by QFAA-Bx0"
echo "(with x any digit, in our case a 0)"
echo

format_address() {
	address=$(echo "$1" | fold -w2 | paste -sd':' -)
}

format_ascii() {
	ascii=$(echo "0x$1" | xxd -r)
}

format_remove_leading_zeros() {
	result=$(echo $1 | sed 's/^0*//')
}

format_remove_leading_f() {
	result=$(echo $1 | sed 's/^F*//')
}

is_bit_set() {
	val=$1
	bit=$2
	div=1
	for i in $(seq 1 $bit); do
		div=$(expr $div '*' 2)
	done
	if [ $(expr \( $val / $div \) % 2) -eq 1 ]; then
		result="1";
	else
		result="0";
	fi
}

hardware_version=$(./readbytes.sh 10000100 4)
format_remove_leading_zeros $hardware_version
echo "Nordic hardware version:\t $result"

#hardware_version=$(nrfjprog -f nrf52 --memrd 0x10000100)
#echo "Nordic hardware version: $hardware_version"

board_variant=$(./readbytes.sh 10000104 4)

#board_variant=$(nrfjprog -f nrf52 --memrd 0x10000104 --n 4 | cut -f2 -d' ')

format_ascii $board_variant
echo "Board variant:\t\t\t $ascii"

echo -n "Board package:\t\t\t "
board_package=$(./readbytes.sh 10000108 2)
case $board_package in
	2000)
		echo "QFxx - 48-pin QFN"
		;;
	2001)
		echo "CHxx - 7x8 WLCSP 56 balls"
		;;
	2002)
		echo "CIxx - 7x8 WLCSP 56 balls"
		;;
	2005)
		echo "CKxx - 7x8 WLCSP 56 balls with backside coating for light protection"
		;;
	*)
		echo "Unknown package"
		;;
esac

echo -n "Complete board identifier:\t "
case $board_package in
	2000)
		echo -n "QF"
		;;
	2001)
		echo -n "CH"
		;;
	2002)
		echo -n "CI"
		;;
	2005)
		echo -n "CK"
		;;
esac

echo "$ascii" | fold -w2 | paste -sd'-' -

device_id=$(./readbytes.sh 10000060 8)
echo "Unique device id:\t\t $device_id"

format_address $(./readbytes.sh 100000A4 8)
echo "Bluetooth address:\t\t $address"

echo -n "Address type:\t\t\t "
is_bit_set $(echo -n 0x$(./readbytes.sh 100000A0 1) | xargs -0 printf "%d") 0
case $result in
	0)
		echo "Public address"
		;;
	1)
		echo "Random address"
		;;
	*)
		echo "Parsing error (bit should be 0 or 1)"
		;;
esac



ram=$(./readbytes.sh 1000010C 4)
format_remove_leading_zeros $ram
echo -n "RAM:\t\t\t\t "
case $result in
	10)
		echo "K16 (16 kB RAM)"
		;;
	20)
		echo "K32 (32 kB RAM)"
		;;
	40)
		echo "K64 (64 kB RAM)"
		;;
	*)
		echo "Unspecified"
		;;
esac




flash=$(./readbytes.sh 10000110 4)
format_remove_leading_zeros $flash
echo -n "FLASH:\t\t\t\t "
case $result in
	80)
		echo "K128 (128 kB FLASH)"
		;;
	100)
		echo "K256 (256 kB FLASH)"
		;;
	200)
		echo "K512 (512 kB FLASH)"
		;;
	*)
		echo "Unspecified"
		;;
esac

temperature_slope_0=$(./readbytes.sh 10000404 4)
format_remove_leading_f $temperature_slope_0
echo "Temperature slope A0:\t\t $result"

temperature_slope_1=$(./readbytes.sh 10000408 4)
format_remove_leading_f $temperature_slope_1
echo "Temperature slope A1:\t\t $result"

temperature_slope_2=$(./readbytes.sh 1000040C 4)
format_remove_leading_f $temperature_slope_2
echo "Temperature slope A2:\t\t $result"

temperature_slope_3=$(./readbytes.sh 10000410 4)
format_remove_leading_f $temperature_slope_3
echo "Temperature slope A3:\t\t $result"

temperature_slope_4=$(./readbytes.sh 10000414 4)
format_remove_leading_f $temperature_slope_4
echo "Temperature slope A4:\t\t $result"

temperature_slope_5=$(./readbytes.sh 10000418 5)
format_remove_leading_f $temperature_slope_5
echo "Temperature slope A5:\t\t $result"


temperature_intercept_0=$(./readbytes.sh 1000041C 4)
format_remove_leading_f $temperature_intercept_0
echo "Temperature y-intercept B0:\t $result"

temperature_intercept_1=$(./readbytes.sh 10000420 4)
format_remove_leading_f $temperature_intercept_1
echo "Temperature y-intercept B1:\t $result"

temperature_intercept_2=$(./readbytes.sh 10000424 4)
format_remove_leading_f $temperature_intercept_2
echo "Temperature y-intercept B2:\t $result"

temperature_intercept_3=$(./readbytes.sh 10000428 4)
format_remove_leading_f $temperature_intercept_3
echo "Temperature y-intercept B3:\t $result"

temperature_intercept_4=$(./readbytes.sh 1000042C 4)
format_remove_leading_f $temperature_intercept_4
echo "Temperature y-intercept B4:\t $result"

temperature_intercept_5=$(./readbytes.sh 10000430 4)
format_remove_leading_f $temperature_intercept_5
echo "Temperature y-intercept B5:\t $result"


temperature_segment_end_0=$(./readbytes.sh 10000434 4)
format_remove_leading_f $temperature_segment_end_0
echo "Temperature segment end T0:\t $result"

temperature_segment_end_1=$(./readbytes.sh 10000438 4)
format_remove_leading_f $temperature_segment_end_1
echo "Temperature segment end T1:\t $result"

temperature_segment_end_2=$(./readbytes.sh 1000043C 4)
format_remove_leading_f $temperature_segment_end_2
echo "Temperature segment end T2:\t $result"

temperature_segment_end_3=$(./readbytes.sh 10000440 4)
format_remove_leading_f $temperature_segment_end_3
echo "Temperature segment end T3:\t $result"

temperature_segment_end_4=$(./readbytes.sh 10000444 4)
format_remove_leading_f $temperature_segment_end_4
echo "Temperature segment end T4:\t $result"
