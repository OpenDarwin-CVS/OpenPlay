/*	11/14/02 * *	Rocco Bowling *	Feline Entertainment *	http://homepage.mac.com/felinegames *	http://homepage.mac.com/felinegames/games/thebelt/index.html *	felinegames@mac.com *	 *	This source code is part of The Belt, my entry in the 2002 uDevGame Mac Game Programming *	contest (refer to www.idevgames.com for more details).  My main goal for The Belt in  *	relation to the purpose of the contest was to create a game which was cross platform *	Mac and Windows from the same CodeWarrior project, without relying on third-party *	cross platform libraries.  Hence, in the bowels of this code you will find functions *	which handle the Mac case, as well as the functions which handles the Windows case. *	Most of the cross platform issues are encapsulated in the engine code, which allows *	you to modify The Belt itself in an easy, platform independent manner.  The Belt *	is also an example of good program design, with modular pieces of code, as well *	as centralized object and application managers. * *	For the record, this program is provided on an "as-is" basis for educational purposes *	only.  The source code is subject to the uDevGame Source Code License, so please refer *	to that if you have any questions. * *	If you have any questions specific to the code, feel free to send me an email. */#ifndef _X_NETWORK_H_#define _X_NETWORK_H_#pragma mark *** Includes ***#if(__dest_os == __mac_os)#define TARGET_MAC_CARBON#else#define TARGET_WINDOWS#endif#ifdef TARGET_MAC_CARBON#include <OpenTransport.h>#include <OpenTransportProviders.h>#include <string.h>#include <stdio.h>#endif#ifdef TARGET_WINDOWS#include <windows.h>#include <winsock.h>#include <stdio.h>#define closesocket close#endif#pragma mark *** Definitions ***typedef struct{	unsigned char address[4];	unsigned short port;}net_address;typedef struct{	#ifdef TARGET_MAC_CARBON	EndpointRef endpoint;	EndpointRef client_endpoint;	OTNotifyUPP notifierUPP;	TCall client_call;	struct InetAddress address;	struct InetAddress client_address;	#endif		#ifdef TARGET_WINDOWS	int socket, client_socket;	struct sockaddr_in address;	struct sockaddr_in client_address;	HANDLE commThread;	unsigned int commThreadID;	#endif		unsigned int status;	long packetsReceived;	long packetsSent;	char tcp_connected;		void (*dataFunc)(void * comm, char * buffer, unsigned int size, net_address * source);}net_communication;typedef struct{	#ifdef TARGET_WINDOWS	HANDLE lock;	#endif		#ifdef TARGET_MAC_CARBON	OTLock lock;	#endif	}NetLock;#pragma mark *** Globals ***#pragma mark *** Prototypes ***/***** General Communication Routines *****/extern net_address getCommunicationAddress(net_communication * comm);extern int compareNetworkAddresses(net_address a, net_address b);extern net_address makeNetworkAddress(char * address, unsigned short port);extern net_address defaultNetworkAddress(unsigned short port);extern char * stringNetworkAddress(net_address addr);/***** UDP Communication Routines *****/extern int initCommunication(net_communication * comm, net_address address, void (*dataFunc)(net_communication * comm, char * buffer, unsigned int size, net_address * source));extern void closeCommunication(net_communication * comm);extern int sendNetworkPacket(net_communication * comm, char * data, unsigned int size, net_address target);extern int recvNetworkPacket(net_communication * comm, char * buffer, unsigned int size, net_address * source);/***** TCP Communication Routines *****/extern int initTCPServerCommunication(net_communication * comm, net_address address, void (*dataFunc)(net_communication * comm, char * buffer, unsigned int size, net_address * source));extern void disconnectTCPServerCommunication(net_communication * comm);extern void closeTCPServerCommunication(net_communication * comm);extern int tcpServerSendPacket(net_communication * comm, char * data, unsigned int size);extern int initTCPConnection(net_communication * comm, net_address target, void (*dataFunc)(net_communication * comm, char * buffer, unsigned int size, net_address * source));extern void closeTCPClientCommunication(net_communication * comm);extern int tcpClientSendPacket(net_communication * comm, char * data, unsigned int size);/***** Synchronization Routines *****/extern NetLock * createLock();extern void destructLock(NetLock * lock);extern void aquireLock(NetLock * lock);extern void releaseLock(NetLock * lock);extern void enterCriticalSection(net_communication * comm);extern void leaveCriticalSection(net_communication * comm);#endif