#include "stdafx.h"
#include "CSerialPort.h"
#include <iostream>
#include <process.h>

using namespace std;


// void MoniterThread(LPVOID lParam);


CSerialPort::CSerialPort()
{
    m_hCom = INVALID_HANDLE_VALUE;
    m_bOpen = FALSE;
    m_bAsyn = FALSE;
    m_dwEvtMask = EV_RXCHAR; // EV_RXCHAR输入缓冲区收到数据后，触发事件
	m_sData = "";

	InitializeCriticalSection(&m_cs);
}


CSerialPort::~CSerialPort()
{
	DeleteCriticalSection(&m_cs);
    ClosePort();
}


bool CSerialPort::OpenPort(const stSerialPort_t & stSerialPort)
{
    /* 参数检查 */

    /* 变量声明和初始化 */
    DWORD dwEvtMask = 0;     // 串口使能事件
    DCB db;                  // 串口状态结构体
    const DWORD dwInQueue = 1024;   // 输入缓冲区大小
    const DWORD dwOutQueue = 1024;  // 输出缓冲区大小
    COMMTIMEOUTS  CommTimeOuts ;    // 超时结构体
    wchar_t sPort[20] = {0};
    swprintf_s(sPort, L"\\\\.\\COM%d", stSerialPort.nPort);
    int nBaudRate = stSerialPort.nBaudRate;
    int nParity = stSerialPort.nParity;
    int nStopBits = stSerialPort.nStopBits;
    int nDataBits = stSerialPort.nDataBits;
    int nFileOpenFlag = m_bAsyn ? FILE_FLAG_OVERLAPPED : 0;

    ClosePort();

    /* 函数主体 */
    // 打开串口
    m_hCom = CreateFile(sPort,
                        GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, nFileOpenFlag, 0);

    if(INVALID_HANDLE_VALUE == m_hCom)
    {
        cerr << sPort << "打开失败！" << endl;
        CloseHandle(m_hCom);
        return false;
    }
    // 设置参数
    GetCommState(m_hCom, &db);
    db.DCBlength = sizeof(db);
    db.BaudRate = nBaudRate;
    db.Parity = nParity;
    db.StopBits = nStopBits;
    db.ByteSize = nDataBits;
    SetCommState(m_hCom, &db);
    // 设置缓冲区大小
    SetupComm(m_hCom, dwInQueue, dwOutQueue);
    PurgeComm(m_hCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    // 设置超时,异步操作和同步操作都适用，这里的超时是指操作的完成时间，而不是函数操作的返回时间
    GetCommTimeouts(m_hCom, &CommTimeOuts);
    // 如果值为MAXDWORD,并且ReadTotalTimeoutConstant和ReadTotalTimeoutMultiplier两个值都为0,则指定读操作携带已经收到的字符立即返回，即使没有收到任何字符。
    CommTimeOuts.ReadIntervalTimeout = MAXDWORD;  // 接收时，两字符间最大的时延
    CommTimeOuts.ReadTotalTimeoutMultiplier = 0;  // 读取每字节的超时
    CommTimeOuts.ReadTotalTimeoutConstant = 0;    // 读串口数据的固定超时
    // 总超时 = ReadTotalTimeoutMultiplier * 字节数 + ReadTotalTimeoutConstant
    CommTimeOuts.WriteTotalTimeoutMultiplier = 50;   // 写每字节的超时
    CommTimeOuts.WriteTotalTimeoutConstant = 2000;   // 写串口数据的固定超时
    SetCommTimeouts(m_hCom, &CommTimeOuts) ;

	m_bOpen = true;

    // 设置事件
    GetCommMask(m_hCom, &dwEvtMask);
    dwEvtMask |= m_dwEvtMask;
    SetCommMask(m_hCom, dwEvtMask);

    // 开始检测事件
    if(!startMoniter())
    {
        cerr << "串口事件检测器开启失败！"<< endl;
    }

    return true;
}


void CSerialPort::ClosePort(void)
{
	m_sData = "";
    m_bOpen = false;
    CloseHandle(m_hCom);
    m_hCom = INVALID_HANDLE_VALUE;
}


 bool CSerialPort::IsOpen(void)
 {
    return m_bOpen;
 }


 bool CSerialPort::SendCmd(const char sCmd[], bool bCR/* = true*/)
 {
    if(!IsOpen())
        return false;
    string s = sCmd;

    if(bCR)
        s += "\r\n";

    DWORD dwNumberOfBytesWrite = 0;
    if(m_bAsyn)
    {
        OVERLAPPED oWrite;
		oWrite.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		oWrite.Internal = 0;
		oWrite.InternalHigh = 0;
		oWrite.Offset = 0;
		oWrite.OffsetHigh = 0;
        BOOL bRet = WriteFile(m_hCom, s.c_str(), s.length(), nullptr, &oWrite);
        if(!bRet)
        {
            DWORD dwRet = GetLastError();
            if(ERROR_IO_PENDING == dwRet)
            { 
                bRet = GetOverlappedResult(m_hCom, &oWrite, &dwNumberOfBytesWrite, TRUE);
                if(!bRet)
                {
                    cerr << "串口写入失败，失败代码：" << GetLastError() << endl;
                    return false;
                }
            }
            else
            {
                cerr << "串口写入失败，失败代码：" << dwRet << endl;
                return false;
            }
        }
        else
        {
            // 获取写入的字节数
            GetOverlappedResult(m_hCom, &oWrite, &dwNumberOfBytesWrite, TRUE);
        }
    }
    else
    {
        BOOL bRet = WriteFile(m_hCom, s.c_str(), s.length(),
                              &dwNumberOfBytesWrite, nullptr);
        if(!bRet)
        {
            cerr << "串口写入失败，失败代码：" << GetLastError() << endl;
            return false;
        }
    }
    // cout << "写入字节数：" << dwNumberOfBytesWrite << endl;
    return true;
 }



 int CSerialPort::GetData(char data[], int nMaxSize)
 {
	 int nRetSize = 0;
	 EnterCriticalSection(&m_cs);
	 int nSize = m_sData.length();
	 if (nSize <= 0)
	 {
	 }
	 else if (nSize < nMaxSize)
	 {
		 nRetSize = nSize;
		 strcpy_s(data, nMaxSize, m_sData.c_str());
		 m_sData.clear();
	 }
	 else
	 {
		 nRetSize = nMaxSize - 1;
		 strcpy_s(data, nMaxSize, m_sData.substr(0, nRetSize).c_str());
		 m_sData = m_sData.substr(nRetSize).c_str();
	 }
	 LeaveCriticalSection(&m_cs);

	 return nRetSize;
 }

 /**
 * @brief 同步与异步
 * 同步ReadFile在什么情况下会返回：1. 要求读取的字节数量已经达到；
 * 2. 管道操作时，在管道写的一端，写操作完成；
 * 3. 异步文件句柄被使用且读操作是异步操作；
 * 4. 有错误发生
 * 同样ReadFile的操作与当前的通讯超时(COMMTIMEOUTS)设定有关系
 * 当ReadIntervalTimeout的值为MAXDWORD,并且ReadTotalTimeoutConstant和ReadTotalTimeoutMultiplier两个值都为0,
 * 则指定读操作携带已经收到的字符立即返回，即使没有收到任何字符。
 * 如果输入的字符超过输入缓冲区，该如何处理
 */
 bool CSerialPort::readData()
 {
     if(!IsOpen())
         return false;

     DWORD dwNumberOfBytesRead = 0;
     char sData[4096];
     memset(sData, 0, 4096);
  
	 // 不管是否有收到字符，立刻返回
	 OVERLAPPED oRead;
	 oRead.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	 oRead.Internal = 0;
	 oRead.InternalHigh = 0;
	 oRead.Offset = 0;
	 oRead.OffsetHigh = 0;
	 BOOL bRet = ReadFile(m_hCom, sData, 4095, &dwNumberOfBytesRead, &oRead);
	 if(!bRet)
	 {
		cerr << "串口读取失败，失败代码：" << GetLastError() << endl;
		return false;
	 }
   
	 if (dwNumberOfBytesRead > 0)
	 {
		 //cout << "读取字节数：" << dwNumberOfBytesRead << endl;
		 // cout << "读取内容：" << sData << endl;
		 EnterCriticalSection(&m_cs);
		 m_sData = sData;
		 LeaveCriticalSection(&m_cs);
	 }

     return true;
 }


 void MoniterThread(LPVOID lParam)
 {
    CSerialPort * pThis = (CSerialPort*)lParam;
    if(nullptr == pThis)
        return;
    DWORD dwEvtMask = 0;     // 串口使能事件
    DWORD dwNumberOfBytesRead = 0;
    OVERLAPPED o;
    o.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    o.Internal = 0;
    o.InternalHigh = 0;
    o.Offset = 0;
    o.OffsetHigh = 0;
    BOOL bRet = FALSE;
    while(pThis->IsOpen())
    {
        bRet = WaitCommEvent(pThis->m_hCom, &dwEvtMask, &o);
        if(bRet)
        {
            if(dwEvtMask & EV_RXCHAR)
            {
                // Read Data
                pThis->readData();
            }
        }
        else
        {
            DWORD dwRet = GetLastError();
            if(ERROR_IO_PENDING == dwRet)
            {
                GetOverlappedResult(pThis->m_hCom, &o, &dwNumberOfBytesRead, TRUE);
				if (dwEvtMask & EV_RXCHAR)
				{
					// Read Data
					pThis->readData();
				}
            }
            else
            {
                cerr << "串口事件检测失败！" << endl;
                return;
            }
        }
    }
	cout << "串口事件检测线程退出！" << endl;
 }


 bool CSerialPort::startMoniter()
 {
    _beginthread(MoniterThread, 0, this);
    return true;
 }


 void CSerialPort::SetAsynchronous(void)
 {
	 if (!IsOpen())
		 m_bAsyn = true;
 }