#include <stdio.h>
#include <windows.h>

#define SPARE_AREA_BYTE 16
#define SPARE_AREA_BIT 128

#define BLOCK_PER_SECTOR 32
#define SECTOR_PER_BYTE 512 //�� ������ �����Ͱ� ��ϵǴ� ������ ũ�� (byte)
#define SECTOR_INC_SPARE_BYTE 528 //Spare ������ ������ �� ������ ũ�� (byte)

//8��Ʈ ũ���� Spare Area �ʱⰪ ����
#define SPARE_INIT_VALUE (0xff) //0xff(16) = 255(10) = 11111111(2)
#define TRUE_BIT (0x1)
#define FALSE_BIT (0x0)

#define SUCCESS 1
#define COMPLETE 0
#define FAIL -1

/***
	�ʱⰪ ��� 0x1�� �ʱ�ȭ
	��� ���� Erase ���� ���� �� ��� 0x1�� �ʱ�ȭ
	---------
	�� ����(������)�� Spare Area�� ��ü 16byte�� ���� ù 1byte�� ��� �� ����(������)�� ���� ������ ���
	BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
	��Ÿ, Block Address(Logical) Area, ECC Area �� �߰������� ��� ��, META_DATA ����ü, SPARE_read, SPARE_write ����
	---------
	< meta ���� ���� ��� >

	- FTL_write :
	1) �ش� ����(������)�� meta ���� �Ǵ� ����� meta ���� �Ǻ��� ���Ͽ� ���ǵ� Spare Area ó�� �Լ�(Spare_area.h, Spare_area.cpp)�κ��� meta������ �޾ƿ� ó�� ����
	2) ���� ����(������)�� �������� ������ ����� ���Ͽ� meta������ ���� �� ��� �� �����͸� Flash_write�� �����Ͽ� ����
	3) � ��ġ�� ���Ͽ� Overwrite�� ������ ��� �ش� ��ġ(������ �Ǵ� ���)�� ��ȿȭ��Ű�� ���Ͽ� ���ǵ� Spare Areaó�� �Լ��� ���Ͽ� ����

	- Flash_write :
	1) �������� ����(������)�� ���� ������ ��� �� �ش� ����(������)�� Spare Area�� ���� meta ���� ��� ����
	2) ���� ����(������)�� ���� ������ ����� ���ؼ��� meta ���� (�� ����(������) ����)�� �ݵ�� ���� ���Ѿ� ��
	=> �̿� ����, ȣ�� ������ �ش� ����(������)�� ���� �̸� ĳ�õ� meta������ ���� ��� ���� Spare Area�� ���� meta������ �о���̰�,
	�ش� ����(������)�� ���������, ��� ����. ������� ������, �÷��� �޸��� Ư���� ���� Overwrite ����
	3) �̸� ���Ͽ�, ���� ������ FTL_write���� �� ����(������) ���θ� �̸� �����ų ���, Flash_write���� meta ���� �Ǻ��� �������� ���� �ܼ� ��ϸ� ������ �� ������,
	���� ����� ������� ���� ���, ���� �۾��� ���� Overwrite ������ �����ϱ� ���ؼ��� ���� ������ ������ �аų�, ���� ��ĺ��� ������ ó�� ������ ������ �Ѵ�.
	�̿� ����, Common ������ �ܼ�ȭ�� ���Ͽ�, �������� ����(������)������ ����� �߻��ϴ� Flash_write�󿡼��� �� ����(������) ���θ� �����Ѵ�.
***/

typedef enum class META_DATA_UPDATE_STATE : const unsigned //Meta ���� ���� ����
{
	/***
		EX) ��� ������ �����ϴ� 0�� �������� ���� meta ������ VALID_BLOCK, INVALID��� ����
		�ش� ����� ��ȿȭ ��Ŵ���� ���ؼ� VALID_BLOCK => INVALID_BLOCK �Ǿ��µ� 0�� �����¿� Meta ������ ���������μ�, ������ �÷��� �޸� ������ ��ȿȭ�� ���� ī���Ͱ� �����Ǵ� ���� �߻�
		����, �о���� meta ������ ���Ͽ� �� ��� ���¿� ���� ���´� �ʱ⿡ OUT_DATED ���¸� ����, �ش� ���� ���� �� UPDATED ���¸� ���´�.
		������ �÷��� �޸� ������ UPDATED ���¿� ���ؼ��� �����Ѵ�.
	***/
	INIT = (0x0), //�о���̱� �� ����
	OUT_DATED = (0x1), //�о���� �ʱ� ���� (�ش� meta ���� �� ��� �� ������ �÷��� �޸� ���� ���� ���� ����)
	UPDATED = (0x2) //���ŵ� ���� (�ش� meta ���� �� ��� �� ������ �÷��� �޸� ���� ���� �䱸)
}UPDATE_STATE;

enum class BLOCK_STATE : const unsigned //��� ���� ����
{
	/***
		�� ����� ù ��° ����(������)�� Spare ������ �ش� ��� ������ ����
		�Ϲ� ������ ��� Ȥ�� ������ ������ ����� �Ұ����� Spare ��Ͽ� ���Ͽ�,
		- EMPTY : �ش� ����� ��� ���� ��� �����¿� ���� ����ְ�, ��ȿ�ϴ�. (�ʱ� ����)
		- VALID : �ش� ����� ��ȿ�ϰ�, �Ϻ� ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�.
		- INVALID : �ش� ��� ���� ��� �����¿� ���� ��ȿ�ϰų�, �Ϻ� ��ȿ �����͸� �����ϰ� ������ �� �̻� ��� �Ұ���. ���� ���ؼ� ��� ���� Erase �����Ͽ��� ��
	***/

	/***
		BLOCK_TYPE || IS_VALID || IS_EMPTY
		��Ͽ� ���� 6���� ����, 2^3 = 8, 3��Ʈ �ʿ�
	***/

	NORMAL_BLOCK_EMPTY = (0x7), //�ʱ� ���� (Default), 0x7(16) = 7(10) = 111(2)
	NORMAL_BLOCK_VALID = (0x6), //0x6(16) = 6(10) = 110(2)
	NORMAL_BLOCK_INVALID = (0x4), //0x4(16) = 4(10) = 100(2)
	SPARE_BLOCK_EMPTY = (0x3), //0x3(16) = 3(10) = 011(2)
	SPARE_BLOCK_VALID = (0x2), //0x2(16) = 2(10) = 010(2)
	SPARE_BLOCK_INVALID = (0x0) //0x0(16) = 0(10) = 000(2)
};

enum class SECTOR_STATE : const unsigned //����(������) ���� ����
{
	/***
		��� ����(������)���� ����
		- EMPTY : �ش� ����(������)�� ����ְ�, ��ȿ�ϴ�. (�ʱ� ����)
		- VALID : �ش� ����(������)�� ��ȿ�ϰ�, ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�.
		- INVALID : �ش� ����(������)�� ��ȿ�ϰ�, ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�. ���� ���ؼ� ��� ���� Erase �����Ͽ��� ��
	***/

	/***
		IS_VALID || IS_EMPTY
		���Ϳ� ���� 3���� ����, 2^2 = 4, 2��Ʈ �ʿ�
	***/

	EMPTY = (0x3), //�ʱ� ���� (Default), 0x3(16) = 3(10) = 11(2)
	VALID = (0x2), //0x2(16) = 2(10) = 10(2)
	INVALID = (0x0) //0x0(16) = 0(10) = 00(2)
};

class META_DATA //Flash_read,write�� FTL_read,write���� ������ ó���� ���� �ܺ������� ���� �� ����
{
public:
	META_DATA();
	~META_DATA();

	BLOCK_STATE get_block_state();
	SECTOR_STATE get_sector_state();

	//!! ������ ����, Spare Area ó�� �������� ��� �� ���� ���� ���� ���� Ȯ�� �� ������ �÷��� �޸� ���� ����
	UPDATE_STATE get_block_update_state();
	UPDATE_STATE get_sector_update_state();

	void set_block_state(BLOCK_STATE src_block_state); //��� ���� ����
	void set_sector_state(SECTOR_STATE src_sector_state); //���� ���� ����

private:
	BLOCK_STATE block_state; //��� ����
	SECTOR_STATE sector_state; //���� ����
	UPDATE_STATE block_update_state; //��� ���� ���� ����
	UPDATE_STATE sector_update_state; //���� ���� ���� ����
};


/*** Flash_read,write�� FTL_read,write���� �������� ó���� ���� �ܺ������� ���� �� ���� ***/
int SPARE_read(unsigned char* spare_area_pos, META_DATA*& dst_meta_buffer); //�б�
int SPARE_write(unsigned char* spare_area_pos, META_DATA*& src_meta_buffer); //����
void print_meta_info(META_DATA*& src_meta_buffer); //���
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