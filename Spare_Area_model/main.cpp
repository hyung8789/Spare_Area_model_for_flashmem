#include "Spare_area.h"

unsigned char sector[SECTOR_INC_SPARE_BYTE] = { NULL, };

/***
	AND : 둘 다 1인 경우메만 1
	OR : 둘 다 0인 경우만 0
	XOR : 둘 다 같은 경우만 0
	~ : 반전
***/

void bitdisp(int c, int start_digits, int end_digits)
{
	printf("bits : ");
	for (int i = start_digits; i >= end_digits; i--)
		printf("%1d", (c) & (0x1 << (i)) ? 1 : 0);
	printf("\n");
}


META_DATA::META_DATA()
{
	this->block_state = BLOCK_STATE::NORMAL_BLOCK_EMPTY;
	this->sector_state = SECTOR_STATE::EMPTY;
	this->this_state = META_DATA_STATE::INVALID; //초기값 : 현재 META DATA 사용 불가능한 무효 상태
}

META_DATA::~META_DATA() {}

int META_DATA::SPARE_read(unsigned char* spare_area_pos)
{
	this->validate_meta_data(); //새로운 META DATA로 갱신 위해 유효 상태로 변경

	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 시작 ***/

	unsigned char bits_8_buffer; //1byte == 8bit크기의 블록 및 섹터(페이지) 정보에 관하여 Spare Area를 읽어들인 버퍼 

	//이하 삭제
	//unsigned block_state_buffer = (0x7); //블록 상태 버퍼, 0x7(16) = 7(10) = 111(2)
	//unsigned sector_state_buffer = (0x3); //섹터 상태 버퍼, 0x3(16) = 3(10) = 11(2)
	//__int8 state_buffer_bit_digits; //블록 상태 버퍼, 섹터 상태 버퍼의 비트 자리 위치 카운터

	//현재 Spare area의 1바이트만 사용하므로 해당 부분만 읽는다
	bits_8_buffer = spare_area_pos[SECTOR_PER_BYTE]; //read Spare area pos

	/*** 읽어들인 8비트(2^7 ~2^0)에 대해서 블록 상태(2^7 ~ 2^5) 판별 ***/
	/* 삭제
	state_buffer_bit_digits = 2; //블록 상태 버퍼의 2^2 자리 위치부터
	for (__int8 bit_digits = 7; bit_digits >= 5; bit_digits--) //블록 상태 정보에 대하여
	{
		switch ((bits_8_buffer) & (TRUE_BIT << (bit_digits)))
		{
		case TRUE_BIT: //이미 0x1이므로 자릿수만 감소
			state_buffer_bit_digits--;
			break;

		case FALSE_BIT: //해당 위치에 ^(exclusive not) 연산
			block_state_buffer ^= 0x1 << state_buffer_bit_digits;
			state_buffer_bit_digits--;
			break;

		default:
			printf("판별실패\n");
			system("pause);
			break;
		}
	}
	*/
	switch ((((bits_8_buffer) >> (5)) & (0x7))) //추출 끝나는 2^5 자리가 LSB에 오도록 오른쪽으로 5번 쉬프트하여, 00000111(2)와 AND 수행
	{
	case (const unsigned)BLOCK_STATE::NORMAL_BLOCK_EMPTY:
		this->block_state = BLOCK_STATE::NORMAL_BLOCK_EMPTY;
		break;

	case (const unsigned)BLOCK_STATE::NORMAL_BLOCK_VALID:
		this->block_state = BLOCK_STATE::NORMAL_BLOCK_VALID;
		break;

	case (const unsigned)BLOCK_STATE::NORMAL_BLOCK_INVALID:
		this->block_state = BLOCK_STATE::NORMAL_BLOCK_INVALID;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_EMPTY:
		this->block_state = BLOCK_STATE::SPARE_BLOCK_EMPTY;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_VALID:
		this->block_state = BLOCK_STATE::SPARE_BLOCK_VALID;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_INVALID:
		this->block_state = BLOCK_STATE::SPARE_BLOCK_INVALID;
		break;

	default:
		printf("Block State ERR\n");
		goto WRONG_META_ERR;
	}
		
	/*** 읽어들인 8비트(2^7 ~2^0)에 대해서 섹터 상태(2^4 ~ 2^3) 판별 ***/
	/* 삭제
	state_buffer_bit_digits = 1; //섹터 상태 버퍼의 2^1 자리 위치부터
	for (__int8 bit_digits = 4; bit_digits >= 3; bit_digits--) //섹터(페이지) 상태 정보에 대하여
	{
		switch ((bits_8_buffer) & (TRUE_BIT << (bit_digits)))
		{
		case TRUE_BIT: //이미 0x1이므로 자릿수만 감소
			state_buffer_bit_digits--;
			break;

		case FALSE_BIT: //해당 위치에 ^(exclusive not) 연산
			sector_state_buffer ^= 0x1 << state_buffer_bit_digits;
			state_buffer_bit_digits--;
			break;		
		
		default:
			printf("판별실패\n");
			system("pause);
		}
	}
	*/
	switch ((((bits_8_buffer) >> (3)) & (0x3))) //추출 끝나는 2^3 자리가 LSB에 오도록 오른쪽으로 3번 쉬프트하여, 00000011(2)와 AND 수행
	{
	case (const unsigned)SECTOR_STATE::EMPTY:
		this->sector_state = SECTOR_STATE::EMPTY;
		break;

	case (const unsigned)SECTOR_STATE::VALID:
		this->sector_state = SECTOR_STATE::VALID;
		break;

	case (const unsigned)SECTOR_STATE::INVALID:
		this->sector_state = SECTOR_STATE::INVALID;
		break;

	default:
		printf("Sector State ERR\n");
		goto WRONG_META_ERR;
	}

	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 종료 ***/

	//기타 Meta 정보 추가 시 읽어서 처리 할 코드 추가

	return SUCCESS;

WRONG_META_ERR: //잘못된 meta정보 오류
	fprintf(stderr, "오류 : 잘못된 meta 정보 (SPARE_read)\n");
	system("pause");
	exit(1);
}

int META_DATA::SPARE_write(unsigned char* spare_area_pos)
{
	this->invalidate_meta_data(); //Spare Area에 기록 후 재사용 불가능 하도록 기존 META DATA 무효화

	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 시작 ***/

	// BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
	unsigned bits_8_buffer = ~(SPARE_INIT_VALUE); //1byte == 8bit크기의 블록 및 섹터(페이지) 정보에 관하여 Spare Area에 기록 할 버퍼, 00000000(2)

	switch (this->block_state) //1바이트 크기의 bits_8_buffer에 대하여
	{
	case BLOCK_STATE::NORMAL_BLOCK_EMPTY: //2^7, 2^6, 2^5 비트를 0x1으로 설정
		bits_8_buffer |= (0x7 << 5); //111(2)를 5번 왼쪽 쉬프트하여 11100000(2)를 OR 수행
		break;

	case BLOCK_STATE::NORMAL_BLOCK_VALID: //2^7, 2^6 비트를 0x1로 설정
		bits_8_buffer |= (0x6 << 5); //110(2)를 5번 왼쪽 쉬프트하여 11000000(2)를 OR 수행
		break;

	case BLOCK_STATE::NORMAL_BLOCK_INVALID: //2^7 비트를 0x1로 설정
		bits_8_buffer |= (TRUE_BIT << 7); //10000000(2)를 OR 수행
		break;

	case BLOCK_STATE::SPARE_BLOCK_EMPTY: //2^6, 2^5 비트를 0x1로 설정
		bits_8_buffer |= (0x6 << 4); //110(2)를 4번 왼쪽 쉬프트하여 01100000(2)를 OR 수행
		break;

	case BLOCK_STATE::SPARE_BLOCK_VALID: //2^6 비트를 0x1로 설정
		bits_8_buffer |= (TRUE_BIT << 6); //01000000(2)를 OR 수행
		break;

	case BLOCK_STATE::SPARE_BLOCK_INVALID: //do nothing
		break;

	default:
		goto WRONG_META_ERR;
	}

	switch (this->sector_state) //1바이트 크기의 bits_8_buffer에 대하여
	{
	case SECTOR_STATE::EMPTY: //2^4, 2^3 비트를 0x1로 설정
		bits_8_buffer |= (0x3 << 4); //11(2)를 3번 왼쪽 쉬프트하여 00011000(2)를 OR 수행
		break;

	case SECTOR_STATE::VALID: //2^4비트를 0x1로 설정
		bits_8_buffer |= (TRUE_BIT << 4); //00010000(2)를 OR 수행
		break;

	case SECTOR_STATE::INVALID: //do nothing
		break;

	default:
		goto WRONG_META_ERR;
	}
	
	//DUMMY 3비트 처리 (2^2 ~ 2^0)
	bits_8_buffer |= (0x7); //00000111(2)를 OR 수행

	spare_area_pos[SECTOR_PER_BYTE] = bits_8_buffer;
	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 종료 ***/

	//기타 Meta 정보 추가 시 기록 할 코드 추가

	return SUCCESS;

WRONG_META_ERR: //잘못된 meta정보 오류
	fprintf(stderr, "오류 : 잘못된 meta 정보 (SPARE_write)\n");
	system("pause");
	exit(1);

}

void META_DATA::print_meta_info()
{
	printf("Block State : ");

	switch (this->block_state)
	{
	case BLOCK_STATE::NORMAL_BLOCK_EMPTY:
		printf("NORMAL_BLOCK_EMPTY\n");
		break;

	case BLOCK_STATE::NORMAL_BLOCK_VALID:
		printf("NORMAL_BLOCK_VALID\n");
		break;

	case BLOCK_STATE::NORMAL_BLOCK_INVALID:
		printf("NORMAL_BLOCK_INVALID\n");
		break;

	case BLOCK_STATE::SPARE_BLOCK_EMPTY:
		printf("SPARE_BLOCK_EMPTY\n");
		break;

	case BLOCK_STATE::SPARE_BLOCK_VALID:
		printf("SPARE_BLOCK_VALID\n");
		break;

	case BLOCK_STATE::SPARE_BLOCK_INVALID:
		printf("SPARE_BLOCK_INVALID\n");
		break;

	}

	printf("Sector State : ");
	switch (this->sector_state)
	{
	case SECTOR_STATE::EMPTY:
		printf("EMPTY\n");
		break;

	case SECTOR_STATE::VALID:
		printf("VALID\n");
		break;

	case SECTOR_STATE::INVALID:
		printf("INVALID\n");
		break;
	}
}

META_DATA_STATE& META_DATA::get_current_state() //현재 META_DATA의 상태 정보 반환
{
	return this->this_state;
}

void META_DATA::validate_meta_data() //새로운 META DATA로 갱신 위해 유효 상태로 변경
{
	switch (this->this_state)
	{
	case META_DATA_STATE::INVALID:
		//기존에 Spare Area에 기록 위하여 사용되었던 Meta 정보에 대하여 Spare Area에 기록하기 위해 다시 접근하였으므로 오류
		this->this_state = META_DATA_STATE::VALID; //유효 상태로 변경
		break;

	case META_DATA_STATE::VALID: //이미 유효 할 경우
		//기존의 Meta 정보에 대하여 처리가 되지 않았으므로 오류
		fprintf(stderr, "오류 : Already valid (validate_meta_data)");
		system("pause");
		exit(1);
		break;

	case META_DATA_STATE::READ_ONLY:
		//do nothing
		break;
	}
}

void META_DATA::invalidate_meta_data() //Spare Area에 기록 후 재사용 불가능 하도록 기존 META DATA 무효화
{
	switch (this->this_state)
	{
	case META_DATA_STATE::INVALID: //이미 무효할 경우
		//기존에 Spare Area에 기록 위하여 사용되었던 Meta 정보에 대하여 Spare Area에 기록하기 위해 다시 접근하였으므로 오류
		fprintf(stderr, "오류 : Already invalid (invalidate_meta_data)");
		system("pause");
		exit(1);
		break;

	case META_DATA_STATE::VALID:
		this->this_state = META_DATA_STATE::INVALID; //무효 상태로 변경
		break;

	case META_DATA_STATE::READ_ONLY:
		fprintf(stderr, "오류 : read only state에 대한 쓰기 접근 발생 (invalidate_meta_data)\n");
		system("pause");
		exit(1);
		break;
	}
}


void main()
{
	META_DATA meta_data;
	
	//init
	for (int byte_unit = SECTOR_PER_BYTE; byte_unit < SECTOR_INC_SPARE_BYTE; byte_unit++) //섹터 내(0~527)의 512 ~ 527 까지 Spare Area에 대해 할당
	{
		sector[byte_unit] = SPARE_INIT_VALUE; //0xff(16) = 11111111(2) = 255(10) 로 초기화
	}

	/***
		AND : 둘 다 1인 경우메만 1
		OR : 둘 다 0인 경우만 0
		XOR : 둘 다 같은 경우만 0
		~ : 반전
	***/

	bitdisp(sector[512], 7, 0);
	//sector[512] &= ((0xff) << (8)); //전체 비트 반전

	//읽기 테스트
	printf("읽기 테스트\n");
	meta_data.SPARE_read(sector);
	meta_data.print_meta_info();

	//쓰기 테스트
	/***
	* 1 : SPARE read로 meta 정보 판독
	* 2 : meta 정보 변경
	* 3 : SPARE write로 Spare Area에 기록
	***/
	
	meta_data.debug_invalidate_meta_data(); //쓰기 테스트 전 기존 데이터 무효화

	printf("\n쓰기 테스트\n");
	meta_data.SPARE_read(sector);

	printf("--change state 1--\n");
	meta_data.sector_state = SECTOR_STATE::VALID;
	meta_data.block_state = BLOCK_STATE::SPARE_BLOCK_INVALID;
	meta_data.print_meta_info();

	printf("\nSpare_write 수행\n");
	meta_data.SPARE_write(sector);
	bitdisp(sector[512], 7, 0);
	
	printf("\n재판독 수행\n");
	meta_data.SPARE_read(sector);
	meta_data.print_meta_info();

	printf("\n--change state 2--\n");
	meta_data.sector_state = SECTOR_STATE::INVALID;
	meta_data.block_state = BLOCK_STATE::NORMAL_BLOCK_INVALID;
	meta_data.print_meta_info();

	printf("\nSpare_write 수행\n");
	meta_data.SPARE_write(sector);
	bitdisp(sector[512], 7, 0);

	printf("\n재판독 수행\n");
	meta_data.SPARE_read(sector);
	meta_data.print_meta_info();

	/*
	//Meta Data에 대한 유효성 테스트
	printf("유효성 테스트 : Already valid (validate_meta_data) 발생하여야 함\n");
	meta_data.SPARE_read(sector);
	
	*/
	switch (meta_data.get_current_state())
	{
	case META_DATA_STATE::INVALID:
		printf("invalid\n");
		break;

	case META_DATA_STATE::VALID:
		printf("valid\n");
		break;

	case META_DATA_STATE::READ_ONLY:
		printf("read_only\n");
		break;
	
	default:
		printf("err\n");
		break;
	}
	system("pause");
}