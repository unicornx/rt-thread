#!/bin/bash

function get_board_type()
{
	BOARD_CONFIG=("CONFIG_BOARD_TYPE_MILKV_DUO" "CONFIG_BOARD_TYPE_MILKV_DUO_SPINOR" "CONFIG_BOARD_TYPE_MILKV_DUO_SPINAND" "CONFIG_BOARD_TYPE_MILKV_DUO256M" "CONFIG_BOARD_TYPE_MILKV_DUO256M_SPINOR" "CONFIG_BOARD_TYPE_MILKV_DUO256M_SPINAND")
	BOARD_VALUE=("milkv-duo" "milkv-duo-spinor" "milkv-duo-spinand" "milkv-duo256m" "milkv-duo256m-spinor" "milkv-duo256m-spinand")
	STORAGE_VAUE=("sd" "spinor" "spinand" "sd" "spinor" "spinand")

	for ((i=0;i<${#BOARD_CONFIG[@]};i++))
	do
		config_value=$(grep -w "${BOARD_CONFIG[i]}" ${PROJECT_PATH}/.config | cut -d= -f2)
		if [ "$config_value" == "y" ]; then
			BOARD_TYPE=${BOARD_VALUE[i]}
			STORAGE_TYPE=${STORAGE_VAUE[i]}
			break
		fi
	done

	export BOARD_TYPE=${BOARD_TYPE}
	export STORAGE_TYPE=${STORAGE_TYPE}

	echo $BOARD_TYPE
	if [ "${BOARD_TYPE}" == "milkv-duo" ]; then
		MV_BOARD_LINK="cv1800b_milkv_duo_sd"
		CHIP_ARCH="cv180x"
	elif [ "${BOARD_TYPE}" == "milkv-duo-spinand" ]; then
		MV_BOARD_LINK="cv1800b_milkv_duo_spinand"
		CHIP_ARCH="cv180x"
	elif [ "${BOARD_TYPE}" == "milkv-duo-spinor" ]; then
		MV_BOARD_LINK="cv1800b_milkv_duo_spinor"
		CHIP_ARCH="cv180x"
	elif [ "${BOARD_TYPE}" == "milkv-duo256m" ]; then
		MV_BOARD_LINK="cv1812cp_milkv_duo256m_sd"
		CHIP_ARCH="cv181x"
	elif [ "${BOARD_TYPE}" == "milkv-duo256m-spinand" ]; then
		MV_BOARD_LINK="cv1812cp_milkv_duo256m_spinand"
		CHIP_ARCH="cv181x"
	elif [ "${BOARD_TYPE}" == "milkv-duo256m-spinor" ]; then
		MV_BOARD_LINK="cv1812cp_milkv_duo256m_spinor"
		CHIP_ARCH="cv181x"
	fi

	export MV_BOARD_LINK=${MV_BOARD_LINK}
	export CHIP_ARCH=${CHIP_ARCH}
}

