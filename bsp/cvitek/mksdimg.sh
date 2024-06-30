#/bin/sh
set -e

PROJECT_PATH=$1
IMAGE_NAME=$2

if [ -z "$PROJECT_PATH" ] || [ -z "$IMAGE_NAME" ]; then
	echo "Usage: $0 <PROJECT_DIR> <IMAGE_NAME>"
	exit 1
fi

ROOT_PATH=$(pwd)
echo ${ROOT_PATH}

. board_env.sh

get_board_type

echo $MV_BOARD_LINK
echo $CHIP_ARCH

echo "start compress kernel..."

lzma -c -9 -f -k ${PROJECT_PATH}/${IMAGE_NAME} > ${PROJECT_PATH}/dtb/${BOARD_TYPE}/Image.lzma

mkdir -p ${ROOT_PATH}/output/${BOARD_TYPE}
./pre-build/tools/mkimage -f ${PROJECT_PATH}/dtb/${BOARD_TYPE}/multi.its -r ${ROOT_PATH}/output/${BOARD_TYPE}/boot.${STORAGE_TYPE}

if [ "${STORAGE_TYPE}" == "spinor" ] || [ "${STORAGE_TYPE}" == "spinand" ]; then

	echo "arch: ${CHIP_ARCH}"
	echo "board: ${MV_BOARD_LINK}"
	
	IMGTOOL_PATH=${ROOT_PATH}/pre-build/tools/common/image_tool
	FLASH_PARTITION_XML=${ROOT_PATH}/pre-build/boards/"${CHIP_ARCH}"/"${MV_BOARD_LINK}"/partition/partition_"${STORAGE_TYPE}".xml
	python3 "$IMGTOOL_PATH"/raw2cimg.py "${ROOT_PATH}"/output/"${BOARD_TYPE}"/boot."$STORAGE_TYPE" "${ROOT_PATH}/output/${BOARD_TYPE}" "$FLASH_PARTITION_XML"
fi