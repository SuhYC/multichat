# multichat
cpp project <br/>
[기존 Blocking Socket 버전](https://github.com/SuhYC/multichat/tree/main/BlockingSocket)을 IOCP로 다시 만들었다.<br/>

# 요약
TCP <br/>
IOCP <br/>
MultiThread Programming <br/>
MSSQL - SQLServer - odbc <br/>
DB Injection Check

# 새로 얻은 경험
## 1. IOCP
- 기존 Blocking Socket 버전의 경우 1개의 클라이언트에 연결할 때마다 서버는 1개의 쓰레드를 생성해야 했다. 이 경우 연결된 클라이언트가 많아지면 쓰레드의 수가 많아지며 이는 동기화 비용이 증가하는 결과로 이어진다.
- 이를 IOCP에서는 Send와 Recv를 요청해두고 비동기로 처리하며 완료한IO에 대한 결과를 GetQueuedCompletionPort()함수로 받아 처리하며 1:1연결이 아닌 M:N연결이 가능해져 과도한 쓰레드 생성을 막을 수 있다.
- IOCP에서는 송수신에 대한 처리가 예약으로 이루어지기 때문에 어떤 데이터에 대한 WSASend요청 직후 데이터의 메모리를 해제해버리면 문제가 발생한다. WSASend는 송신요청이지 송신처리가 아니다.
- WSA_IO_PENDING : 에러코드로 반환되긴 하지만 치명적인 에러가 아니다. Overlapped Operation은 성공적으로 시작했지만 완료되지 않은 것으로 WOULDBLOCK과의 차이는 전자는 완료되지 않았을 뿐 정상처리될 것이고, 후자는 소켓버퍼가 이미 가득찼다던지 하는 이유로 전송에 실패한 것이다.
- 1Send : 비동기 입출력의 경우 요청한 순서대로 처리되지 않을 수 있다. (TCP의 Segment 순서보장과는 다르다. 하나의 메시지 내에서 분할이 이루어진건 순서가 보장된다.) 이를 방지하기 위해 각 연결마다 한번의 송신이 완료되면 다음 데이터를 송신하도록 코드를 구성했다.
- AcceptEx, ConnectEx : 초기 windows api에는 Send와 Recv만 비동기로 제공되었다. AcceptEx와 ConnectEx는 이후 추가된 내용으로 각각 함수포인터를 이용해 불러온 후 실행하여야 한다. (최신버전에서는 AcceptEx는 직접 호출해도 상관 없다고 한다.)
## 2. Thread
- 쓰레드는 자기 자신을 join할 수 없다. (굉장히 부끄러운 배움이었다..) 각 쓰레드가 join으로 메인쓰레드와 합류하는 과정에 혹시 해당 쓰레드가 호출하지는 않는지 확인하자.
## 3. Winsock.h
- Windows.h 헤더는 소켓통신과 같이 사용하려고 하면 에러가 발생할 수 있다. 구버전의 winsock이 windows헤더에 포함되어 있다. 반드시 winsock.h헤더 선언 이후에 windows.h헤더를 선언하자. (이 순서로 호출하면 #ifndef문에 의해 중복으로 선언되는 것을 막을 수 있다.)
## 4. DB Injection
- DB Injection에 사용되는 방법은 "와 같은 특수문자로 쿼리를 비트는 방법 외에도 SQL 예약어를 사용해서 시도하는 방법도 있다.
## 5. smart_pointer
- 원시포인터로 패킷을 구성하면 session 하나당 하나의 패킷이 구성되어야 하지만 RefCount를 이용해 하나의 패킷으로 여러개의 client에 메시지를 보낼 수 있다. 해당 패킷이 사용되는 마지막 메시지에서 패킷을 정리한다.
- ~deleter를 사용하여 복잡한 구조의 포인터를 해제하거나, 배열포인터를 해제할 수 있다. 해당 코드 내에선 stOverlappedEx*->m_wsaBuf.buf를 해제하고 stOverlappedEx포인터를 해제하는 방식으로 작성.~
- 스마트포인터의 오버헤드 문제로 원시포인터로 복귀. 다만 브로드캐스팅을 위해 패킷은 refCount를 활용하는 방식으로 재구성. atomic을 활용해 동기화비용을 줄임.
# 문서
### 유즈케이스 다이어그램
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/usecase.png" width="500"><br/>


### 클래스 다이어그램
#### Server <br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/ClassServerDefine.png" width="500"><br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/ClassServerNetwork.png" width="500"><br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/ClassServerRoom.png" width="500"><br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/ClassServerDB.png" width="500"><br/>

#### Client <br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/ClassClient.png" width="500"><br/>

### 시퀀스 다이어그램
#### 1. Sign <br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqSign.png" width="500"><br/>

#### 2. EnterRoom<br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqEnterRoom.png" width="500"><br/>

#### 3. Chat<br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqChat.png" width="500"><br/>

#### 4. Run<br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqRun1.png" width="500"><br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqRun2.png" width="500"><br/>

#### 5. End<br/>
<img src="https://github.com/SuhYC/multichat/blob/main/IOCP/image/SeqEnd.png" width="500"><br/>

# 기능 <br/>
### 초기상태 : 로그인 화면 (SignInPage)
1. /n 또는 /N을 입력하여 회원가입화면으로 전환할 수 있음. <br/>
2. ID와 Password를 입력하여 서버에 로그인 요청을 할 수 있음.
- DB에 저장된 ID와 Password와 일치할 경우 로그인 성공 메시지가 반환된다. 이후 로비화면으로 진입한다.
- 일치하는 ID가 없거나 Password가 틀린 경우 로그인 실패 메시지가 반환된다.

### 회원가입화면 (SignUpPage)
1. /l 또는 /L을 입력하여 로그인화면으로 전환할 수 있음. <br>
2. ID와 Password와 Nickname을 입력하여 서버에 회원가입 요청을 할 수 있음.
- DB에 이미 중복된 ID로 회원가입이 된 경우 회원가입 실패 메시지가 반환된다.
- ID나 Password, Nickname의 각 조건을 만족하지 않는 문자열을 입력한 경우 회원가입 실패 메시지가 반환된다.
- 회원가입에 성공한 경우 회원가입 성공 메시지가 반환되고 이후 로비화면으로 진입한다.

### 로비화면
1. /q 또는 /Q를 입력하여 클라이언트를 종료할 수 있음.
2. /n 또는 /N을 입력하여 신규 채팅방 개설 요청을 할 수 있음.
- 서버에 생성된 모든 채팅방 객체가 사용중인 경우 채팅방 생성 실패 메시지가 반환된다.
- 신규 채팅방 개설에 성공하면 채팅방 생성 성공 메시지가 반환되고 채팅방화면으로 진입한다.
3. 6글자의 영문으로 구성된 채팅방코드를 입력하여 기존에 생성된 채팅방 참여 요청이 가능하다.
- 동일한 채팅방 코드를 가진 채팅방이 없는 경우 채팅방 참여 실패 메시지가 반환된다.
- 동일한 채팅방 코드를 가진 채팅방을 찾은 경우 채팅방 참여 성공 메시지가 반환되고 채팅방화면으로 진입한다.

### 채팅방화면
1. /q 또는 /Q를 입력하여 현재 있는 채팅방을 퇴장할 수 있음. <br/>
- 서버에 퇴장 신호를 전달하고 클라이언트는 응답을 받지 않아도 로비로 나옴. (입장때는 코드를 받아야했지만 퇴장때는 그런 절차가 필요하지 않음.)
2. 이외의 문자열은 채팅을 친 것으로 간주하고 유저들에게 전파함. <br/>
- 같은 채팅방에 있는 유저들에게만 전파함.

# To-Do
1. 클라이언트를 유니티C#으로 구현하기. <br/>
- 기본 cmd창은 너무 밋밋하다.
2. 비밀번호를 해싱하여 저장하기. (sniffing 방지) <br/>
- 적당한 해쉬 알고리즘을 찾아 도입해보자.
3. 공개채팅방과 비밀채팅방을 구분하여 생성할 수 있도록 하고 공개채팅방목록을 로비에서 확인할 수 있도록 하자

## 개발환경
VS2022   
Windows11   
C++14
