#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void * send_msg(void *arg); //메시지 전송 함수 선언
void * recv_msg(void * arg); //메시지 수신 함수 선언
void error_handling(char * msg); //오류 핸들링 함수 선언

char name[NAME_SIZE]="[DEFAULT]"; //채팅자 이름 배열 선언
char msg[BUF_SIZE]; //메시지 배열 선언


int main(int argc, char *argv[])
{
	int sock; //파일 디스크립터 저장
	struct sockaddr_in serv_addr; //서버 소켓 주소 구조체 생성
	pthread_t snd_thread, rcv_thread; //보내고 받는 스레드 선언
	void * thread_return; //스레드 리턴 선언, ip와 port, 채팅닉네임 받기 
	if(argc!=4) { //실행파일경로 IP PORT번호 채팅닉네임을 입력받지 않은 경우 
		printf("Usage : %s <IP> <port> <name>\n", argv[0]); 
		exit(1); //프로그램 비정상 종료
	}

	sprintf(name, "[%s]", argv[3]); //채팅닉네임(argv[3])을 [%s]서식으로 name 배열에 저장 
	sock=socket(PF_INET, SOCK_STREAM, 0); //TCP 소켓 생성 

	/*서버 주소정보 초기화 */
	memset(&serv_addr, 0, sizeof(serv_addr)); //서버 구조체의 메모리를 0으로 초기화
	serv_addr.sin_family=AF_INET; //IPv4 사용
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]); //서버 IP 지정
	serv_addr.sin_port=htons(atoi(argv[2])); //포트 지정(문자열을 int로 변환 후 네트워크 바이트로 변환)
	
	/*서버 주소정보를 기반으로 연결요청
	 * 이때 비로소 클라이언트 소켓이 됨 */
	if(connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1) //실패시 -1
		error_handling("connect() error"); //에러 출력
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); //스레드 생성 및 실행: 메시지 보냄
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock); //스레드 생성 및 실행: 메시지 받음
	pthread_join(snd_thread, &thread_return); //스레드 종료까지 대기: 송신 스레드 소멸
	pthread_join(rcv_thread, &thread_return); //스레드 종료까지 대기: 수신 스레드 소멸
	close(sock); //클라이언트 소켓 연결종료
	return 0;
}

/*메시지 송신 함수*/음
void * send_msg(void * arg) //send thread main
{
	int sock=*((int*)arg); //호출받은 클라이언트 파일 디스크립터
	char name_msg[NAME_SIZE+BUF_SIZE]; //메시지 배열 선언
	while(1)
	{
		fgets(msg, BUF_SIZE, stdin); //키보드 입력을 받음 
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) //q나 Q를 입력하기 전까지 문자열 전송
		{
			close(sock); //클라이언트 소켓 연결종료 후
			exit(0); //프로그램 종료
		}
		sprintf(name_msg, "%s %s", name, msg); //client 이름과 msg를 합쳐 메시지 배열에 입력
		write(sock, name_msg, strlen(name_msg)); //널문자 제외하고 서버로 문자열을 보냄
	}
	return NULL; //송신 종료
}

/*메시지 수신 함수*/
void * recv_msg(void * arg) //read thread main
{
	int sock=*((int*)arg); //클라이언트의 파일 디스크립터
	char name_msg[NAME_SIZE+BUF_SIZE]; //채팅닉네임+메시지 버퍼
	int str_len; //문자열 길이
	while(1)
	{
	str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1); //메시지 수신
	if(str_len==-1) //read 실패시= 통신이 끊겼다면
		return (void*)-1; //스레드 종료
		name_msg[str_len]=0; //메시지 끝을 설정
		fputs(name_msg, stdout); //화면에 수신된 메시지 표시
	}
	return NULL; //수신 종료
}

/* 오류 핸들링 함수*/
void error_handling(char * msg)
{
	fputs(msg, stderr); //오류 메시지 표시
	fputc('\n', stderr);
	exit(1); //비정상 종료 처리
}
