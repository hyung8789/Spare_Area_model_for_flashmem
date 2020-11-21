#include <stdio.h>
#include <windows.h>

#define SPARE_AREA_BYTE 16
#define SPARE_AREA_BIT 128

#define BLOCK_PER_SECTOR 32
#define SECTOR_PER_BYTE 512 //한 섹터의 데이터가 기록되는 영역의 크기 (byte)
#define SECTOR_INC_SPARE_BYTE 528 //Spare 영역을 포함한 한 섹터의 크기 (byte)

//8비트 크기의 Spare Area 초기값 지정
#define SPARE_INIT_VALUE (0xff) //0xff(16) = 255(10) = 11111111(2)
#define TRUE_BIT (0x1)
#define FALSE_BIT (0x0)

#define SUCCESS 1
#define COMPLETE 0
#define FAIL -1

/***
* 초기값 모두 0x1로 초기화
* 블록 단위 Erase 연산 수행 시 모두 0x1로 초기화
 ---------
* 각 섹터(페이지)의 Spare Area의 전체 16byte에 대해 첫 1byte를 블록 및 섹터(페이지)의 상태 정보로 사용
* BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
* 기타, Block Address(Logical) Area, ECC Area 등 추가적으로 사용 시, META_DATA 구조체, SPARE_read, SPARE_write 수정
***/

enum class BLOCK_STATE : const unsigned //블록 상태 정보
{
	/***
	* 각 블록의 첫 번째 섹터(페이지)의 Spare 영역에 해당 블록 정보를 관리
	* 일반 데이터 블록 혹은 직접적 데이터 기록이 불가능한 Spare 블록에 대하여,
	* EMPTY : 해당 블록은 블록 내의 모든 오프셋에 대해 비어있고, 유효하다. (초기 상태)
	* VALID : 해당 블록은 유효하고, 일부 유효한 데이터가 기록되어 있다.
	* INVALID : 해당 블록 내의 모든 오프셋에 대해 무효하거나, 일부 유효 데이터를 포함하고 있지만 더 이상 사용 불가능. 재사용 위해서 블록 단위 Erase 수행하여야 함
	***/

	/***
	* BLOCK_TYPE || IS_VALID || IS_EMPTY
	* 블록에 대한 6가지 상태, 2^3 = 8, 3비트 필요
	***/

	NORMAL_BLOCK_EMPTY = (0x7), //초기 상태 (Default), 0x7(16) = 7(10) = 111(2)
	NORMAL_BLOCK_VALID = (0x6), //0x6(16) = 6(10) = 110(2)
	NORMAL_BLOCK_INVALID = (0x4), //0x4(16) = 4(10) = 100(2)
	SPARE_BLOCK_EMPTY = (0x3), //0x3(16) = 3(10) = 011(2)
	SPARE_BLOCK_VALID = (0x2), //0x2(16) = 2(10) = 010(2)
	SPARE_BLOCK_INVALID = (0x0) //0x0(16) = 0(10) = 000(2)
};

enum class SECTOR_STATE : const unsigned //섹터(페이지) 상태 정보
{
	/***
	* 모든 섹터(페이지)에서 관리
	* EMPTY : 해당 섹터(페이지)는 비어있고, 유효하다. (초기 상태)
	* VALID : 해당 섹터(페이지)는 유효하고, 유효한 데이터가 기록되어 있다.
	* INVALID : 해당 섹터(페이지)는 무효하고, 무효한 데이터가 기록되어 있다. 재사용 위해서 블록 단위 Erase 수행하여야 함
	***/

	/***
	* IS_VALID || IS_EMPTY
	* 섹터에 대한 3가지 상태, 2^2 = 4, 2비트 필요
	***/

	EMPTY = (0x3), //초기 상태 (Default), 0x3(16) = 3(10) = 11(2)
	VALID = (0x2), //0x2(16) = 2(10) = 10(2)
	INVALID = (0x0) //0x0(16) = 0(10) = 00(2)
};

struct META_DATA
{
	BLOCK_STATE block_state; //블록 상태
	SECTOR_STATE sector_state; //섹터 상태
};


//Flash_read,write와 FTL_read,write간의 계층적인 처리를 위해 외부적으로 생성 및 접근한다.
int SPARE_read(unsigned char* spare_area_pos, META_DATA*& dst_meta_buffer); //읽기
int SPARE_write(unsigned char* spare_area_pos, META_DATA*& src_meta_buffer); //쓰기
void print_meta_info(META_DATA*& src_meta_buffer); //출력
void bitdisp(int c, int start_digits, int end_digits);

int deallocate_single_meta_buffer(META_DATA*& src_meta_buffer)
{
	if (src_meta_buffer != NULL)
	{
		delete src_meta_buffer;
		src_meta_buffer = NULL;

		return SUCCESS;
	}

	return FAIL;
}

int deallocate_block_meta_buffer_array(META_DATA**& src_block_meta_buffer_array)
{
	if (src_block_meta_buffer_array != NULL)
	{
		for (__int8 offset_index = 0; offset_index < BLOCK_PER_SECTOR; offset_index++)
		{
			delete src_block_meta_buffer_array[offset_index];
		}
		delete[] src_block_meta_buffer_array;

		src_block_meta_buffer_array = NULL;

		return SUCCESS;
	}

	return FAIL;
}