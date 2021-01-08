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
	초기값 모두 0x1로 초기화
	블록 단위 Erase 연산 수행 시 모두 0x1로 초기화
	---------
	각 섹터(페이지)의 Spare Area의 전체 16byte에 대해 첫 1byte를 블록 및 섹터(페이지)의 상태 정보로 사용
	BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
	기타, Block Address(Logical) Area, ECC Area 등 추가적으로 사용 시, META_DATA 구조체, SPARE_read, SPARE_write 수정
	---------
	< meta 정보 관리 방법 >

	- FTL_write :
	1) 해당 섹터(페이지)의 meta 정보 또는 블록의 meta 정보 판별을 위하여 정의된 Spare Area 처리 함수(Spare_area.h, Spare_area.cpp)로부터 meta정보를 받아와 처리 수행
	2) 실제 섹터(페이지)에 직접적인 데이터 기록을 위하여 meta정보를 변경 및 기록 할 데이터를 Flash_write에 전달하여 수행
	3) 어떤 위치에 대하여 Overwrite를 수행할 경우 해당 위치(페이지 또는 블록)를 무효화시키기 위하여 정의된 Spare Area처리 함수를 통하여 수행

	- Flash_write :
	1) 직접적인 섹터(페이지)에 대한 데이터 기록 및 해당 섹터(페이지)의 Spare Area에 대한 meta 정보 기록 수행
	2) 실제 섹터(페이지)에 대한 데이터 기록을 위해서는 meta 정보 (빈 섹터(페이지) 여부)도 반드시 변경 시켜야 함
	=> 이에 따라, 호출 시점에 해당 섹터(페이지)에 대해 미리 캐시된 meta정보가 없을 경우 먼저 Spare Area를 통한 meta정보를 읽어들이고,
	해당 섹터(페이지)가 비어있으면, 기록 수행. 비어있지 않으면, 플래시 메모리의 특성에 따른 Overwrite 오류
	3) 이를 위하여, 상위 계층의 FTL_write에서 빈 섹터(페이지) 여부를 미리 변경시킬 경우, Flash_write에서 meta 정보 판별을 수행하지 않은 단순 기록만 수행할 수 있지만,
	매핑 방식을 사용하지 않을 경우, 쓰기 작업에 따른 Overwrite 오류를 검출하기 위해서는 직접 데이터 영역을 읽거나, 매핑 방식별로 별도의 처리 로직을 만들어야 한다.
	이에 따라, Common 로직의 단순화를 위하여, 직접적인 섹터(페이지)단위의 기록이 발생하는 Flash_write상에서만 빈 섹터(페이지) 여부를 변경한다.
***/

typedef enum class META_DATA_UPDATE_STATE : const unsigned //Meta 정보 갱신 상태
{
	/***
		EX) 블록 정보를 관리하는 0번 오프셋의 기존 meta 정보가 VALID_BLOCK, INVALID라고 가정
		해당 블록을 무효화 시킴으로 인해서 VALID_BLOCK => INVALID_BLOCK 되었는데 0번 오프셋에 Meta 정보를 갱신함으로서, 가변적 플래시 메모리 정보인 무효화된 섹터 카운터가 증가되는 문제 발생
		따라서, 읽어들인 meta 정보에 대하여 각 블록 상태와 섹터 상태는 초기에 OUT_DATED 상태를 갖고, 해당 정보 변경 시 UPDATED 상태를 갖는다.
		가변적 플래시 메모리 정보는 UPDATED 상태에 대해서만 갱신한다.
	***/
	INIT = (0x0), //읽어들이기 전 상태
	OUT_DATED = (0x1), //읽어들인 초기 상태 (해당 meta 정보 재 기록 시 가변적 플래시 메모리 정보 갱신 하지 않음)
	UPDATED = (0x2) //갱신된 상태 (해당 meta 정보 재 기록 시 가변적 플래시 메모리 정보 갱신 요구)
}UPDATE_STATE;

enum class BLOCK_STATE : const unsigned //블록 상태 정보
{
	/***
		각 블록의 첫 번째 섹터(페이지)의 Spare 영역에 해당 블록 정보를 관리
		일반 데이터 블록 혹은 직접적 데이터 기록이 불가능한 Spare 블록에 대하여,
		- EMPTY : 해당 블록은 블록 내의 모든 오프셋에 대해 비어있고, 유효하다. (초기 상태)
		- VALID : 해당 블록은 유효하고, 일부 유효한 데이터가 기록되어 있다.
		- INVALID : 해당 블록 내의 모든 오프셋에 대해 무효하거나, 일부 유효 데이터를 포함하고 있지만 더 이상 사용 불가능. 재사용 위해서 블록 단위 Erase 수행하여야 함
	***/

	/***
		BLOCK_TYPE || IS_VALID || IS_EMPTY
		블록에 대한 6가지 상태, 2^3 = 8, 3비트 필요
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
		모든 섹터(페이지)에서 관리
		- EMPTY : 해당 섹터(페이지)는 비어있고, 유효하다. (초기 상태)
		- VALID : 해당 섹터(페이지)는 유효하고, 유효한 데이터가 기록되어 있다.
		- INVALID : 해당 섹터(페이지)는 무효하고, 무효한 데이터가 기록되어 있다. 재사용 위해서 블록 단위 Erase 수행하여야 함
	***/

	/***
		IS_VALID || IS_EMPTY
		섹터에 대한 3가지 상태, 2^2 = 4, 2비트 필요
	***/

	EMPTY = (0x3), //초기 상태 (Default), 0x3(16) = 3(10) = 11(2)
	VALID = (0x2), //0x2(16) = 2(10) = 10(2)
	INVALID = (0x0) //0x0(16) = 0(10) = 00(2)
};

class META_DATA //Flash_read,write와 FTL_read,write간의 계층적 처리를 위해 외부적으로 생성 및 접근
{
public:
	META_DATA();
	~META_DATA();

	BLOCK_STATE get_block_state();
	SECTOR_STATE get_sector_state();

	//!! 물리적 계층, Spare Area 처리 계층에서 블록 및 섹터 정보 갱신 상태 확인 후 가변적 플래시 메모리 정보 갱신
	UPDATE_STATE get_block_update_state();
	UPDATE_STATE get_sector_update_state();

	void set_block_state(BLOCK_STATE src_block_state); //블록 상태 변경
	void set_sector_state(SECTOR_STATE src_sector_state); //섹터 상태 변경

private:
	BLOCK_STATE block_state; //블록 상태
	SECTOR_STATE sector_state; //섹터 상태
	UPDATE_STATE block_update_state; //블록 정보 갱신 상태
	UPDATE_STATE sector_update_state; //섹터 정보 갱신 상태
};


/*** Flash_read,write와 FTL_read,write간의 계층적인 처리를 위해 외부적으로 생성 및 접근 ***/
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