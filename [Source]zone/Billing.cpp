#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h> 
#include <fcntl.h>
#include <time.h> 
#include <math.h>
#include <errno.h>
#include "./basedef.h"
#include "./CPSock.h"
#include "./Language.h"

#include "CUser.h"
#include "CMob.h"
#include "CItem.h"
#include "SendFunc.h"
#include "GetFunc.h"
#include "Billing.h"

// Missing Language.h string IDs for billing messages
#define _DN_Billing_Notice1	300
#define _DD_Billing_Notice2	301
#define _NN_Billing_Notice3	302
#define _NN_Billing_Notice4	303
#define _NN_Billing_Notice5	304
#define _NN_Billing_Notice6	305
#define _DD_Billing_Notice7	306
#define _DN_Billing_Notice8	307
#define _NN_Reconnect		308
#define _DN_Billing_Code	309

extern		CUser                  pUser    [MAX_USER];
extern		CMob                   pMob     [MAX_MOB];
extern int  BILLING;
extern int	CHARSELBILL;
extern int	ServerGroup;
void		CloseUser            (int conn);
void		Log ( char * String1, char * String2 ,unsigned int ip);

void ProcessBILLMessage( char*msg )
{	char temp[256];
	struct _AUTH_GAME * m = ( struct _AUTH_GAME *)msg;
	int    Type   = ntohl(m->Packet_Type);
	int    Result = ntohl(m->Result);
	int    Remain = ntohl(m->Bill_Remain);
	char * AccountName = m->User_ID;
	    // 1. User�߿��� AccountName�� ã�´�.
	    // 2. ������.. Billing�� Close�� ������.
	    // 3. ������ Result�� �����Ͽ� pConn�� Billing�� �����Ѵ�.
	    // �ٸ� ���������ϰ��, ���� �����ϱ⸦ ������ ��û�ؼ� ������ �޽����� �����ϴ� �κ��� �ʿ��ϴ�.
	int conn=0;
	char szConn[32];
	szConn[0]=m->Session[25];
	szConn[1]=m->Session[26];
	szConn[2]=m->Session[27];
	szConn[3]=m->Session[28];
	szConn[4]=0;
	sscanf(szConn,"%d",&conn);
	
	if	(conn<=0|| conn>=MAX_USER) 
	{
		if (Type==6)	// �ش� ����� ���� ȸ��
		{	m->Result=htonl(1);
			SendBilling2(m,6);
		}	else	SendBilling2((_AUTH_GAME*)msg,2);// �ش� ������� ������ ������ ȸ��
		return;
	}
	// conn���� ��ȿ�ϸ� Ŀ�ؼ��� ����������, id�� ��ġ�Ǵ��� ����.
	int idmatch = strcmp(AccountName,pUser[conn].AccountName);
	int msecmatch =0;
	if (pUser[conn].Session[20]!=m->Session[20]||pUser[conn].Session[21]!=m->Session[21]||pUser[conn].Session[22]!=m->Session[22]||pUser[conn].Session[23]!=m->Session[23]) msecmatch = 1;

	if	(pUser[conn].Mode==USER_EMPTY || idmatch || msecmatch) 
	{	if	(Type==6)	// �ش� ����� ���� ȸ��
		{	m->Result=htonl(1);
			SendBilling2(m,6);
		}	else  SendBilling2((_AUTH_GAME*)msg,2);// �ش� ������� ������ ������ ȸ��
		return;
	}


	int admin = 0;
	if (pUser[conn].Mode==USER_PLAY) admin=pUser[conn].Admin;

	char PacketUser[128];
	char ServerUser[128];
	strcpy(PacketUser,AccountName);strupr(PacketUser);
	strcpy(ServerUser,pUser[conn].AccountName);strupr(ServerUser);
    if	(strcmp(PacketUser,ServerUser)) 
	{	if	(Type==6)
		{	m->Result=htonl(1);
			SendBilling2(m,6);          
		}	else  SendBilling2((_AUTH_GAME*)msg,2);// SendBillMessage
	}

	if (Type==1 || Type==8)
	{
        // �������ʹ� conn�� ��ȿ�ϴ�.
		strcpy(pUser[conn].User_Status,m->User_Status);
		if	(Result==0||m->Bill_PayType[0]=='S')
		{	pUser[conn].Billing = 2;  // ���
			int hour= Remain/3600;
		    int min = (Remain - 3600*hour)/60;
	        int sec = Remain%60;
			char temp2[256];
            if	(m->Bill_Method[0]=='T')  // �ð��� //"FM" ������, "FD" ������, "TH" ����
			{	if (hour==0 && min==0 ) sprintf(temp, g_pMessageStringTable[_DN_Billing_Notice1],sec); else //"���� ���� %d�� ���ҽ��ϴ�."
				if (hour==0) sprintf(temp, g_pMessageStringTable[_DD_Billing_Notice2],min,sec);  //"���� ���� %d�� %d�� ���ҽ��ϴ�."
				else sprintf(temp, g_pMessageStringTable[_DD_Billing_Notice7],hour,min);  //"���� ���� %�ð� %d�� ���ҽ��ϴ�."                   
				sprintf(temp2,"^%s",temp);
				if	(BILLING==2 || (BILLING==3 && admin>=1))
				{	if	(CHARSELBILL==0 && Type==8 ) SendClientMessage(conn,temp2);
					if	(CHARSELBILL!=0 && Type==1 ) SendClientMessage(conn,temp2);
				}
			}	else                   // ������
			if	(m->Bill_Method[0]=='F')
			{
			   //if (ddate<0)      ddate=0;
				if	(m->Bill_PayType[0]=='S')	pUser[conn].Child=1;
				else							pUser[conn].Child=0;
				sprintf(temp, g_pMessageStringTable[_DN_Billing_Notice8],Remain);  //"���� ���� %d�� ���ҽ��ϴ�"
				sprintf(temp2,"^%s",temp);
				if	( (BILLING==2||(BILLING==3&&admin>=1)) && (m->User_Status[0]!='P'))
				{
					if	(CHARSELBILL==0 && Type==8 ) SendClientMessage(conn,temp2);
					if	(CHARSELBILL!=0 && Type==1 ) SendClientMessage(conn,temp2);
				}
			}
		    return;
		}   else
	    if	(Result==10||Result==11||Result==12)//10:�������������� 11:�ܾ׾��� 12:������ ����
		{	pUser[conn].Billing = 3;  // ��
			return;
		}	else
	    if	(Result==32)                  
		{	pUser[conn].Billing = 4;  // ������
		}	else
		if	(Result==35)                  
		{	pUser[conn].Billing = 5;  // ��������
			sprintf(temp,  g_pMessageStringTable[_NN_Reconnect]); ////"����Ŀ� �ٽ� �������ֽñ� �ٶ��ϴ�.
			if (BILLING==2 || (BILLING==3 && admin>=1)) SendClientMessage(conn,temp);
		}	else
		{	pUser[conn].Billing = 6;  // ��Ÿ����.
			sprintf(temp,  g_pMessageStringTable[_DN_Billing_Code] ,Result); ////"������������ [%d]�� �����ڵ带 ���½��ϴ�."
			if (BILLING==2 || (BILLING==3 && admin>=1)) SendClientMessage(conn,temp);
		}
	}	else
	if	(Type==2)
	{

	}	else
	if  (Type==5)
	{
		int hour= Remain/3600;
		int min = (Remain - 3600*hour)/60;
	    int sec = Remain%60;
		if  (BILLING==2 || (BILLING==3 && admin>=1))
		{
			if  (Result==1)
			{   if (hour==0 && min==0 ) sprintf(temp, g_pMessageStringTable[_DN_Billing_Notice1],sec); else //"���� ���� %d�� ���ҽ��ϴ�."
                 if (hour==0) sprintf(temp, g_pMessageStringTable[_DD_Billing_Notice2],min,sec);  //"���� ���� %d�� %d�� ���ҽ��ϴ�."
			     else sprintf(temp, g_pMessageStringTable[_DD_Billing_Notice7],hour,min);  //"���� ���� %�ð� %d�� ���ҽ��ϴ�."                   
			     if (BILLING==2 || (BILLING==3 && pUser[conn].Mode==USER_PLAY)) SendClientMessage(conn,temp);
			}   else
			if  (Result==-3)
			{
			     //SendClientMessage(conn,temp);
			}  else
			{	if	(Result==0)	
				{	//SendClientSignalParm(conn,0,_MSG_BillingPage,0);
					SendClientMessage(conn, g_pMessageStringTable[_NN_Billing_Notice3]);  // "���� ������ �ܿ� �ð��� ��� ����ϼ̽��ϴ�."
				}	else
				if	(Result==-1)
				{	//SendClientSignalParm(conn,0,_MSG_BillingPage,0);
					SendClientMessage(conn, g_pMessageStringTable[_NN_Billing_Notice4]);  //"�����ݾ���,ȯ�� ó���Ǿ� ������ �����մϴ�."
				}	else
				if	(Result==-2)	SendClientMessage(conn, g_pMessageStringTable[_NN_Billing_Notice5] );//"�ٸ� ���������� ���ϰ����� �����Ͽ� ������ �����մϴ�."
				else SendClientMessage(conn, g_pMessageStringTable[_NN_Billing_Notice6]); //"���������� ���Ͽ� ������ �����մϴ�."
				
				pUser[conn].cSock.SendMessage();
				CloseUser(conn);
			}
		}
	}	else
	if  (Type==6)
	{	m->Result=htonl(0);
		SendBilling2(m,6);          
	}
	return;
}


// SendBilling defined in Server.cpp
// SendBilling2 defined in Server.cpp
