#include "DataBase.h"

DataBase::DataBase()
{
	Initialize();
}

DataBase::~DataBase()
{
	End();
}

bool DataBase::End()
{
	SQLCancel(hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);

	return true;
}

bool DataBase::Initialize()
{
	// 예약어 목록 작성
	MakeReservedWordList();


	SQLRETURN retcode;

	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		//errorDisplay(retcode);
		return false;
	}

	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		//errorDisplay(retcode);
		return false;
	}

	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		//errorDisplay(retcode);
		return false;
	}

	Connect();

	return true;
}

bool DataBase::Connect()
{
	SQLRETURN retcode;
	std::wstring odbc = L"TestDB";
	std::wstring id = L"KIM";
	std::wstring pwd = L"kim123";

	SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

	retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		//errorDisplay(retcode);
		return false;
	}

	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	return true;
}

eReturnCode DataBase::SignUp(std::string id_, std::string pw_, std::string nickname_)
{
	if (!CheckPWIsValid(pw_))
	{
		return eReturnCode::SIGNUP_INVALID_PW;
	}

	if (!CheckNicknameIsValid(nickname_))
	{
		return eReturnCode::SIGNUP_INVALID_NICK;
	}

	std::lock_guard<std::mutex> guard(mLock);

	if (!CheckIDIsValid(id_))
	{
		return eReturnCode::SIGNUP_INVALID_ID;
	}

	// INSERT 쿼리문 작성
	std::wstring query = L"INSERT INTO USERDATA (ID, Password, Nickname) VALUES (?, ?, ?)";
	
	SQLPrepare(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, (SQLPOINTER)id_.c_str(), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, (SQLPOINTER)pw_.c_str(), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 255, 0, (SQLPOINTER)nickname_.c_str(), 0, NULL);

	SQLExecute(hstmt);

	return eReturnCode::SIGNUP_SUCCESS;
}

bool DataBase::CheckIDIsValid(std::string id_)
{
	if (!InjectionCheck(id_))
	{
		return false;
	}

	if (id_.size() < 6 || id_.size() > 10)
	{
		return false;
	}


	// ----- 이후 중복체크 -----


	SQLLEN cbParam1 = SQL_NTS;

	// SELECT 쿼리문 작성
	std::wstring query = L"SELECT ID FROM USERDATA WHERE ID = ?";

	// SELECT 후 데이터를 저장할 변수
	int empno = 0;

	SQLLEN dbEmpno = 0;

	// SQLBindParameter( ) : DB에 넘겨줄 파라미터를 연결한다.
	// 두번째 인자 : 파라미터 순서
	SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, id_.length(), 0, (char*)id_.c_str(), 0, &cbParam1);
	// 쿼리문을 실행
	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);

	// 실행후 결과가 NO_DATA로 나오는지 확인 필요

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLBindCol(hstmt, 1, SQL_C_LONG, &empno, 100, &dbEmpno);

		retcode = SQLFetch(hstmt);

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			SQLCloseCursor(hstmt);
			return false;
		}

		else if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
		{
			//errorDisplay(retcode);
		}

		else if (retcode == SQL_NO_DATA)
		{
			SQLCloseCursor(hstmt);
			return true;
		}
	}
	else if (retcode == SQL_NO_DATA)
	{
		SQLCloseCursor(hstmt);
		return true;
	}
	else
	{
		//errorDisplay(retcode);
	}

	SQLCloseCursor(hstmt);
	return false;
}

bool DataBase::CheckPWIsValid(std::string pw_)
{
	if (!InjectionCheck(pw_))
	{
		return false;
	}

	if (pw_.size() < 6 || pw_.size() > 10)
	{
		return false;
	}

	return true;
}

bool DataBase::CheckNicknameIsValid(std::string nickname_)
{
	if (!InjectionCheck(nickname_))
	{
		return false;
	}

	if (nickname_.size() < 2 || nickname_.size() > 10)
	{
		return false;
	}

	return true;
}

bool DataBase::InjectionCheck(const std::string str_)
{
	// 특수기호 제거
	for (int i = 0; i < str_.size(); i++)
	{
		if (str_[i] >= '0' && str_[i] <= '9')
		{
			continue;
		}
		
		if (str_[i] >= 'A' && str_[i] <= 'Z')
		{
			continue;
		}

		if (str_[i] >= 'a' && str_[i] <= 'z')
		{
			continue;
		}

		return false;
	}

	// 예약어 제거
	for (std::string word : reservedWord)
	{
		if (IsStringContains(str_, word))
		{
			return false;
		}
	}

	return true;
}

std::string DataBase::SignIn(std::string id_, std::string pw_)
{
	if (!InjectionCheck(id_))
	{
		return std::string();
	}

	if (!InjectionCheck(pw_))
	{
		return std::string();
	}

	if (id_.size() < 6 || id_.size() > 10)
	{
		return std::string();
	}

	if (pw_.size() < 6 || pw_.size() > 10)
	{
		return std::string();
	}

	std::lock_guard<std::mutex> guard(mLock);

	SQLLEN cbParam1 = SQL_NTS;

	// SELECT 쿼리문 작성
	std::wstring query = L"SELECT * FROM USERDATA WHERE ID = ?";

	// SELECT 후 데이터를 저장할 변수
	constexpr int size = 15;
	SQLCHAR nickName[size], password[size];
	ZeroMemory(nickName, size);
	ZeroMemory(password, size);

	SQLLEN cbNickName = 0, cbPassword = 0;

	// SQLBindParameter( ) : DB에 넘겨줄 파라미터를 연결한다.
	// 두번째 인자 : 파라미터 순서
	SQLRETURN retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, id_.length(), 0, (char*)id_.c_str(), 0, &cbParam1);
	// 쿼리문을 실행
	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLBindCol(hstmt, 2, SQL_C_CHAR, &password, size, &cbPassword);
		SQLBindCol(hstmt, 3, SQL_C_CHAR, &nickName, size, &cbNickName);

		while (true)
		{
			retcode = SQLFetch(hstmt);

			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				for (int i = 0; i < cbPassword; i++)
				{
					if (password[i] == ' ')
					{
						password[i] = NULL;
						break;
					}
				}
				password[cbPassword] = NULL;
				for (int i = 0; i < cbNickName; i++)
				{
					if (nickName[i] == ' ')
					{
						nickName[i] = NULL;
						break;
					}
				}
				nickName[cbNickName] = NULL;

				if (!pw_.compare(std::string((const char*)password)))
				{
					return std::string((const char*)nickName);
				}
				else
				{
					return std::string("[INFAIL]");
				}
			}

			else if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
			{
				//errorDisplay(retcode);
				break;
			}

			else if (retcode == SQL_NO_DATA)
			{
				//errorDisplay(retcode);
				SQLCloseCursor(hstmt);
				return std::string("[INFAIL]");
			}

		}
	}
	else
	{
		//errorDisplay(retcode);
	}

	SQLCloseCursor(hstmt);
	return std::string();
}

bool DataBase::IsStringContains(std::string orgstr, std::string somestr)
{
	size_t strsize1 = orgstr.size(), strsize2 = somestr.size();

	for (size_t i = 0; i < strsize1; i++)
	{
		// 더 이상 찾을 수 없음
		if (i + strsize2 > strsize1)
		{
			return false;
		}

		if (orgstr[i] == somestr[0])
		{
			// 같은 문자열인가
			bool check = true;

			for (size_t j = 1; j < strsize2; j++)
			{
				// 같은 문자열이 아님
				if (orgstr[i + j] != somestr[j])
				{
					check = false;
					break;
				}
			}

			if (check)
			{
				return true;
			}
		}
	}

	return false;
}

void DataBase::MakeReservedWordList()
{
	reservedWord.clear();

	// 예약어 : OR, SELECT, INSERT, DELETE, UPDATE, CREATE, DROP, EXEC, UNION, FETCH, DECLARE, TRUNCATE
	reservedWord.push_back("OR");
	reservedWord.push_back("SELECT");
	reservedWord.push_back("INSERT");
	reservedWord.push_back("DELETE");
	reservedWord.push_back("UPDATE");
	reservedWord.push_back("CREATE");
	reservedWord.push_back("DROP");
	reservedWord.push_back("EXEC");
	reservedWord.push_back("UNION");
	reservedWord.push_back("FETCH");
	reservedWord.push_back("DECLARE");
	reservedWord.push_back("TRUNCATE");

	return;
}