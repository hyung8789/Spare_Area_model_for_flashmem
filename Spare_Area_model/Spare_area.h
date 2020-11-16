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
* �ʱⰪ ��� 0x1�� �ʱ�ȭ
* ��� ���� Erase ���� ���� �� ��� 0x1�� �ʱ�ȭ
 ---------
* �� ����(������)�� Spare Area�� ��ü 16byte�� ���� ù 1byte�� ��� �� ����(������)�� ���� ������ ���
* BLOCK_TYPE(Normal or Spare, 1bit) || IS_VALID (BLOCK, 1bit) || IS_EMPTY (BLOCK, 1bit) || IS_VALID (SECTOR, 1bit) || IS_EMPTY(SECTOR, 1bit) || DUMMY (3bit)
* ��Ÿ, Block Address(Logical) Area, ECC Area �� �߰������� ��� ��, META_DATA ����ü, SPARE_read, SPARE_write ����
***/

enum class BLOCK_STATE : const unsigned //��� ���� ����
{
	/***
	* �� ����� ù ��° ����(������)�� Spare ������ �ش� ��� ������ ����
	* �Ϲ� ������ ��� Ȥ�� ������ ������ ����� �Ұ����� Spare ��Ͽ� ���Ͽ�,
	* EMPTY : �ش� ����� ��� ���� ��� �����¿� ���� ����ְ�, ��ȿ�ϴ�. (�ʱ� ����)
	* VALID : �ش� ����� ��ȿ�ϰ�, �Ϻ� ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�.
	* INVALID : �ش� ��� ���� ��� �����¿� ���� ��ȿ�ϰų�, �Ϻ� ��ȿ �����͸� �����ϰ� ������ �� �̻� ��� �Ұ���. ���� ���ؼ� ��� ���� Erase �����Ͽ��� ��
	***/

	/***
	* BLOCK_TYPE || IS_VALID || IS_EMPTY
	* ��Ͽ� ���� 6���� ����, 2^3 = 8, 3��Ʈ �ʿ�
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
	* ��� ����(������)���� ����
	* EMPTY : �ش� ����(������)�� ����ְ�, ��ȿ�ϴ�. (�ʱ� ����)
	* VALID : �ش� ����(������)�� ��ȿ�ϰ�, ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�.
	* INVALID : �ش� ����(������)�� ��ȿ�ϰ�, ��ȿ�� �����Ͱ� ��ϵǾ� �ִ�. ���� ���ؼ� ��� ���� Erase �����Ͽ��� ��
	***/

	/***
	* IS_VALID || IS_EMPTY
	* ���Ϳ� ���� 3���� ����, 2^2 = 4, 2��Ʈ �ʿ�
	***/

	EMPTY = (0x3), //�ʱ� ���� (Default), 0x3(16) = 3(10) = 11(2)
	VALID = (0x2), //0x2(16) = 2(10) = 10(2)
	INVALID = (0x0) //0x0(16) = 0(10) = 00(2)
};

enum META_DATA_STATE //META_DATA�� ���� ����
{ 
	VALID, //���� Meta ���� ���� ���� ��� ����
	INVALID, //���� Meta ���� ���� ���� ��� �Ұ��� (Default)
	READ_ONLY //�б� ���� ����, ���� �Ұ��� (�� ������, �� ��� Ž�� Ȥ�� ������ �б� ���� Spare Area �ǵ� �� ���)
};

class META_DATA
{
public:
	META_DATA();
	~META_DATA();
	
	BLOCK_STATE block_state; //��� ����
	SECTOR_STATE sector_state; //���� ����

	int SPARE_read(unsigned char* spare_area_pos); //�б�
	int SPARE_write(unsigned char* spare_area_pos); //����
	void print_meta_info(); //���
	META_DATA_STATE& get_current_state(); //���� META_DATA�� ���� ���� ��ȯ


	/*
	ftl write���� ��� ������ ���� �� META_DATA�� ���� ��ȿ�� �˻� ����
	����, ��ȿ�� ������ ��� �ش� Meta ������ ���� ó���� �߻����� �ʾ����Ƿ� ����
	
	spare read �� ��ȿ ���·� ����=>spare write�� �߻����� �����Ƿ� �̹� ��ȿ����=>�����߻�=>read_only ���� �̿�
	read_only state�� ��� ����?

	�߰� ���� : read_only state�� ��������
	*/



	//����׿� ���� ����
	//META DATA ���� �� ��ȿȭ�� META DATA ������ Spare Area ó�� �Լ��� ���ؼ��� ����
	void debug_validate_meta_data()
	{
		this->validate_meta_data();
	}
	void debug_invalidate_meta_data()
	{
		this->invalidate_meta_data();
	}
	
private:
	void validate_meta_data(); //���ο� META DATA�� ���� ���� ��ȿ ���·� ����
	void invalidate_meta_data(); //Spare Area�� ��� �� ���� �Ұ��� �ϵ��� ���� META DATA ��ȿȭ

	META_DATA_STATE this_state; //���� META_DATA�� ���� ����
};

void bitdisp(int c, int start_digits, int end_digits);