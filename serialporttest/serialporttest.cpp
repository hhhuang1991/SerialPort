// serialporttest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include "CSerialPort.h"

using namespace std;


int main()
{
	cout << "输入串口号：";
	int nPort;
	cin >> nPort;
	CSerialPort serialport;
	stSerialPort_t stSerialPort;
	stSerialPort.nPort = nPort;
	bool bRet = false;
	serialport.SetAsynchronous();
	bRet = serialport.OpenPort(stSerialPort);
	if (!bRet)
	{
		getchar();
		return -1;
	}

	while (true)
	{
		cout << "请选择操作（0-退出，1-读数据，2-写数据）：";
		int nSel = 0;
		cin >> nSel;
		if (0 == nSel) break;
		else if (1 == nSel)
		{
			bool bReadEnd = true;
			while (bReadEnd)
			{
				char buff[1024];
				memset(buff, 0, 1024);
				int nRet = serialport.GetData(buff, 10);
				if (nRet > 0)
				{
					bReadEnd = true;
					cout << "读取字节数：" << nRet << endl;
					// cout << "读取内容：" << buff << endl;
					cout << "读取内容：" << endl;
					for (int i = 0; i < nRet; i++)
					{
						int bytes = (BYTE)buff[i];
						cout << hex << bytes << " " << endl;
					}
				}
				else
				{
					bReadEnd = false;
				}
			}
		}
		else if (2 == nSel)
		{
			static int nFlag = 0;
			char sendbuf[1024] = {0};
			sendbuf[0] = 0x81;
			sendbuf[1] = 0x83;
			sendbuf[2] = 0x85;
			sendbuf[3] = 0x87;
			//sprintf_s(sendbuf, "Hello World: %d", nFlag++);
			serialport.SendCmd(sendbuf);
		}
	}

	serialport.ClosePort();
	getchar();
	return 0;
}

