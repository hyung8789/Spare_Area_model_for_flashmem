#include "Spare_area.h"

unsigned char sector[SECTOR_INC_SPARE_BYTE] = { NULL, };

/***
	AND : 둘 다 1인 경우메만 1
	OR : 둘 다 0인 경우만 0
	XOR : 둘 다 같은 경우만 0
	~ : 반전
***/

void bitdisp(int c, int start_digits, int end_digits) {
	printf("bits : ");
	for (int i = start_digits; i >= end_digits; i--)
		printf("%1d", (c) & (0x1 << (i)) ? 1 : 0);
	printf("\n");
}

int SPARE_read(unsigned char* spare_area_pos, META_DATA*& dst_meta_buffer)
{
	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 시작 ***/

	unsigned char bits_8_buffer; //1byte == 8bit크기의 블록 및 섹터(페이지) 정보에 관하여 Spare Area를 읽어들인 버퍼 

	//이하 삭제
	//unsigned block_state_buffer = (0x7); //블록 상태 버퍼, 0x7(16) = 7(10) = 111(2)
	//unsigned sector_state_buffer = (0x3); //섹터 상태 버퍼, 0x3(16) = 3(10) = 11(2)
	//__int8 state_buffer_bit_digits; //블록 상태 버퍼, 섹터 상태 버퍼의 비트 자리 위치 카운터

	//현재 Spare area의 1바이트만 사용하므로 해당 부분만 읽는다
	bits_8_buffer = spare_area_pos[SECTOR_PER_BYTE]; //read Spare area pos

	if (dst_meta_buffer == NULL)
		dst_meta_buffer = new META_DATA;
	else
		goto MEM_LEAK_ERR;


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
		dst_meta_buffer->block_state = BLOCK_STATE::NORMAL_BLOCK_EMPTY;
		break;

	case (const unsigned)BLOCK_STATE::NORMAL_BLOCK_VALID:
		dst_meta_buffer->block_state = BLOCK_STATE::NORMAL_BLOCK_VALID;
		break;

	case (const unsigned)BLOCK_STATE::NORMAL_BLOCK_INVALID:
		dst_meta_buffer->block_state = BLOCK_STATE::NORMAL_BLOCK_INVALID;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_EMPTY:
		dst_meta_buffer->block_state = BLOCK_STATE::SPARE_BLOCK_EMPTY;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_VALID:
		dst_meta_buffer->block_state = BLOCK_STATE::SPARE_BLOCK_VALID;
		break;

	case (const unsigned)BLOCK_STATE::SPARE_BLOCK_INVALID:
		dst_meta_buffer->block_state = BLOCK_STATE::SPARE_BLOCK_INVALID;
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
		dst_meta_buffer->sector_state = SECTOR_STATE::EMPTY;
		break;

	case (const unsigned)SECTOR_STATE::VALID:
		dst_meta_buffer->sector_state = SECTOR_STATE::VALID;
		break;

	case (const unsigned)SECTOR_STATE::INVALID:
		dst_meta_buffer->sector_state = SECTOR_STATE::INVALID;
		break;

	default:
		printf("Sector State ERR\n");
		goto WRONG_META_ERR;
	}

	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 종료 ***/

	// 기타 Meta 정보 추가 시 읽어서 처리 할 코드 추가

	return SUCCESS;

WRONG_META_ERR: //잘못된 meta정보 오류
	fprintf(stderr, "오류 : 잘못된 meta 정보 (SPARE_read)\n");
	system("pause");
	exit(1);

MEM_LEAK_ERR:
	fprintf(stderr, "오류 : meta 정보에 대한 메모리 누수 발생 (SPARE_read)\n");
	system("pause");
	exit(1);

}

int SPARE_write(unsigned char* spare_area_pos, META_DATA*& src_meta_buffer)
{
	if (src_meta_buffer == NULL)
		return FAIL;

	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 시작 ***/

	// BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
	unsigned bits_8_buffer = ~(SPARE_INIT_VALUE); //1byte == 8bit크기의 블록 및 섹터(페이지) 정보에 관하여 Spare Area에 기록 할 버퍼, 00000000(2)

	switch (src_meta_buffer->block_state) //1바이트 크기의 bits_8_buffer에 대하여
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

	switch (src_meta_buffer->sector_state) //1바이트 크기의 bits_8_buffer에 대하여
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
	
	// DUMMY 3비트 처리 (2^2 ~ 2^0)
	bits_8_buffer |= (0x7); //00000111(2)를 OR 수행

	spare_area_pos[SECTOR_PER_BYTE] = bits_8_buffer;
	/*** Spare Area의 전체 16byte에 대해 첫 1byte의 블록 및 섹터(페이지)의 상태 정보에 대한 처리 종료 ***/

	// 기타 Meta 정보 추가 시 기록 할 코드 추가

	return SUCCESS;

WRONG_META_ERR: //잘못된 meta정보 오류
	fprintf(stderr, "오류 : 잘못된 meta 정보 (SPARE_write)\n");
	system("pause");
	exit(1);

}

void print_meta_info(META_DATA*& src_data)
{
	printf("Block State : ");

	switch (src_data->block_state)
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
	switch (src_data->sector_state)
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

void main()
{
	META_DATA* meta_data = NULL;
	
	system("pause"); //snapshot pos
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

	//읽기 테스트
	printf("읽기 테스트\n");
	SPARE_read(sector, meta_data);
	print_meta_info(meta_data);
	delete meta_data;
	meta_data = NULL;

	sector[512] &= ((0xff) << (8)); //전체 비트 반전
	bitdisp(sector[512], 7,0);

	SPARE_read(sector, meta_data);
	print_meta_info(meta_data);
	delete meta_data;
	meta_data = NULL;

	sector[512] = SPARE_INIT_VALUE; //초기화
	bitdisp(sector[512], 7,0);

	//쓰기 테스트
	printf("\n쓰기 테스트\n");
	SPARE_read(sector, meta_data);

	printf("--change state 1--\n");
	meta_data->sector_state = SECTOR_STATE::VALID;
	meta_data->block_state = BLOCK_STATE::SPARE_BLOCK_INVALID;
	print_meta_info(meta_data);

	printf("\nSpare_write 수행\n");
	SPARE_write(sector, meta_data);
	
	delete meta_data;
	meta_data = NULL;
	
	SPARE_read(sector, meta_data);
	print_meta_info(meta_data);

	printf("--change state 2--\n");
	meta_data->sector_state = SECTOR_STATE::INVALID;
	meta_data->block_state = BLOCK_STATE::NORMAL_BLOCK_INVALID;
	print_meta_info(meta_data);

	printf("\nSpare_write 수행\n");
	SPARE_write(sector, meta_data);

	delete meta_data;
	meta_data = NULL;

	SPARE_read(sector, meta_data);
	print_meta_info(meta_data);

	for (int i = 0; i <= 100000; i++) //memleak test
	{
		delete meta_data;
		meta_data = NULL;
		SPARE_read(sector, meta_data);
	}
	delete meta_data;
	meta_data = NULL;

	system("pause");
}