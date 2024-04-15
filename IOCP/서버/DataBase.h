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

	// DB 초기화
	bool Initialize();
	// DB 연결
	bool Connect();
	// DB 종료
	bool End();

	// 회원가입을 요청하는 함수
	eReturnCode SignUp(std::string id_, std::string pw_, std::string nickname_);

	// 로그인 후 유저닉네임을 받아오는 함수
	std::string SignIn(std::string id_, std::string pw_);

private:

	// ID가 유효한지 판단한다. !! 회원가입에만 쓰인다. 중복확인이 포함되어있다.
	bool CheckIDIsValid(std::string id_);
	// Password가 유효한지 판단한다.
	bool CheckPWIsValid(std::string pw_);
	// Nickname이 유효한지 판단한다.
	bool CheckNicknameIsValid(std::string nickname_);
	// DB인젝션 방지. 특수문자나 SQL예약어를 제거한다.
	bool InjectionCheck(const std::string str_);
	// 원본문자열이 특정 문자열을 가지고 있는지 판단한다.
	bool IsStringContains(std::string orgstr, std::string somestr);
	// 예약어 목록 작성
	void MakeReservedWordList();

	// 예약어 목록 (DB인젝션 방지용)
	std::vector<std::string> reservedWord;

	// ODBC 핸들
	SQLHENV		henv;
	SQLHDBC		hdbc;
	SQLHSTMT	hstmt;

	// Sign In과 Sign Up 간에 상호배제
	std::mutex mLock;
};