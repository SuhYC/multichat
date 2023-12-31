# multichat
cpp project <br/>
기본형 폴더에 있는건 인터넷에 떠돌아다니는 채팅방코드를 윈도우버전으로 조금 수정한 것 뿐입니다.<br/>
응용 폴더에 있는 코드를 확인하시면 됩니다.

# 요약
TCP 통신 <br/>
멀티스레드 프로그래밍

# 기능
초기상태 : 로비; 입장한 채팅방이 없음
1. n 또는 N을 입력하여 새로운 채팅방을 개설할 수 있음. <br/>
새로운 채팅방 코드 6자를 부여받으며 이 코드는 기존의 채팅방 코드와 중복되지 않음. (서버는 클라이언트에 채팅방 코드를 보내줌으로서 자신의 채팅방의 코드를 확인할 수 있도록 함.)
2. q또는 Q를 입력하여 통신을 종료할 수 있음. <br/>
로비에서 q를 입력받으면 통신을 종료함.
3. 6자리의 영문 대문자로 이루어진 채팅방코드를 입력하여 기존에 개설된 방에 참여할 수 있음. <br/>
서버에 존재하는 채팅방 중 하나의 채팅방코드를 정확히 입력하면 해당 채팅방에 입장함. (클라이언트에는 채팅방코드를 다시 보내줌으로서 정상적으로 입장했음을 표현.)<br/>
일치하는 채팅방 코드가 없다면 입장실패. 그대로 로비상태 유지. (클라이언트에는 fail을 보내줌으로서 입장하지 못했음을 표현)<br/>

채팅방에 참여한 상태
1. q 또는 Q를 입력하여 현재 있는 채팅방을 퇴장할 수 있음. <br/>
서버에 퇴장 신호 ("q")를 전달하고 클라이언트는 응답을 받지 않아도 로비로 나옴. (입장때는 코드를 받아야했지만 퇴장때는 그런 절차가 필요하지 않음.)
2. 이외의 문자열은 채팅을 친 것으로 간주하고 유저들에게 전파함. <br/>
같은 채팅방에 있는 유저들에게만 전파함.

# 새로 얻은 경험 (Heart Beat)
구상한 바를 모두 구현하고 테스트한 뒤 콘솔을 종료하는 과정에서 클라이언트 콘솔을 종료하였을 때 서버측에 반영이 안되는 현상이 발생했다. (물론 이때는 if문 분기중 채팅방O && SOCKECT_ERROR 상황에 대한 분기를 깜빡하는 바람에 발생한 오류였다.) <br/>
당시엔 "클라이언트가 비정상 종료를 한 경우에 대한 처리"를 해야한다고 생각하여 여러가지 고민을 하였는데, recv함수를 호출하면 클라이언트로부터 데이터를 받기 전까지 block 상태에 놓이게 되고, 서버측에선 클라이언트의 상태를 알 수 없기 때문에 한없이 기다리는 상황 또한 나올 수 있다.<br/>
물론 소켓을 Non Blocking 형태로 구현할 수도 있겠지만 수신할 데이터가 없어도 계속 반복문을 돌기 때문에 서버에 가해지는 부하가 커질거 같아 다른 방법을 생각해보기로 했다.

그래서 문득 생각난 것은 학부 네트워크 강의 중 나왔던 키워드인 Heart Beat.<br/>
드라마나 영화에서 중환자 옆에 항상 있는 심장박동을 확인하는 기계처럼 상대측과의 연결이 정상적인지 떠보는(?) 기술이다.<br/>
한쪽의 호스트가 상대에게 정상적인 연결인지 물으면 반대쪽 호스트가 연결이 정상적임을 응답하여 정상연결을 확인한다.

이러한 개념이 cpp tcp에는 어떤 식으로 구현가능한지 구글링을 해본 결과 얻은 정보는 SIO_KEEPALIVE_VALS. (SO_KEEPALIVE도 있지만 얘는 현재 PC에 있는 모든 소켓 레지스트리를 수정한다고.. 되도록 SIO_KEEPALIVE_VALS를 사용하는 것이 권장된다.)<br/>
동시에 해당 코드가 필요한 상황에 대한 예시도 제공되었다.<br/>
완벽한 구현을 했다고 생각해서 상사에게 보고하였으나 서버전원을 내려버리는 기상천외한 테스트에서 탈락하여 사내 솔루션 문서를 다시 읽어보게 되는 계기가 되었다고..

다시 돌아와서 실험환경을 재현했다. 노트북으로는 서버 프로그램을, 데스크탑으로는 클라이언트 프로그램을 실행하고 이것저것 테스트한 뒤 데스크탑의 네트워크를 끊어버렸다. (모바일 테더링으로 와이파이 끌어다 쓰니까 쉽게 끊겨서 좋더라)<br/>
아직 SIO_KEEPALIVE_VALS 옵션을 안넣으니 아무리 기다려도 서버측에선 클라이언트가 끊긴 것도 모른채 클라이언트가 만들어둔 채팅방을 고이고이 모셔두고 있었다.

이번에는 SIO_KEEPALIVE_VALS 옵션을 넣고 다시 테스트하니 연결 끊고 10초도 안되어 소켓이 죽은소켓임을 확인하고 연결을 끊었다. 물론 클라이언트가 점거해둔 채팅방도 다시 메모리를 회수하는데 성공.
