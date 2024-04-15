#pragma once
#include "Define.h"

#include <Windows.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include <mutex>
#include <vector>


class DataBase
{
public:
	DataBase();
	~DataBase();

	// DB �ʱ�ȭ
	bool Initialize();
	// DB ����
	bool Connect();
	// DB ����
	bool End();

	// ȸ�������� ��û�ϴ� �Լ�
	eReturnCode SignUp(std::string id_, std::string pw_, std::string nickname_);

	// �α��� �� �����г����� �޾ƿ��� �Լ�
	std::string SignIn(std::string id_, std::string pw_);

private:

	// ID�� ��ȿ���� �Ǵ��Ѵ�. !! ȸ�����Կ��� ���δ�. �ߺ�Ȯ���� ���ԵǾ��ִ�.
	bool CheckIDIsValid(std::string id_);
	// Password�� ��ȿ���� �Ǵ��Ѵ�.
	bool CheckPWIsValid(std::string pw_);
	// Nickname�� ��ȿ���� �Ǵ��Ѵ�.
	bool CheckNicknameIsValid(std::string nickname_);
	// DB������ ����. Ư�����ڳ� SQL���� �����Ѵ�.
	bool InjectionCheck(const std::string str_);
	// �������ڿ��� Ư�� ���ڿ��� ������ �ִ��� �Ǵ��Ѵ�.
	bool IsStringContains(std::string orgstr, std::string somestr);
	// ����� ��� �ۼ�
	void MakeReservedWordList();

	// ����� ��� (DB������ ������)
	std::vector<std::string> reservedWord;

	// ODBC �ڵ�
	SQLHENV		henv;
	SQLHDBC		hdbc;
	SQLHSTMT	hstmt;

	// Sign In�� Sign Up ���� ��ȣ����
	std::mutex mLock;
};