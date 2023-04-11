/////////////////////////////////////////////////////////////////////
//			AMF Technology Base Class

#pragma once

// структура пользовательской информации о AMF файле
// заполняется функцией bool CAmf::GetInfo(AMFINFO*)
typedef struct _amfinfo{
	BYTE	amfver;		// версия AMF файла
	WORD	objcon;		// кличество объектов в AMF файле
	BOOL	isrw;		// доступ к AMF файлу; TRUE - Read/Write, FALSE - Read only
	}AMFINFO;

class CAmf
{
public:
  CAmf();
  virtual ~CAmf();
  BYTE Open(LPSTR lpFile,BOOL bCreate,BYTE AmfVer);
  void Close(void);
  BOOL GetInfo(AMFINFO* lpStructInfo);
  BYTE Add(LPSTR lpLogicPath,LPSTR lpMask);
  BYTE Extract(LPSTR lpDestPath,LPSTR lpLogPath);
  BYTE Delete(LPSTR lpLogPath);
  LPSTR GetTemporaryPath(void);
  BOOL SetTemporaryPath(LPSTR lpNewPath);
  DWORD GetBoosterSize(void);
  BOOL SetBoosterSize(DWORD dwNewSize);
  BOOL GetObjectName(BYTE bStrLen,LPSTR lpRetName,WORD wObjNum);
  WORD GetObjectNum(LPSTR lpObjName);
  BYTE GetObjectAttr(LPDWORD lpSize,LPDWORD lpAlloc,LPSTR lpObjName,WORD wObjNum);
  HANDLE GetHandle(void);

protected:
	virtual BYTE CreateAmf(BYTE);
	virtual BYTE CheckID(void);
	BYTE ExtractObject(LPSTR lpDestPath,WORD wExObject);
	BYTE DeleteObj(WORD wDelObj);
	BYTE AddEach(LPSTR lpLogPath,WIN32_FIND_DATA* lpW32FD);
	BOOL GetObjName(char* lpRet,WORD wNum);
	BOOL GetObjSizeAlloc(LPDWORD lpSize,LPDWORD lpAlloc,WORD wNumObj);
	BYTE Xoring(HANDLE,char*,BYTE,DWORD);
	BYTE DeXoring(char*,BYTE,DWORD);
	BYTE DataPump(DWORD dwTransCounter,HANDLE hSource,HANDLE hDest);
// структура заголовка файла ресурсов AMF
struct _amfheader{
	BYTE xkey;			// XOR ключ для кодирования/декодирования ОАТ
	BYTE amfv;			// версия AMF файла
	WORD fcon;			// кличество объектов в AMF файле
	}*LPAMFHEADER;

BOOL IsAmfOpen;			// открыт ли в настоящий момент AMF файл
						// TRUE - открыт , FALSE - закрыт
BOOL IsOpenedRW;		// режим доступа к открытому AMF файлу
						// TRUE - чтение/запись , FALSE - чтение
char* BufferOAT;		// указатель на буфер содержащий OAT
char* AmfID;			// указатель на идентификатор AMF файла
HANDLE hAmfFile;		// дескриптор AMF файла
char bufFullPathAmf[MAX_PATH]; // полный путь к открытому AMF файлу
char bufFullTempPath[MAX_PATH];// полный путь к TEMP файлу
DWORD dwDataBooster;	// содержит размер буфера в памяти для 
						// перекачки больших массивов данных 
						// (чем больше число, тем быстрее обмен данными)
};
