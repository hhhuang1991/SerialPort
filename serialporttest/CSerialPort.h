#ifndef CSERIALPORT_H
#define CSERIALPORT_H
#include <Windows.h>
#include <string>

/*校验位*/
enum COM_PARITY
{
    COM_PARITY_NONE = 0,    // 无校验
    COM_PARITY_ODD,         // 偶校验
    COM_PARITY_EVEN,        // 奇校验
    COM_PARITY_MARK,
    COM_PARITY_SPACE,
};

/*停止位*/
enum COM_STOPBITS
{
    COM_STOPBITS_10 = 0,    // 1
    COM_STOPBITS_15,        // 1.5
    COM_STOPBITS_20,        // 2
};

/*数据位*/
enum COM_DATABITS
{
    COM_DATABITS_5 = 5,
    COM_DATABITS_6 = 6,
    COM_DATABITS_7 = 7,
    COM_DATABITS_8 = 8,
};

/*串口参数结构*/
typedef struct SerialPortData
{
    int nPort;
    int nBaudRate;
    int nParity;
    int nStopBits;
    int nDataBits;
    SerialPortData()
    {
        nPort = 1;
        nBaudRate = 9600;
        nParity = COM_PARITY_NONE;
        nStopBits = COM_STOPBITS_10;
        nDataBits = COM_DATABITS_8;
    }
}stSerialPort_t;

class CSerialPort
{
public:
    CSerialPort();
    ~CSerialPort();

	/**
	* @brief 事件检测线程函数
	*/
	friend void MoniterThread(LPVOID lParam);
	
private:
    /**
    * @brief 异步模式下开始检测串口事件
    */
    bool startMoniter();

	bool readData();

public:
	/**
	* @brief 设置串口是以同步方式通讯，还是异步方式通讯。如果需要设置为异步通讯，必须在OpenPort之间调用
	*/
	void SetAsynchronous(void);
    /**
    * @brief 打开串口
    */
    bool OpenPort(const stSerialPort_t &);
    /**
    * @brief 关闭串口
    */
    void ClosePort(void);
    /**
    * @brief 串口是否开启
    */
    bool IsOpen(void);
    /**
    * @brief 发送指令
    * @param[in] sCmd 指令字符串
    * @param[in] bCR 是否自动加回车换行。默认加回车换行符
    */
    bool SendCmd(const char sCmd[], bool bCR = true);

    /**
    * @brief 读取返回内容，并清空内容
    * @param[out] data 数据缓冲区
    * @param[in] nMaxSize data的最大空间
	* @return 返回实际读取的数据的字节大小
    */
	int GetData(char data[], int nMaxSize);

public:

private:
	HANDLE m_hCom;   // 串口设备句柄
    bool m_bOpen;    // 串口是否打开
    bool m_bAsyn;    // true = 异步， false = 同步
    DWORD m_dwEvtMask;  // 异步通讯时，设置的串口事件

	CRITICAL_SECTION m_cs; // 对m_sData操作的保护
	std::string m_sData;   // 保存读取到的数据
};

#endif // CSERIALPORT_H
