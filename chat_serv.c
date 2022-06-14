#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void *arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt=0; //서버에 접속한 클라이언트 수
int clnt_socks[MAX_CLNT]; //클라이언트와의 송수신을 위해 생성한 소켓의 파일 디스크립터를 저장한 배열
pthread_mutex_t mutx; //뮤텍스를 통한 스레드 동기화를 위한 변수

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock; //소켓번호를 저장할 변수 두 개 선언
	struct sockaddr_in serv_adr, clnt_adr; //서버와 클라이언트 소켓 주소 구조체 생성
	int clnt_adr_sz; //클라이언트 주소의 소켓 길이
	pthread_t t_id; //종료 대기 및 리턴 값을 받을 스레드의 id값을 저장.
	if(argc!=2) { //실행파일경로 PORT번호를 입력받지 않은 경우
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL); //뮤텍스 생성
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //TCP 소켓 생성

	/*서버 주소정보 초기화*/
	memset(&serv_adr, 0, sizeof(serv_adr)); //서버 구조체의 메모리 할당, 0으로 초기화
	serv_adr.sin_family=AF_INET; //IPv4 사용
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY); //서버 ip 지정
	serv_adr.sin_port=htons(atoi(argv[1])); //포제트번호를 지
	
	/*서버 주소정보를 기반으로 주소 할당 */
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //bind를 이용하여 소켓 서버의 주소 정보를 할당(실패시 -1 반환) 
		error_handling("bind() error"); //오류 출력
	
	/*서버소켁(리스닝 소켓)이 됨
	 * 연결요청대기 */
	if(listen(serv_sock, 5)==-1) //크기가 5인 연결요청 대기큐를 생성 후 클라이언트의 연결요청을 기다림(실패시 -1 반환)
		error_handling("listen() error");

	
	while(1){
		clnt_adr_sz=sizeof(clnt_adr); //클라이언트 구조체 크기

		/* 클라이언트의 연결요청을 수락하고, 클라이언트와의 송수신을 위한 새로운 소켓 생성 후 소켓의 디스크립터 반환*/
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz); 

		pthread_mutex_lock(&mutx); //임계영역에 진입하기 위해 뮤텍스 잠금을 요청 
		clnt_socks[clnt_cnt++]=clnt_sock; //클라이언트 수와 파일 디스크립터를 등록
		pthread_mutex_unlock(&mutx); //뮤텍스 잠금 해제(임계영역 진입 불가)

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock); //스레드 생성 및 실행
		pthread_detach(t_id); //스레드가 종료되면 소멸시킴
		printf("Connected cliend IP: %s \n", inet_ntoa(clnt_adr.sin_addr)); //클라이언트 IP 정보를 문자열로 변환하여 출력
	}
	close(serv_sock); //서버소켓을 닫음
	return 0;정
}


void * handle_clnt(void *arg)
{
	int clnt_sock=*((int*)arg); //클라이언트와의 연결을 위해 생성된 소켓의 파일 디스크립터
	int str_len=0, i; //str_len: read함수를 통해 소켓으로부터 읽어들인 문자의 길이를 저장
	char msg[BUF_SIZE]; //메시지 배열 선언

	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0) //클라이언트로부터 EOF를 수신할 때까지 읽어서(소켓의 연결이 끊길 때까지 읽음)
		send_msg(msg, str_len); //모든 클라이언트들에게 메시지를 전송

	pthread_mutex_lock(&mutx); //뮤텍스 잠금, 임계영역 시작
	
	/*연결 종료 클라이언트 제거*/
	for(i=0; i<clnt_cnt; i++) 
	{
		if(clnt_sock==clnt_socks[i]) //현재 해당하는 클라이언트 소켓의 파일 디스크립터를 찾으면
		{
			while(i++<clnt_cnt-1) //스레드를 1개 삭제할 것이기 때문에 -1을 해줘야 함.
				clnt_socks[i]=clnt_socks[i+1]; //클라이언트가 연결요청을 했으므로 현재 소켓이 원래 위치했던 곳을 기준으로 뒤의 클라이언트를 땡겨와 덮어씌워 삭제함.
			break;
		}
	}
	clnt_cnt--; //클라이언트 수 감소
	pthread_mutex_unlock(&mutx); //뮤텍스 잠금 해제, 임계영역 끝
	close(clnt_sock); //클라이언트와의 송수신을 위해 생성했던 소켓 종료
	return NULL; //서비스 종료
}

/*메시지 전송 함수*/
void send_msg(char * msg, int len) //send to all
{
	int i;
	pthread_mutex_lock(&mutx); //clnt_cnt, clnt_socks[] 사용을 위해 뮤텍스 잠금
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, len); //접속한 모두에게 메시지 보내기
	pthread_mutex_unlock(&mutx); //뮤텍스 잠금 해제
}

/*에러 핸들링 함수*/
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
