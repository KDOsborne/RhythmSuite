#include <sodium.h>
#include "server.h"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "../error/error.h"
#include "../text/text.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define PORT 1997
#define PACKET_SIZE 128
#define RESPONSE_SIZE 32

#define CREDFILE "credentials.bin"
#define SERVER_IP "3.21.215.88" //"192.168.50.252"

struct server_struct* server_;

struct message {
	long 			command;
	unsigned char 	data[PACKET_SIZE-crypto_box_MACBYTES-4];
};

typedef struct message_queue {
	struct message m;
	struct message_queue *head,*tail;
}message_queue;

typedef struct server_thread {
	HANDLE				handle_;
	DWORD				id_;
	SOCKET 				socket_;
	struct sockaddr_in 	clientService_;
	char				result_[64];
	char*				data_;
	int					rc_,dataSize_;
	struct message		message_;
}server_thread;

struct createaccount_message {
	long	  		command;
	unsigned char 	username[16];
	unsigned char 	password[crypto_generichash_BYTES];
};

struct login_message {
	long	  		command;
	unsigned char 	username[16];
	unsigned char 	password[crypto_generichash_BYTES];
};

struct validate_message {
	long	  		command,id;
	unsigned char 	hash[crypto_generichash_BYTES];
};

struct submit_message {
	long		  command;
	unsigned char username[16];
	unsigned char hash[crypto_generichash_BYTES];
	unsigned char score[48];
};

struct request_message {
	long	  		command,id,flags;
};

struct checkupdate_message {
	long	  		command;
	unsigned char 	hash[crypto_generichash_BYTES];
};

static void auto_login();
static void save_credentials();
static int load_credentials();
static int senddata(SOCKET,struct message*);
static DWORD WINAPI run_thread(void*);
static int receive_data(int,int);
static void add_message(struct message*);
static void clear_messages();

static unsigned char client_pk[crypto_kx_PUBLICKEYBYTES] = {
	0x69,0x77,0xCD,0xC0,0xB8,0xC7,0x1D,0x16,
	0x3C,0x60,0x08,0x57,0xB2,0x1E,0x9C,0x6D,
	0xF7,0x6D,0xEC,0x31,0x6D,0x07,0x66,0x7B,
	0xF1,0x22,0x1E,0xCD,0x5A,0x1A,0x43,0x73
};
static unsigned char client_sk[crypto_kx_SECRETKEYBYTES] = {
	0x4A,0xB0,0xA6,0x02,0xEF,0xFA,0x85,0xE1,
	0x9A,0x70,0x59,0xC4,0xF3,0x16,0x7F,0xFE,
	0x23,0xAF,0x5B,0x90,0xE0,0x1B,0xD5,0xB1,
	0xDB,0x4F,0x99,0x9D,0x4F,0x54,0x53,0xAF
};
static unsigned char server_pk[crypto_kx_PUBLICKEYBYTES] = {
	0x8E,0xF5,0x79,0x6C,0x11,0xE6,0xD1,0x82,
	0x6E,0xCF,0x3F,0x54,0x9F,0x25,0xE3,0xB0,
	0x64,0xC6,0x37,0x5A,0x43,0x1B,0xEA,0x00,
	0x95,0xC1,0x7B,0x74,0x57,0x78,0x89,0x5C
};
static unsigned char nonce[crypto_box_NONCEBYTES] = {
	0x65,0x70,0x40,0x34,0x00,0x5f,0x5f,0x69,
	0x6d,0x70,0x5f,0x5f,0x76,0x66,0x70,0x72,
	0x69,0x6e,0x74,0x66,0x00,0x5f,0x5f,0x69
};

static char validate_file[MAX_PATH];
static char validate_file_queue[MAX_PATH];

static int sodium_initialized, winsock_initialized;
static WSADATA winsock;
static server_thread threads_[NUM_MESSAGES-1];
static int thread_ids[NUM_MESSAGES-1];

static message_queue* mqueue;

void init_server()
{
	server_ = (struct server_struct*)malloc(sizeof(struct server_struct));
	memset(server_,0,sizeof(struct server_struct));
	memset(server_->username_,'\0',16);
	
	for(int i = 0; i < NUM_MESSAGES-1; i++)
	{
		server_->sdata_[i].data = NULL;
		thread_ids[i] = i;
	}
	
	mqueue = NULL;
	memset(threads_,0,sizeof(threads_));
	
	if(sodium_init() < 0) 
	{
		write_error("Sodium Failed to Initialize",NULL,0);
		sodium_initialized = -1;
		return;
    }
	else
	{
		sodium_initialized = 1;
	}
	
	int iResult = WSAStartup(MAKEWORD(2, 2), &winsock);
    if (iResult != NO_ERROR) 
	{
		write_error_int("WSAStartup Error: ",WSAGetLastError(),0);
        winsock_initialized = -1;
        return;
    }
	else
		winsock_initialized = 1;
	
	if(load_credentials() == -1)
		save_credentials();
	else
		auto_login();
	server_update();
}
 
void process_server()
{
	for(int i = 0; i < NUM_MESSAGES-1; i++)
	{
		if(threads_[i].rc_ == 1 || threads_[i].rc_ == -1)
		{
			if(WaitForSingleObject(threads_[i].handle_,0) == WAIT_OBJECT_0)
			{
				CloseHandle(threads_[i].handle_);
				if(threads_[i].rc_ == -1)
				{
					//write_error(threads_[i].result_,NULL,0);
					server_->sdata_[i].status = -1;
				}
				else
				{
					server_->sdata_[i].status = 1;
					if(i+1 == MESSAGE_CREATE)
					{
						if(server_->sdata_[MESSAGE_CREATE-1].result[0] == 1)
							auto_login();
					}
				}
				server_->update_ = 1;
				memset(&threads_[i],0,sizeof(server_thread));
			}
		}
		else
		{
			message_queue* mq = mqueue;
			
			while(mq != NULL)
			{
				if(ntohl(mq->m.command)-1 == i)
				{
					if(threads_[i].rc_ == 0)
					{
						threads_[i].rc_ = 10;
						memcpy(&threads_[i].message_,&mq->m,sizeof(struct message));
						
						server_->scount_[i]--;
						server_->sdata_[i].status = 10;
						server_->sdata_[i].bytes = 0;
						memset(server_->sdata_[i].result,0,sizeof(server_->sdata_[0].result));
						if(server_->sdata_[i].data != NULL)
						{
							free(server_->sdata_[i].data);
							server_->sdata_[i].data = NULL;
						}
						
						if(i+1 == MESSAGE_VALIDATE || i+1 == MESSAGE_SUBMIT)
							sprintf(validate_file,"%s",validate_file_queue);
						
						threads_[i].handle_ = CreateThread(NULL,0,run_thread,&thread_ids[i],0,&threads_[i].id_);
					
						if(mq->head == NULL)
						{
							mqueue = mqueue->tail;
							if(mqueue != NULL)
								mqueue->head = NULL;
							free(mq);
							
							mq = mqueue;
							continue;
						}
						else
						{
							message_queue* temp = mq;
							mq = mq->head;
							mq->tail = temp->tail;
							if(mq->tail != NULL)
								mq->tail->head = mq;
							
							free(temp);
						}
					}
				}
				mq = mq->tail;
			}
		}
	}
}

void render_serverinfo()
{	
	if(!server_->signedin_)
	{
		if(server_->sdata_[MESSAGE_LOGIN-1].status == 1)
			render_simpletext("FAILED TO SIGN IN...",1.0,1.0,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
		else if(server_->sdata_[MESSAGE_LOGIN-1].status == -1)
			render_simpletext("COULDNT CONNECT TO SERVER...",1.0,1.0,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
		else if(server_->sdata_[MESSAGE_LOGIN-1].status == 10)
			render_simpletext("SIGNING IN...",1.0,1.0,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
		else
			render_simpletext("NOT SIGNED IN",1.0,1.0,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
	}
	else
	{
		char buffer[32];
		sprintf(buffer,"SIGNED IN: %s",(char*)server_->username_);
		render_simpletext(buffer,1.0,1.0,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
	}
	
	server_->update_ = 0;
}

void clear_serverdata(int id)
{
	if(server_->sdata_[id].data != NULL)
		free(server_->sdata_[id].data);
	memset(server_->sdata_+id,0,sizeof(struct server_data));
	server_->sdata_[id].data = NULL;
}

void logout()
{
	server_->signedin_ = 0;
	memset(server_->username_,'\0',16);
	memset(server_->password_,0,crypto_generichash_BYTES);
	
	save_credentials();
	
	if(server_->sdata_[MESSAGE_LOGIN-1].status != 10)
		clear_serverdata(MESSAGE_LOGIN-1);
}

void create_account(char* username, char* password)
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(strlen(username) < 3 || strlen(password) < 8)
		return;
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_CREATE);
	memcpy(((struct createaccount_message*)&m)->username,username,strlen(username)+1);
	crypto_generichash(((struct createaccount_message*)&m)->password,crypto_generichash_BYTES,password,strlen(password),NULL,0);
	
	add_message(&m);
}

void login(char* username, char* password)
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(strlen(username) < 3 || strlen(password) < 8)
		return;
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_LOGIN);
	memcpy(((struct login_message*)&m)->username,username,strlen(username)+1);
	crypto_generichash(((struct login_message*)&m)->password,crypto_generichash_BYTES,password,strlen(password),NULL,0);
	
	add_message(&m);
}

static void auto_login()
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(strlen((char*)server_->username_) < 3)
		return;

	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_LOGIN);
	memcpy(((struct login_message*)&m)->username,server_->username_,16);
	memcpy(((struct login_message*)&m)->password,server_->password_,crypto_generichash_BYTES);

	add_message(&m);
}

void validate_track(long id, char* filename)
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(!server_->signedin_)
		return;
	
	FILE* fp = fopen(filename,"rb");
	if(!fp)
		return;
	fclose(fp);
	sprintf(validate_file_queue,"%s",filename);
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_VALIDATE);
	((struct validate_message*)&m)->id = htonl(id);
	
	add_message(&m);
}

void submit_score(char* score, char* filename)
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(!server_->signedin_)
		return;
	
	FILE* fp = fopen(filename,"rb");
	if(!fp)
		return;
	fclose(fp);
	sprintf(validate_file_queue,"%s",filename);
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_SUBMIT);
	memcpy(((struct submit_message*)&m)->username,server_->username_,16);
	memcpy(((struct submit_message*)&m)->score,score,strlen(score)+1);
	
	add_message(&m);
}

void request_scores(long id)
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	if(!server_->signedin_)
		return;
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_REQUEST);
	((struct validate_message*)&m)->id = htonl(id);
	
	add_message(&m);
}

void server_update()
{
	if(sodium_initialized == -1 || winsock_initialized == -1)
		return;
	
	FILE* fp = fopen("rhythmsuite.exe","rb");
	if(!fp)
		return;
	
	long bytes;
	fseek(fp,0,SEEK_END);
	bytes = ftell(fp);
	rewind(fp);
	
	unsigned char* data = malloc(bytes);
	fread(data,1,bytes,fp);
	fclose(fp);
	
	struct message m;
	memset(&m,0,sizeof(struct message));
	
	m.command = htonl(MESSAGE_CHECKUPDATE);
	crypto_generichash(((struct checkupdate_message*)&m)->hash,crypto_generichash_BYTES,data,bytes,NULL,0);

	free(data);
	
	add_message(&m);
}

void destroy_server()
{
	if(winsock_initialized == 1)
		WSACleanup();
	
	clear_messages();
	
	for(int i = 0; i < 5; i++)
		if(server_->sdata_[i].data != NULL)
			free(server_->sdata_[i].data);
		
	free(server_);
	server_ = NULL;
}

static void save_credentials()
{
	FILE* fp = fopen(CREDFILE,"wb");
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return;
	}
	
	unsigned char buffer[48], data[crypto_box_MACBYTES+48];
	
	memcpy(buffer,server_->username_,16);
	memcpy(buffer+16,server_->password_,32);
	
	if(crypto_box_easy(data,buffer,sizeof(buffer),nonce,client_pk,client_sk) == -1)
	{
		write_error("ENCRYPTION ERROR",NULL,0);
	}
	
	fwrite(data,sizeof(data),1,fp);
	fclose(fp);
}

static int load_credentials()
{
	FILE* fp = fopen(CREDFILE,"rb");
	if(!fp)
		return -1;
	
	unsigned char buffer[48], data[crypto_box_MACBYTES+48];
	
	fread(data,sizeof(data),1,fp);
	fclose(fp);
	
	if(crypto_box_open_easy(buffer,data,sizeof(data),nonce,client_pk,client_sk) == -1)
	{
		write_error("CREDENTIAL FILE TAMPERED",NULL,0);
		return -1;
	}
	else
	{
		memcpy(server_->username_,buffer,16);
		memcpy(server_->password_,buffer+16,32);
	}
	
	return 0;
}

static DWORD WINAPI run_thread(void* id_)
{
	int id = *((int*)id_);
	if(id+1 == MESSAGE_VALIDATE || id+1 == MESSAGE_SUBMIT)
	{
		FILE* fp = fopen(validate_file,"rb");
		if(!fp)
			return -1;
		
		fseek(fp,0,SEEK_END);
		long size = ftell(fp);
		
		if(size <= 0)
		{
			fclose(fp);
			return -1;
		}

		unsigned char* buffer = malloc(size);
		
		rewind(fp);
		fread(buffer,1,size,fp);
		fclose(fp);
		
		unsigned char hash[crypto_generichash_BYTES];
		crypto_generichash(hash,sizeof(hash),buffer,size,NULL,0);
		
		free(buffer);
		
		if(id+1 == MESSAGE_VALIDATE)
			memcpy(((struct validate_message*)&threads_[id].message_)->hash,hash,crypto_generichash_BYTES);
		else if(id+1 == MESSAGE_SUBMIT)
			memcpy(((struct submit_message*)&threads_[id].message_)->hash,hash,crypto_generichash_BYTES);
	}
	
	threads_[id].socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(threads_[id].socket_ == INVALID_SOCKET)
	{
		sprintf(threads_[id].result_,"Socket Error: %d",WSAGetLastError());
		threads_[id].rc_ = -1;
        return -1;
	}
	
	ZeroMemory(&threads_[id].clientService_, sizeof(threads_[id].clientService_)); 
	threads_[id].clientService_.sin_family = AF_INET;
    //threads_[id].clientService_.sin_addr.s_addr = inet_addr("18.218.70.244");
	threads_[id].clientService_.sin_addr.s_addr = inet_addr(SERVER_IP);
    threads_[id].clientService_.sin_port = htons(PORT);
	
	int rc = connect(threads_[id].socket_,(SOCKADDR*)&(threads_[id].clientService_),sizeof(threads_[id].clientService_));
    if (rc == SOCKET_ERROR) 
	{
		closesocket(threads_[id].socket_);
		sprintf(threads_[id].result_,"Unable to connect to server: %d",WSAGetLastError());
		threads_[id].rc_ = -1;
        return -1;
    }
	
	senddata(threads_[id].socket_,&threads_[id].message_);
	
	unsigned char response[RESPONSE_SIZE];
	int n;
	while ((n = recv(threads_[id].socket_,response,RESPONSE_SIZE,MSG_PEEK)) < RESPONSE_SIZE)
	{
		if(n == -1)
		{
			closesocket(threads_[id].socket_);
			sprintf(threads_[id].result_,"Receive Error: %d",WSAGetLastError());
			threads_[id].rc_ = -1;
			return -1;
		}
	}
	
	n = recv(threads_[id].socket_,response,RESPONSE_SIZE,0);
	
	long result_[4];
	if(crypto_box_open_easy((unsigned char*)result_,response,n,nonce,server_pk,client_sk) == -1)
	{
		closesocket(threads_[id].socket_);
		sprintf(threads_[id].result_,"DECRYPTION FAILED");
		threads_[id].rc_ = -1;
		return -1;
	}
	else
	{
		result_[0] = ntohl(result_[0]);
		result_[1] = ntohl(result_[1]);
		result_[2] = ntohl(result_[2]);
		result_[3] = ntohl(result_[3]);
		
		switch(id+1)
		{
			case MESSAGE_CREATE:
				if(result_[0] == 1)
				{
					memset(server_->username_,0,16);
					memset(server_->password_,0,32);
					memcpy(server_->username_,
						((struct createaccount_message*)&threads_[id].message_)->username,
						16);
					memcpy(server_->password_,
						((struct createaccount_message*)&threads_[id].message_)->password,
						32);
					save_credentials();
				}
			break;
			case MESSAGE_LOGIN:
				if(result_[0] == 1)
				{
					server_->signedin_ = 1;
					
					memset(server_->username_,0,16);
					memset(server_->password_,0,32);
					memcpy(server_->username_,
						((struct login_message*)&threads_[id].message_)->username,
						16);
					memcpy(server_->password_,
						((struct login_message*)&threads_[id].message_)->password,
						32);
					save_credentials();
				}
				else
				{
					server_->signedin_ = 0;
					
					memset(server_->username_,0,16);
					memset(server_->password_,0,32);
					save_credentials();
				}
			break;
			case MESSAGE_VALIDATE:
				if(result_[0] == 2)
				{
					if(receive_data(id,result_[2]) == -1)
						return -1;
				}
			break;
			case MESSAGE_SUBMIT:
				if(result_[0] == 1)
				{
					
				}
			break;
			case MESSAGE_REQUEST:
				if(result_[0] == 1)
				{
					if(receive_data(id,result_[2]) == -1)
						return -1;
				}
			break;
			case MESSAGE_CHECKUPDATE:
				if(result_[0] == 2)
				{
					if(receive_data(id,result_[2]) == -1)
						return -1;
				}
			break;
		}
	}
	
	memcpy(server_->sdata_[id].result,result_,sizeof(result_)); 
	
	rc = shutdown(threads_[id].socket_,SD_BOTH);
	/*if (rc == SOCKET_ERROR)
	{
		closesocket(threads_[id].socket_);
		sprintf(threads_[id].result_,"Shutdown Failed With Error: %d",WSAGetLastError());
		threads_[id].rc_ = -1;
		return -1;
	}*/
	
	closesocket(threads_[id].socket_);
	sprintf(threads_[id].result_,"Completed!");
	threads_[id].rc_ = 1;
	
	return 0;
}

static int receive_data(int id, int bytes)
{
	bytes += crypto_box_MACBYTES;
	
	unsigned char* data = malloc(bytes);
	int n, tot = 0;
	while(tot < bytes)
	{
		n = recv(threads_[id].socket_,data+tot,bytes-tot,0);
		if(n == SOCKET_ERROR)
        {
			int err = WSAGetLastError();
            if(err != WSAEWOULDBLOCK)
			{
				closesocket(threads_[id].socket_);
				sprintf(threads_[id].result_,"Receive Error: %d",WSAGetLastError());
				threads_[id].rc_ = -1;
				free(data);
				return -1;
			}
        }
		else
			tot += n;
		
		Sleep(500);
	}

	bytes -= crypto_box_MACBYTES;
	server_->sdata_[id].data = malloc(bytes);
	
	if(crypto_box_open_easy(server_->sdata_[id].data,data,tot,nonce,server_pk,client_sk) == -1)
	{
		closesocket(threads_[id].socket_);
		sprintf(threads_[id].result_,"DATA DECRYPTION FAILED");
		threads_[id].rc_ = -1;
		free(data);
		free(server_->sdata_[id].data);
		server_->sdata_[id].data = NULL;
		return -1;
	}
	
	server_->sdata_[id].bytes = bytes;
	
	free(data);
	
	return 0;
}

static void add_message(struct message* m)
{
	if(mqueue == NULL)
	{
		mqueue = (message_queue*)malloc(sizeof(message_queue));
		memcpy(&mqueue->m,m,sizeof(struct message));
		mqueue->head = NULL;
		mqueue->tail = NULL;
		server_->scount_[ntohl(m->command)-1]++;
		return;
	}
	
	message_queue* mq = mqueue;
	while(mq != NULL)
	{
		if(mq->m.command == m->command)
		{
			memcpy(&mq->m,m,sizeof(struct message));
			return;
		}
		else if(mq->tail == NULL)
		{
			mq->tail = (message_queue*)malloc(sizeof(message_queue));
			memcpy(&mq->tail->m,m,sizeof(struct message));
			mq->tail->head = mq;
			mq->tail->tail = NULL;
			server_->scount_[ntohl(m->command)-1]++;
			return;
		}
		mq = mq->tail;
	}
}

static void clear_messages()
{
	while(mqueue != NULL)
	{
		message_queue* mq = mqueue;
		mqueue = mqueue->tail;
		free(mq);
	}
}

static int senddata(SOCKET sock, struct message* m)
{	
    unsigned char pbuf[PACKET_SIZE];
	if(crypto_box_easy(pbuf,(unsigned char*)m,sizeof(struct message),nonce,server_pk,client_sk) == -1)
	{
		write_error("ENCRYPTION ERROR",NULL,0);
		return -1;
	}
	
	int buflen = PACKET_SIZE;
	
	int pos = 0;
    while (buflen > 0)
    {
        int n = send(sock,pbuf+pos,buflen,0);

        if(n == SOCKET_ERROR)
        {
			int err = WSAGetLastError();
            if(err != WSAEWOULDBLOCK)
			{
				write_error_int("Send Error: ",WSAGetLastError(),0);
				return -1;
			}
        }

        pos += n;
        buflen -= n;
    }
	
    return 0;
}