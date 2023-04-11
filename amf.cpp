/////////////////////////////////////////////////////////////////////
//			AMF Technology Base Class
//			v1.1b

#include <windows.h>
#include <time.h>
#include <malloc.h>
#include "amf.h"
#include "amfdef.h"

/////////////////////////////////////////////
//		 онструктор / ƒеструктор		   //
/////////////////////////////////////////////

CAmf::CAmf()
{
	IsAmfOpen=FALSE;	// в настоящий момент нет открытого AMF файла
	AmfID="AMF";		// установка идентификатора AMF
	strcpy(bufFullTempPath,"C:\\WINDOWS\\TEMP\\");// установка каталога для TEMP файла по умолчанию
	dwDataBooster=DATA_BUFFER_SIZE;	// устанавливаем размер буфера
	// »нициируем датчик случайных чисел
	srand((unsigned)time(NULL));
	// —ледующая строка инициализирует указатель на структуру _amfheader.
	// ѕри запуске на исполнение без этой инициализации, дома, появляется
	// ошибка (при обращении к структуре _amfheader через LPAMFHEADER).
	// ќднако на работе, даже без следующей инициализации класс функционирует
	// безошибочно.
	LPAMFHEADER=new _amfheader;
}

CAmf::~CAmf()
{
// уничтожаем указатель на структуру _amfheader
delete LPAMFHEADER;
Close();		// закрываем AMF файл, если таковой открыт
}

/////////////////////////////////////////////
//     –еализации методов класса		   //
/////////////////////////////////////////////

BYTE CAmf::Open(LPSTR lpFile,BOOL bCreate,BYTE AmfVer)
{
	BOOL IsOpenRW=TRUE;
	
	if(IsAmfOpen)return O_OPENED;

	// ‘айл открывается и для записи, и для чтения.
	// ¬ многопоточных приложениях не рекомендуется использование
	// записи чего-либо в AMF файл !!!
	hAmfFile=CreateFile(
		lpFile,
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	// пытаемся открыть только для чтения
	// (это на тот случай, если файл находится на CD)
	if(hAmfFile==INVALID_HANDLE_VALUE){
	hAmfFile=CreateFile(
		lpFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
		if(hAmfFile!=INVALID_HANDLE_VALUE)IsOpenRW=FALSE;
		}

	if(hAmfFile==INVALID_HANDLE_VALUE){
		if(!bCreate)return O_ERROR_OPEN;	// нельзя открыть AMF файл
		// безусловное создание AMF файла
		hAmfFile=CreateFile(
		lpFile,
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL);
		// невозможно создать файл
		if(hAmfFile==INVALID_HANDLE_VALUE)return O_CANT_CREATE;

		BYTE bCNResult=CreateAmf(AmfVer);	// для AMF версий 1 и 2
		if((bCNResult==CA_CANT_WRITE)||(bCNResult==CA_NO_MEMORY)){
			CloseHandle(hAmfFile);
			// нельзя задействовать созданный файл
			return O_CANT_CREATE;}
		}

if(GetFileSize(hAmfFile,NULL)<7){
	CloseHandle(hAmfFile);
	return O_NOT_AMF;		// файл не является носителем технологии AMF
	}

SetFilePointer(hAmfFile,NULL,NULL,FILE_BEGIN);	// указатель на начало файла
DWORD dwReaded;
// читаем заголовок AMF файла (версии 1 или 2)
ReadFile(hAmfFile,LPAMFHEADER,sizeof(LPAMFHEADER),&dwReaded,NULL);

// проверяем идентификатор AMF файла (версии 1 или 2)
BYTE bIDResult=CheckID();
if(bIDResult!=CI_OK){
	CloseHandle(hAmfFile);
	return O_NOT_AMF;}		// открытый файл не AMF
// проверяем файл на максимальную версию
if(LPAMFHEADER->amfv>MAX_VER_AMF){
	CloseHandle(hAmfFile);
	return O_NOT_AMF;		// открытый файл не AMF
	}

	IsAmfOpen=TRUE;			// помечаем что AMF файл открыт

	IsOpenedRW=IsOpenRW;	// нужно для затычек к функциям
							// использующим запись в AMF файл
	// сохраняем имя с полным путЄм вновь открытого AMF файла
	strcpy(bufFullPathAmf,lpFile);

	if(IsOpenRW)return O_RW_OK;
	else return O_R_OK;
}

/////////////////////////////////////////////////////////////////////
//	«акрываем AMF файл (если он открыт)
void CAmf::Close(void)
{
	if(IsAmfOpen){
	CloseHandle(hAmfFile);
	IsAmfOpen=FALSE;
	}
}

/////////////////////////////////////////////////////////////////////
//	«аполняем структуру типа LPAMFINFO
BOOL CAmf::GetInfo(AMFINFO* lpStructInfo)
{

	if(!IsAmfOpen)return GI_AMFNOTOPENED;	// AMF файл не открыт
	lpStructInfo->amfver=LPAMFHEADER->amfv;
	lpStructInfo->objcon=LPAMFHEADER->fcon;
	lpStructInfo->isrw=IsOpenedRW?TRUE:FALSE;
	return GI_OK;							// структура заполнена

}

/////////////////////////////////////////////////////////////////////
//	ƒобавление файла(ов) по маске в AMF файл
BYTE CAmf::Add(LPSTR lpLogicPath,LPSTR lpMask)
{
	if(!IsAmfOpen)return A_NO_AMF;
	if(!IsOpenedRW)return A_AMF_READ_ONLY;

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	BYTE bAddResult;

	hFind=FindFirstFile((LPCTSTR)lpMask,&FindFileData);
	if(hFind==INVALID_HANDLE_VALUE)return A_NO_FILES;
	bAddResult=AddEach(lpLogicPath,&FindFileData);

	while(FindNextFile(hFind,&FindFileData))
		{
		bAddResult=AddEach(lpLogicPath,&FindFileData);

		}
	FindClose(hFind);

	return bAddResult;
}

/////////////////////////////////////////////////////////////////////
//	записываем в файл заголовок и криптованный идентификатор AMF
//	(всего 7 байт)
//	“ќЋ№ ќ для 1 и 2 версий AMF
BYTE CAmf::CreateAmf(BYTE AmfVer)
{
DWORD dwRealWrite;
BOOL  bIsWrite;
// заполняем структуру заголовка и записываем в созданный AMF файл
LPAMFHEADER->xkey=(BYTE)GetRandom(0,255);
LPAMFHEADER->amfv=AmfVer;
LPAMFHEADER->fcon=0;
bIsWrite=WriteFile(hAmfFile,LPAMFHEADER,sizeof(LPAMFHEADER),&dwRealWrite,NULL);
if(bIsWrite==0)return CA_CANT_WRITE;
// ксорим идентификатор и записываем его в файл по текущей позиции
BYTE IsXor=Xoring(hAmfFile,AmfID,LPAMFHEADER->xkey,3);
switch(IsXor){
	case X_CANT_WRITE:return CA_CANT_WRITE;
	case X_NO_MEMORY:return CA_NO_MEMORY;
	}

return CA_OK;
}

/////////////////////////////////////////////////////////////////////
//	ксорим передаваемую строку и прописывыем еЄ в файл 
//	по текущей позиции
BYTE CAmf::Xoring(HANDLE hHandle,char* lpSource,BYTE xorkey,DWORD dwSize)
{
BYTE*	lpBuffer;
DWORD	dwCounter=0,dwRealWrite;
BYTE	dta1,dta2;

if(dwSize==NULL)return X_ZERO_SIZE;	// недопустимое значение
lpBuffer=(BYTE*)malloc(dwSize);		// пытаемся выделить память
if(lpBuffer==0)return X_NO_MEMORY;	// нельзя выделить память
while(dwCounter<dwSize){
	if(dwCounter==0)dta1=xorkey;
	dta2=(BYTE)lpSource[dwCounter];
	dta1^=dta2;
	lpBuffer[dwCounter]=dta1;
	dwCounter++;
	}
// записываем содержимое буфера с результатом в файл по текущей позиции
BOOL bIsWrite=WriteFile(hHandle,lpBuffer,dwSize,&dwRealWrite,NULL);
free(lpBuffer);						// высвобождаем память
if(bIsWrite==0)return X_CANT_WRITE;	// нельзя сохранить результат
return X_OK;
}

////////////////////////////////////////////////////////////////////
//	читаем данные из текущей позиции в файле, ксорим прочитанные
//	данные и складываем в буфер
BYTE CAmf::DeXoring(char* lpDest,BYTE xorkey,DWORD dwSize)
{
BYTE*	lpTemp;
DWORD	dwRealRead;
BYTE	dta1,dta2;

if(dwSize==NULL)return DX_ZERO_SIZE;// недопустимое значение
lpTemp=(BYTE*)malloc(dwSize);		// пытаемся выделить память
if(lpTemp==0)return DX_NO_MEMORY;	// нельзя выделить память
// читаем данные из AMF файла
BOOL bIsRead=ReadFile(hAmfFile,lpTemp,dwSize,&dwRealRead,NULL);
if(bIsRead==0)return DX_CANT_READ;
// ксорим
while(dwSize>0){
	if(dwSize==1)dta2=xorkey;
		else dta2=lpTemp[(dwSize-1)-1];
	dta1=lpTemp[dwSize-1];
	dta1^=dta2;
	lpDest[dwSize-1]=(char)dta1;
	dwSize--;
	}

free(lpTemp);						// высвобождаем память

return DX_OK;
}

////////////////////////////////////////////////////////////////////
//	проверка идентификатора AMF файла 
//	(“ќЋ№ ќ для 1 и 2 версий AMF!)
BYTE CAmf::CheckID(void)
{
char* lpFileID;

SetFilePointer(hAmfFile,4,NULL,FILE_BEGIN);	// указатель на ID
lpFileID=(char*)malloc(3);
BYTE IsReXor=DeXoring(lpFileID,LPAMFHEADER->xkey,3);
switch(IsReXor){
	case DX_CANT_READ:
		free(lpFileID);
		return CI_CANT_READ;
	case DX_NO_MEMORY:
		free(lpFileID);
		return CI_NO_MEMORY;
	}
if(strstr(lpFileID,AmfID)==NULL){
	free(lpFileID);
	return CI_NOT_AMF;
	}
free(lpFileID);
return CI_OK;
}

/////////////////////////////////////////////////////////////////////
//	возвращает имя объекта по порядковому номеру в OAT
//	только для AMF v1 и v2
//	TRUE - OK , FALSE - error
//	lpRet - в нЄм возвращается имя объекта (размер строки должен
//	учитывать нулевой завершающий символ)
BOOL CAmf::GetObjName(char* lpRet,WORD wNum)
{
	if((!wNum)||(wNum>LPAMFHEADER->fcon))return FALSE;

	BYTE bSecLenth,bNameLenth;

	if(LPAMFHEADER->amfv==1){
		bSecLenth=20;bNameLenth=12;}
	else {
		bSecLenth=68;bNameLenth=60;}

	SetFilePointer(hAmfFile,((wNum-1)*bSecLenth)+6,0,FILE_BEGIN);

	DWORD	dwRealRead;
	BYTE	dta1,dta2,bSize=bNameLenth;
	BYTE lpTemp[61];	// буфер распаковываемых данных

	// читаем данные из AMF файла
	ReadFile(hAmfFile,&lpTemp,bNameLenth+1,&dwRealRead,NULL);
	// ксорим
	while(bSize>0){
		if((wNum==1)&&(bSize==1)&&(LPAMFHEADER->amfv==2))dta2=LPAMFHEADER->xkey;
			else dta2=lpTemp[bSize-1];
		dta1=lpTemp[bSize];
		dta1^=dta2;
		lpRet[bSize-1]=(char)dta1;
		bSize--;
		}
	lpRet[bNameLenth]='\0';

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//	добавление каждого файла в AMF версии 1 и 2
BYTE CAmf::AddEach(LPSTR lpLogPath,WIN32_FIND_DATA* lpW32FD)
{
	// SecNameLenNull длина имени в секции с Null Term.,
	// SecLen длина секции.
	BYTE SecNameLenNull,SecLen;
	if(LPAMFHEADER->amfv==1){
		SecNameLenNull=13;
		SecLen=20;
		}
	else {
		SecNameLenNull=61;
		SecLen=68;
		}

	char NameToAdd[61];	// имя закачиваемого объекта 
	BOOL IsFound=FALSE;

	// корректировка имени или пути для добавления в новую секцию OAT
	if(LPAMFHEADER->amfv==1){
		// преобразуем имя файла в нижний регистр
		if(strlen(lpW32FD->cFileName)<=12){
			_strlwr(lpW32FD->cFileName);
			strcpy(NameToAdd,lpW32FD->cFileName);
			}
		else {
			_strlwr(lpW32FD->cAlternateFileName);
			strcpy(NameToAdd,lpW32FD->cAlternateFileName);
			}

		}
	else {
		// проверяем, влезет ли генерируемый путь с файлом в 60 байт?
		if((strlen(lpLogPath)+strlen(lpW32FD->cFileName))>60)return AE_LOGPATH_TOOBIG;

		// генерируем логический путь к файлу для новой секции
		strcpy(NameToAdd,lpLogPath);
		strcat(NameToAdd,lpW32FD->cFileName);

		}

	// проверяем, нет ли в OAT имени добавляемого файла 
	if(LPAMFHEADER->fcon){
		WORD wObj=1;	// счЄтчик объектов
		char* strbuf;	// сюда будем выводить имя из OAT
		strbuf=(char*)malloc(SecNameLenNull);
		if(!strbuf)return AE_NO_MEM;
		while(wObj<=LPAMFHEADER->fcon)
			{
			GetObjName(strbuf,wObj);	// получаем имя объекта по номеру
			if(!_stricmp(strbuf,NameToAdd)){
				wObj=0xFFFE;
				IsFound=TRUE;}
			wObj++;
			}
		free(strbuf);
		}
	if(IsFound)return AE_ALREADY_GET;	// в OAT такое имя уже существует. ќшибка. ¬ыход.

	// генерируем имя временного (TEMP) файла, а затем присоединяем
	// его к пути для временных файлов
	char tempName[13];
	char tempPath[MAX_PATH];
	strcpy(tempPath,bufFullTempPath);	// это чтобы не испортить эталон пути (bufFullTempPath)
	wsprintf(tempName,"amf_%d.tmp",GetRandom(1111,9999));
	strcat(tempPath,tempName);
	// теперь tempName - чистое имя временного файла
	// теперь tempPath - путь с именем временного файла

	// создаЄм обновлЄнный AMF заголовок (структура _amfheader) в памяти
	_amfheader NEWAMFHEADER;
	NEWAMFHEADER.amfv=LPAMFHEADER->amfv;
	NEWAMFHEADER.fcon=(LPAMFHEADER->fcon)+1;
	NEWAMFHEADER.xkey=LPAMFHEADER->xkey;

	// открываем временный файл
	// если такой уже существует, обнуляем
		HANDLE hTempFile;
	hTempFile=CreateFile(
	tempPath,
	GENERIC_READ|GENERIC_WRITE,
	NULL,
	NULL,
	CREATE_ALWAYS,
	0,
	NULL);
	// невозможно создать файл
	if(hTempFile==INVALID_HANDLE_VALUE)return AE_CANT_CREATE_TEMP;

	// записываем во временный файл новый AMF заголовок
	DWORD dwRealWrite;
	BOOL bIsWrite=WriteFile(hTempFile,&NEWAMFHEADER,sizeof(NEWAMFHEADER),&dwRealWrite,NULL);
	if(!bIsWrite){
		// закрываем временный файл
		CloseHandle(hTempFile);
		DeleteFile(tempPath);	// удаляем временный файл
		return AE_CANT_WRITE;	// нельзя записать заголовок
		}

	// переписывыем ID и старую часть OAT в TEMP файл
	DWORD dwTransCounter;	// счЄтчик переписываемых данных (в байтах)
	BYTE  bData;			// переменная для байта перемещаемых данных
	dwTransCounter=/*(LPAMFHEADER->fcon*SecLen)+*/3;
		// установка указателя в AMF файле
	SetFilePointer(hAmfFile,4,0,FILE_BEGIN);
		// (указатель в TEMP файле уже установлен)

		// буферизированная перекачка данных (ID и старая часть OAT)
	switch(DataPump(dwTransCounter,hAmfFile,hTempFile)){
		case DP_NO_MEM:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			return AE_NO_MEM;
		case DP_CANT_WRITE:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			return AE_CANT_WRITE;
			}

	// преобразуем указатели на старые объекты
	// т.е. увеличиваем их на длину одной секции
	if(LPAMFHEADER->fcon){
		WORD count;
		BYTE bXorByte;
		LPDWORD lpDoubleWord;
		char* lpSecBuffer;
		lpSecBuffer=(char*)malloc(SecLen);
		if(!lpSecBuffer){
			CloseHandle(hTempFile);
			DeleteFile(tempPath);			// удаляем временный файл
			return AE_NO_MEM;}
		lpDoubleWord=(LPDWORD)lpSecBuffer;	// область выделенной памяти в DWORD формате

		for(count=0;count<LPAMFHEADER->fcon;count++){
			// XOR ключ для распаковывания
			if((LPAMFHEADER->amfv==2)&&(count==0))
				bXorByte=NEWAMFHEADER.xkey;
			else {
				SetFilePointer(hAmfFile,(count*SecLen)+6,0,FILE_BEGIN);
				ReadFile(hAmfFile,&bXorByte,1,&dwRealWrite,NULL);}
			// указатель на начало секции
			SetFilePointer(hAmfFile,(count*SecLen)+7,0,FILE_BEGIN);
			// читаем и распаковываем секцию
			if(DeXoring(lpSecBuffer,bXorByte,SecLen)){
				free(lpSecBuffer);
				CloseHandle(hTempFile);
				DeleteFile(tempPath);			// удаляем временный файл
				return AE_NO_MEM;}
			if(LPAMFHEADER->amfv==1)lpDoubleWord[4]+=SecLen;// увеличиваем смещение до начала данных
			else lpDoubleWord[16]+=SecLen;					// увеличиваем смещение до начала данных
			// XOR ключ для пакования
			if((LPAMFHEADER->amfv==2)&&(count==0))
				bXorByte=NEWAMFHEADER.xkey;
			else {
				SetFilePointer(hTempFile,-1,0,FILE_END);
				ReadFile(hTempFile,&bXorByte,1,&dwRealWrite,NULL);}
			// пишем правленую секцию в конец “Ёћѕа
			switch(Xoring(hTempFile,lpSecBuffer,bXorByte,SecLen)){
				case X_CANT_WRITE:
					free(lpSecBuffer);
					CloseHandle(hTempFile);
					DeleteFile(tempPath);			// удаляем временный файл
					return AE_CANT_WRITE;
				case X_NO_MEMORY:
					free(lpSecBuffer);
					CloseHandle(hTempFile);
					DeleteFile(tempPath);			// удаляем временный файл
					return AE_NO_MEM;
				}

			}
		free(lpSecBuffer);
		}

	// сохраняем последний прочитанный байт для XOR операции
	// или из заголовка
	if((LPAMFHEADER->amfv==2)&&(NEWAMFHEADER.fcon==1))
		bData=NEWAMFHEADER.xkey;
	else {
		SetFilePointer(hTempFile,-1,0,FILE_END);
		ReadFile(hTempFile,&bData,1,&dwRealWrite,NULL);
		}

	// открываем добавляемый файл (только для чтения)
	// с параллельным доступом для чтения (FILE_SHARE_READ),
	// так как текущий AMF файл имеет параллельный доступ для
	// чтения и записи (FILE_SHARE_WRITE|FILE_SHARE_READ), то
	// добавления самого в себя не произойдЄт!!!
		HANDLE hAddFile;
	hAddFile=CreateFile(
	lpW32FD->cFileName,
	GENERIC_READ,
	FILE_SHARE_READ,
	NULL,
	OPEN_EXISTING,
	0,
	NULL);
	// невозможно открыть файл
	if(hAddFile==INVALID_HANDLE_VALUE){
		CloseHandle(hTempFile);
		DeleteFile(tempPath);			// удаляем временный файл
		return AE_CANT_OPEN_ADD;}

	// если добавляемый файл имеет нулевой размер, выход
	if(!GetFileSize(hAddFile,NULL)){
		CloseHandle(hAddFile);
		CloseHandle(hTempFile);
		DeleteFile(tempPath);			// удаляем временный файл
		return AE_ZERO_ADD;
		}

	// генерируем секцию нового объекта и записываем в TEMP файл
	char* secbuf;		// буфер для генерируемой секции
	LPDWORD bufshift;	// для записи 32-битных чисел в секции
	secbuf=(char*)malloc(SecLen);
	if(!secbuf){
		// закрываем временный файл
		CloseHandle(hTempFile);
		DeleteFile(tempPath);			// удаляем временный файл
		CloseHandle(hAddFile);
		return AE_NO_MEM;}
	ZeroMemory(secbuf,SecLen);
	strcpy(secbuf,NameToAdd);				// прописываем имя
	bufshift=(LPDWORD)secbuf;				// область выделенной памяти в DWORD формате
	if(LPAMFHEADER->amfv==1){
		bufshift[3]=GetFileSize(hAddFile,NULL);	// длина загружаемого файла
		bufshift[4]=(GetFileSize(hAmfFile,NULL)+20);// смещение до начала данных объекта
		}
	else {
		bufshift[15]=GetFileSize(hAddFile,NULL);	// длина загружаемого файла
		bufshift[16]=(GetFileSize(hAmfFile,NULL)+68);// смещение до начала данных объекта
		}

		// кодируем и записываем новую секцию
	switch(Xoring(hTempFile,secbuf,bData,SecLen)){
		case X_CANT_WRITE:
			free(secbuf);
			CloseHandle(hTempFile);
			DeleteFile(tempPath);			// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_CANT_WRITE;
		case X_NO_MEMORY:
			free(secbuf);
			CloseHandle(hTempFile);
			DeleteFile(tempPath);			// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_NO_MEM;
		}

	free(secbuf);

	// переписываем старые данные во временный файл
		// получаем длину старых данных всех объектов
	dwTransCounter=(GetFileSize(hAmfFile,NULL)-7)-(LPAMFHEADER->fcon*SecLen);
		// устанавливаем указатель на начало данных в AMF файле
	SetFilePointer(hAmfFile,7+(LPAMFHEADER->fcon*SecLen),0,FILE_BEGIN);

		// буферизированная перекачка данных hAmfFile->hTempFile
	switch(DataPump(dwTransCounter,hAmfFile,hTempFile)){
		case DP_NO_MEM:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_NO_MEM;
		case DP_CANT_WRITE:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_CANT_WRITE;
			}

	/// ***

	// перекачиваем данные из добавляемого файла во временный
	SetFilePointer(hAddFile,0,0,FILE_BEGIN);
	dwTransCounter=GetFileSize(hAddFile,NULL);

		// буферизированная перекачка данных hAddFile->hTempFile
	switch(DataPump(dwTransCounter,hAddFile,hTempFile)){
		case DP_NO_MEM:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_NO_MEM;
		case DP_CANT_WRITE:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			CloseHandle(hAddFile);
			return AE_CANT_WRITE;
			}

	// закрываем добавляемый файл
	CloseHandle(hAddFile);
	// закрываем временный файл
	CloseHandle(hTempFile);

	// процесс обновления AMF файла
	Close();						// закрываем AMF файл
	CopyFile(tempPath,bufFullPathAmf,FALSE);	// копируем временный файл в рабочий AMF
	DeleteFile(tempPath);			// удаляем временный файл
	Open(bufFullPathAmf,FALSE,1);	// открываем обновлЄнный AMF файл


	return AE_OK;
}

/////////////////////////////////////////////////////////////////////
//	буферизированная перекачка больших массивов данных
BYTE CAmf::DataPump(DWORD dwTransCounter,HANDLE hSource,HANDLE hDest)
{
	BYTE* membuf;
	DWORD dwRealWrite;

	while(dwTransCounter>0){
		if(dwTransCounter>=dwDataBooster){		// данных больше чем ...
			membuf=(BYTE*)malloc(dwDataBooster);
				if(!membuf){
					return DP_NO_MEM;
					}
			dwTransCounter-=dwDataBooster;
			}
		else	{
			membuf=(BYTE*)malloc(dwTransCounter);
			if(!membuf){
					return DP_NO_MEM;
					}
			dwTransCounter=0;
			}

		ReadFile(hSource,membuf,_msize(membuf),&dwRealWrite,NULL);	// читаем
		WriteFile(hDest,membuf,_msize(membuf),&dwRealWrite,NULL);// сохраняем
		if(dwRealWrite<_msize(membuf)){
			free(membuf);
			return DP_CANT_WRITE;
			}
		free(membuf);
		}

	return DP_OK;
}

/////////////////////////////////////////////////////////////////////
//	”даление объектов или логических каталогов из текущего
//	AMF файла
BYTE CAmf::Delete(LPSTR lpLogPath)
{
	if(!IsAmfOpen)return D_NO_AMF;
	if(!LPAMFHEADER->fcon)return D_NO_OBJ;
	if(!IsOpenedRW)return D_AMF_READ_ONLY;

	char SlashBuf[61];
	LPSTR lpSlashBuf=SlashBuf;	// буфер поиска слэша
	char ObjName[61];
	LPSTR lpObjName=ObjName;		// буфер имени объекта
	WORD wObjNum;					// номер объекта

	// удаление объектов из корневого логического каталога
	// ћј– ≈– - lpLogPath==""
	// AMF v1 и 2
	if(!strlen(lpLogPath)){
		for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
			GetObjName(lpObjName,wObjNum);
			if(!strstr(lpObjName,"/")){
				if(DeleteObj(wObjNum)==DO_OK){	// если удаление прошло успешно...
						if(wObjNum<(LPAMFHEADER->fcon+1))wObjNum--;
					}
				}
			}
		return D_OK;
		}

	// удаление логического каталога
	// ћј– ≈– - lpLogPath==".../"         последний символ слэш !!!
	// только для AMF версии 2
	if((LPAMFHEADER->amfv==2)&&(lpLogPath[strlen(lpLogPath)-1]=='/')){
		for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
			GetObjName(lpObjName,wObjNum);
			if(strstr(lpObjName,lpLogPath)==lpObjName){
				strncpy(lpSlashBuf,lpObjName+strlen(lpLogPath),strlen(lpObjName)-strlen(lpLogPath));
				lpSlashBuf[strlen(lpObjName)-strlen(lpLogPath)]='\0';
				if(!strstr(lpSlashBuf,"/")){
					if(DeleteObj(wObjNum)==DO_OK){	// если удаление прошло успешно...
						if(wObjNum<(LPAMFHEADER->fcon+1))wObjNum--;
						}
					}
				}

			}
		return D_OK;
		}

	// удаление конкретного объекта
	// ћј– ≈– - lpLogPath==полный путь (если есть) и имя объекта
	for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
		GetObjName(lpObjName,wObjNum);
		if(!_stricmp(lpObjName,lpLogPath))return DeleteObj(wObjNum);
		}
	return D_OBJ_NOTFOUND;

}

/////////////////////////////////////////////////////////////////////
//	”даление конкретного объекта из текущего AMF файла
BYTE CAmf::DeleteObj(WORD wDelObj)
{
	HANDLE hTempFile;	// дескриптор временного файла
	DWORD dwRealWrite;	// количество реально прочитанных байт
	BOOL bIsWrite;		// если NULL, то ошибка записи (WriteFile())
	DWORD dwTransCounter;// счЄтчик переписываемых данных (в байтах)
	DWORD dwSizeStoringFile,	// размер
		  dwAllocStoringFile;	// расположение
	BYTE SecNameLenNull,SecLen;

	// SecNameLenNull длина имени в секции с Null Term.,
	// SecLen длина секции.
	if(LPAMFHEADER->amfv==1){
		SecNameLenNull=13;
		SecLen=20;
		}
	else {
		SecNameLenNull=61;
		SecLen=68;
		}

	// генерируем имя временного (TEMP) файла, а затем присоединяем
	// его к пути для временных файлов
	char tempName[13];
	char tempPath[MAX_PATH];
	strcpy(tempPath,bufFullTempPath);	// это чтобы не испортить эталон пути (bufFullTempPath)
	wsprintf(tempName,"amf_%d.tmp",GetRandom(1111,9999));
	strcat(tempPath,tempName);
	// теперь tempName - чистое имя временного файла
	// теперь tempPath - путь с именем временного файла

	// создаЄм обновлЄнный AMF заголовок (структура _amfheader) в памяти
	_amfheader NEWAMFHEADER;
	NEWAMFHEADER.amfv=LPAMFHEADER->amfv;
	NEWAMFHEADER.fcon=(LPAMFHEADER->fcon)-1;
	NEWAMFHEADER.xkey=LPAMFHEADER->xkey;

	// открываем временный файл
	// если такой уже существует, обнуляем
	hTempFile=CreateFile(
	tempPath,
	GENERIC_READ|GENERIC_WRITE,
	NULL,
	NULL,
	CREATE_ALWAYS,
	0,
	NULL);
	// невозможно создать файл
	if(hTempFile==INVALID_HANDLE_VALUE)return DO_CANT_CREATE_TEMP;

	// записываем во временный файл новый AMF заголовок
	bIsWrite=WriteFile(hTempFile,&NEWAMFHEADER,sizeof(NEWAMFHEADER),&dwRealWrite,NULL);
	if(!bIsWrite){
		// закрываем временный файл
		CloseHandle(hTempFile);
		DeleteFile(tempPath);	// удаляем временный файл
		return DO_CANT_WRITE;	// нельзя записать заголовок
		}

	// переписывыем ID
	dwTransCounter=3;
		// установка указателя в AMF файле
	SetFilePointer(hAmfFile,4,0,FILE_BEGIN);
		// (указатель в TEMP файле уже установлен)

		// буферизированная перекачка данных (ID)
	switch(DataPump(dwTransCounter,hAmfFile,hTempFile)){
		case DP_NO_MEM:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			return DO_NO_MEM;
		case DP_CANT_WRITE:
			CloseHandle(hTempFile);
			DeleteFile(tempPath);	// удаляем временный файл
			return DO_CANT_WRITE;
			}

	// корректируем и переписываем OAT и данные (объекты)
	// не забываем уменьшить указатели на данные на длину одной секции
		// если удаляется последний объект, то ни OAT, ни данные не трогаем!
	if(NEWAMFHEADER.fcon){
		WORD count,counttemp;
		BYTE bXorByte;
		LPDWORD lpDoubleWord;
		char* lpSecBuffer;
		lpSecBuffer=(char*)malloc(SecLen);
		if(!lpSecBuffer){
			CloseHandle(hTempFile);
			DeleteFile(tempPath);			// удаляем временный файл
			return DO_NO_MEM;}
		lpDoubleWord=(LPDWORD)lpSecBuffer;	// область выделенной памяти в DWORD формате

		// узнаЄм размер и позицию объекта
		if(!GetObjSizeAlloc(
		   &dwSizeStoringFile,
		   &dwAllocStoringFile,
		   wDelObj))return DO_CANT_GET_SIZEALLOC;

		// корректируем OAT
		counttemp=0;	// для определения XOR ключа OAT TEMP файла
		for(count=0;count<LPAMFHEADER->fcon;count++){
			// XOR ключ для распаковывания
			if((LPAMFHEADER->amfv==2)&&(count==0))
				bXorByte=NEWAMFHEADER.xkey;
			else {
				SetFilePointer(hAmfFile,(count*SecLen)+6,0,FILE_BEGIN);
				ReadFile(hAmfFile,&bXorByte,1,&dwRealWrite,NULL);}
			// указатель на начало секции
			SetFilePointer(hAmfFile,(count*SecLen)+7,0,FILE_BEGIN);
			// читаем и распаковываем секцию
			if(DeXoring(lpSecBuffer,bXorByte,SecLen)){
				free(lpSecBuffer);
				CloseHandle(hTempFile);
				DeleteFile(tempPath);			// удаляем временный файл
				return DO_NO_MEM;}
			// если текущая секция удаляемая, то пропускаем еЄ обработку и запись в TEMP
			if((count+1)!=wDelObj){
				// корректировочный расчЄт позиции объекта
				if(LPAMFHEADER->amfv==1){
					if(lpDoubleWord[4]<dwAllocStoringFile)lpDoubleWord[4]-=SecLen;
					else lpDoubleWord[4]-=(dwSizeStoringFile+SecLen);
					}
				else {
					if(lpDoubleWord[16]<dwAllocStoringFile)lpDoubleWord[16]-=SecLen;
					else lpDoubleWord[16]-=(dwSizeStoringFile+SecLen);
					}
				// XOR ключ для пакования
				if((LPAMFHEADER->amfv==2)&&(counttemp==0))
					bXorByte=NEWAMFHEADER.xkey;
				else {
					SetFilePointer(hTempFile,-1,0,FILE_END);
					ReadFile(hTempFile,&bXorByte,1,&dwRealWrite,NULL);}
				// пишем правленую секцию в конец “Ёћѕа
				switch(Xoring(hTempFile,lpSecBuffer,bXorByte,SecLen)){
					case X_CANT_WRITE:
						free(lpSecBuffer);
						CloseHandle(hTempFile);
						DeleteFile(tempPath);			// удаляем временный файл
						return DO_CANT_WRITE;
					case X_NO_MEMORY:
						free(lpSecBuffer);
						CloseHandle(hTempFile);
						DeleteFile(tempPath);			// удаляем временный файл
						return DO_NO_MEM;		}
				counttemp++;
				}

			}	// корректируем OAT
		free(lpSecBuffer);

		// переписываем данные без данных удаляемого объекта
			// 1 часть данных (данные перед удаляемым объектом)
		dwTransCounter=dwAllocStoringFile-((LPAMFHEADER->fcon*SecLen)+7);
		if(dwTransCounter){
			SetFilePointer(hAmfFile,(LPAMFHEADER->fcon*SecLen)+7,0,FILE_BEGIN);
			// буферизированная перекачка данных перед удаляемым объектом
			switch(DataPump(dwTransCounter,hAmfFile,hTempFile)){
				case DP_NO_MEM:
					CloseHandle(hTempFile);
					DeleteFile(tempPath);	// удаляем временный файл
					return DO_NO_MEM;
				case DP_CANT_WRITE:
					CloseHandle(hTempFile);
					DeleteFile(tempPath);	// удаляем временный файл
					return DO_CANT_WRITE;	}
			}
			// 2 часть данных (данные после удаляемого объекта)
		dwTransCounter=GetFileSize(hAmfFile,NULL)-(dwAllocStoringFile+dwSizeStoringFile);
		if(dwTransCounter){
			SetFilePointer(hAmfFile,dwAllocStoringFile+dwSizeStoringFile,0,FILE_BEGIN);
			// буферизированная перекачка данных после удаляемого объекта
			switch(DataPump(dwTransCounter,hAmfFile,hTempFile)){
				case DP_NO_MEM:
					CloseHandle(hTempFile);
					DeleteFile(tempPath);	// удаляем временный файл
					return DO_NO_MEM;
				case DP_CANT_WRITE:
					CloseHandle(hTempFile);
					DeleteFile(tempPath);	// удаляем временный файл
					return DO_CANT_WRITE;	}
			}

		}	// корректируем и переписываем OAT и данные (объекты)

	CloseHandle(hTempFile);			// узакрываем TEMP файл

	// процесс обновления AMF файла
	Close();						// закрываем AMF файл
	CopyFile(tempPath,bufFullPathAmf,FALSE);	// копируем временный файл в рабочий AMF
	DeleteFile(tempPath);			// удаляем временный файл
	Open(bufFullPathAmf,FALSE,1);	// открываем обновлЄнный AMF файл

	return DO_OK;
}

/////////////////////////////////////////////////////////////////////
//	»звлечение объекта или целой директории объектов из AMF файла и
//	сохранение их в желаемой директории
BYTE CAmf::Extract(LPSTR lpDestPath,LPSTR lpLogPath)
{
	if(!IsAmfOpen)return E_NO_AMF;
	if(!LPAMFHEADER->fcon)return E_NO_OBJ;

	char CurDirBuf[MAX_PATH];
	LPSTR lpCurDirBuf=CurDirBuf;	// буфер директории
	char ObjName[61];
	LPSTR lpObjName=ObjName;		// буфер имени объекта
	WORD wObjNum;					// номер объекта

	// проверка на существование каталога для сохранения
	// если таковой не существует, то создаЄм
	if(strlen(lpDestPath)){
		GetCurrentDirectory(MAX_PATH,lpCurDirBuf);	// сохраняем текущий каталог
		if(!SetCurrentDirectory(lpDestPath)){
			if(!CreateDirectory(lpDestPath,NULL))return E_CANT_CREATE_DIR;
			}
		SetCurrentDirectory(lpCurDirBuf);	// восстанавливаем текущий каталог
		}

	// извлечение объектов из корневого логического каталога
	// ћј– ≈– - lpLogPath==""
	// AMF v1 и 2
	if(!strlen(lpLogPath)){
		for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
			GetObjName(lpObjName,wObjNum);
			if(!strstr(lpObjName,"/")){
				ExtractObject(lpDestPath,wObjNum);
				}
			}
		return E_OK;
		}

	// извлечение объектов из логического каталога
	// ћј– ≈– - lpLogPath==".../"         последний символ слэш !!!
	// только для AMF версии 2
	if((LPAMFHEADER->amfv==2)&&(lpLogPath[strlen(lpLogPath)-1]=='/')){
		for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
			GetObjName(lpObjName,wObjNum);
			if(strstr(lpObjName,lpLogPath)==lpObjName){
				strncpy(lpCurDirBuf,lpObjName+strlen(lpLogPath),strlen(lpObjName)-strlen(lpLogPath));
				lpCurDirBuf[strlen(lpObjName)-strlen(lpLogPath)]='\0';
				if(!strstr(lpCurDirBuf,"/"))ExtractObject(lpDestPath,wObjNum);
				}

			}
		return E_OK;
		}

	// извлечение конкретного объекта
	// ћј– ≈– - lpLogPath==полный путь (если есть) и имя объекта
	for(wObjNum=1;wObjNum<=LPAMFHEADER->fcon;wObjNum++){
		GetObjName(lpObjName,wObjNum);
		if(!_stricmp(lpObjName,lpLogPath))return ExtractObject(lpDestPath,wObjNum);
		}
	return E_OBJ_NOTFOUND;
}

/////////////////////////////////////////////////////////////////////
//	»звлечение объекта (по номеру) из AMF файла и сохранение
//	его в отдельный файл
//	!!! каталог для сохранения должен существовать !!!
BYTE CAmf::ExtractObject(LPSTR lpDestPath,WORD wExObject)
{
	// если строка объекта пуста или больше допустимого, то выход
	if((!wExObject)||(wExObject>LPAMFHEADER->fcon))return EO_ZERO_OBJ;

	HANDLE hStoreFile;			// для дескриптора сохраняемого файла
	char PathToBuild[MAX_PATH];	// для пути и имени создаваемого файла
	char* lpNameBuildFile;		// для имени создаваемого файла
	char* lpTokenTemp;			// буфер поиска

	char ObjName[61];			// для имени объекта
	char* lpObjName;			// адрес имени объекта
	lpObjName=ObjName;			// инициализация

	// находим длину и расположение желаемого объекта в AMF файле
	DWORD dwSizeStoringFile,	// размер
		  dwAllocStoringFile;	// расположение
	BOOL bGetResult=GetObjSizeAlloc(&dwSizeStoringFile,
								    &dwAllocStoringFile,
								    wExObject);
	if(!bGetResult)return EO_CANT_GET_SIZEALLOC;

	// выбираем каталог
	if(lpDestPath){
		strcpy(PathToBuild,lpDestPath);	// параметр lpDestPath - строка
		}
	else strcpy(PathToBuild,"");		// параметр lpDestPath - NULL

	// проверка на наличие завершающего слэша в пути.
	// если нет, то поставить.
	BYTE LenthBuildPath=strlen(PathToBuild);
	if(LenthBuildPath){
		if(PathToBuild[LenthBuildPath-1]!='\\')
			strcat(PathToBuild,"\\");
		}

	// получаем имя объекта
	if(!GetObjName(lpObjName,wExObject))return EO_CANT_GET_OBJNAME;

	// присоединяем к пути имя файла
	if(LPAMFHEADER->amfv==1){
		strcat(PathToBuild,lpObjName);
		}
	else {
		// выделяем имя объекта из логического пути
		lpTokenTemp=strtok(lpObjName,"/");
		while(lpTokenTemp)
			{
			lpNameBuildFile=lpTokenTemp;
			lpTokenTemp=strtok(NULL,"/");
			}
		strcat(PathToBuild,lpNameBuildFile);
		}

	// создаЄм файл по пути для вывода объекта
	// если файл с таким именем существует, то он будет перезаписан
	hStoreFile=CreateFile(
		PathToBuild,
		GENERIC_WRITE,
		NULL,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL);
		// невозможно создать файл
		if(hStoreFile==INVALID_HANDLE_VALUE)return EO_CANT_CREATE;

		// позиция на начало данных объекта
	SetFilePointer(hAmfFile,dwAllocStoringFile,NULL,FILE_BEGIN);
		// перекачиваем данные
	switch(DataPump(dwSizeStoringFile,hAmfFile,hStoreFile)){
		case DP_NO_MEM:
			CloseHandle(hStoreFile);
			DeleteFile(PathToBuild);	// удаляем временный файл
			return EO_NO_MEM;
		case DP_CANT_WRITE:
			CloseHandle(hStoreFile);
			DeleteFile(PathToBuild);	// удаляем временный файл
			return EO_CANT_WRITE;
			}

	CloseHandle(hStoreFile);
	return EO_OK;
}

/////////////////////////////////////////////////////////////////////
//	найти размер и месторасположения объекта указанного 
//	в числовом	значении
//	для AMF версий 1 и 2
BOOL CAmf::GetObjSizeAlloc(LPDWORD lpSize,LPDWORD lpAlloc,WORD wNumObj)
{
	if(!LPAMFHEADER->fcon)return FALSE;
	if((!wNumObj)||(wNumObj>LPAMFHEADER->fcon))return FALSE;

	BYTE bSecLenth;
	BYTE bXor;
	DWORD dwRealRead;

	if(LPAMFHEADER->amfv==1)bSecLenth=20;
	else bSecLenth=68;

	char bufferDW[8];
	char* bufDW;
	bufDW=bufferDW;
	LPDWORD lpbufDW;

	// указатель на XOR ключ
	SetFilePointer(hAmfFile,((bSecLenth*wNumObj)+7)-9,0,FILE_BEGIN);
	// читаем XOR ключ
	ReadFile(hAmfFile,&bXor,1,&dwRealRead,NULL);
	// распаковываем массив с данными в формате char
	if(DeXoring(bufDW,bXor,8))return FALSE;

	// переводим данные в формат DWORD
	lpbufDW=(LPDWORD)bufDW;
	(*lpSize)=lpbufDW[0];
	(*lpAlloc)=lpbufDW[1];

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//	ѕолучение строки содержащей каталог временных файлов класса CAmf
LPSTR CAmf::GetTemporaryPath(void)
{
	return bufFullTempPath;
}

//////////////////////////////////////////////////////////////////////
//	”становка строки содержащей каталог временных файлов класса CAmf
//	¬озвращаемое значение: OK - TRUE, error - FALSE
BOOL CAmf::SetTemporaryPath(LPSTR lpNewPath)
{
	// если длина желаемой строки превосходит MAX_PATH, то выход
	if(strlen(lpNewPath)>MAX_PATH)return FALSE;

	char bufText[MAX_PATH];
	strcpy(bufText,lpNewPath);

	// установка пользователем каталога для TEMP файла
	if(!strlen(lpNewPath)){
		strcpy(bufFullTempPath,bufText);
		return TRUE;
		}

	if(strlen(lpNewPath)<MAX_PATH){
		if(lpNewPath[strlen(lpNewPath)-1]!='\\')
			strcat(bufText,"\\");
			strcpy(bufFullTempPath,bufText);
			return TRUE;
		}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////
//	ѕолучение размера буфера для перекачки больших объЄмов данных
DWORD CAmf::GetBoosterSize(void)
{
	return dwDataBooster;
}

/////////////////////////////////////////////////////////////////////
//	”становка длины буфера для парекачки больших объЄмов данных
//	¬озвращаемое значение: TRUE - OK, FALSE - error
BOOL CAmf::SetBoosterSize(DWORD dwNewSize)
{
	if(dwNewSize){
		dwDataBooster=dwNewSize;
		return TRUE;
	}
	else return FALSE;		// если устанавливаемое число NULL
}

/////////////////////////////////////////////////////////////////////
//	ѕолучение имени объекта (с логическим путЄм) по порядковому номеру
//	из таблицы объектов
//	bStrLen - длина буфера lpRetName (с Null Terminate символом)
//	lpRetName - буфер для получаемого имени объекта (размер должен
//				быть равен bStrLen)
//	wObjNum - порядковый номер объекта в ќј“
//	¬озвращаемые значения:
//	TRUE - OK, FALSE - ошибка
BOOL CAmf::GetObjectName(BYTE bStrLen,LPSTR lpRetName,WORD wObjNum)
{
	if(!IsAmfOpen)return FALSE;		// AMF файл не открыт
	if((bStrLen==NULL)||(bStrLen==1))return FALSE;
	if(!LPAMFHEADER->fcon)return FALSE;	// AMF файл не содержит объектов

	BYTE bTempStrLen;				// длина временного буфера имени
	char *strbuf;					// временный буфер имени

	bTempStrLen=(LPAMFHEADER->amfv==1)?13:61;	// устанавливаем длину сектора имени секции ќј“
	strbuf=(char*)malloc(bTempStrLen);	// пытаемся выделить память под временный буфер
	if(!strbuf)return FALSE;			// недостаточно памяти
	if(!GetObjName(strbuf,wObjNum)){free(strbuf);return FALSE;}
	if(bStrLen>=bTempStrLen)strcpy(lpRetName,strbuf);
		else {
		strncpy(lpRetName,strbuf,bStrLen-1);
		lpRetName[bStrLen-1]='\0';
		}

	free(strbuf);	// высвобождаем память выделенную под временный буфер

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//	ѕолучение порядкового номера объекта в AMF файле, запрашиваемого
//	по имени(логическому пути)
//	<<< ¬џѕќЋЌя≈“—я ћ≈ƒЋ≈ЌЌќ >>>
//	LPSTR lpObjName - имя объекта
//	¬озвращаемое значение: ѕорядковый номер объекта в AMF файле,
//	если такой объект не найден, тогда возвращается NULL.
//	ѕри возникновении какой либо другой ошибке, также, возвращается NULL
WORD CAmf::GetObjectNum(LPSTR lpObjName)
{
	if(!IsAmfOpen)return NULL;		// AMF файл не открыт
	if(!LPAMFHEADER->fcon)return NULL;	// AMF файл не содержит объектов
	if(!strlen(lpObjName))return NULL;	// пустая строка параметра

	BYTE bTempStrLen;				// длина временного буфера имени
	WORD countobj=1;				// счЄтчик объектов
	char *strbuf;					// временный буфер имени

	bTempStrLen=(LPAMFHEADER->amfv==1)?13:61;	// устанавливаем длину сектора имени секции ќј“
	strbuf=(char*)malloc(bTempStrLen);	// пытаемся выделить память под временный буфер
	if(!strbuf)return NULL;				// недостаточно памяти

	while(countobj<=LPAMFHEADER->fcon){
		if(!GetObjName(strbuf,countobj)){free(strbuf);return NULL;}
		if(!_stricmp(strbuf,lpObjName)){
			free(strbuf);
			return countobj;		// возврат номера объекта
		}
		countobj++;
	}
	free(strbuf);

	return NULL;					// объект не найден
}

/////////////////////////////////////////////////////////////////////
//	ѕолучение атрибутов объекта, т.е. его размера и масторасположения
//	ѕараметры:
//	lpSize и lpAlloc - адреса для сохранения атрибутов
//	lpObjName - имя исследуемого объекта
//	wObjNum - номер исследуемого объекта
//	------------------------------------------------------------------
//	¬Ќ»ћјЌ»≈!!! ≈сли параметр wObjNum равен NULL или ошибочен, тогда
//	объект ищется по параметру lpObjName. ≈сли параметр wObjNum не 
//	равен NULL, то объект ищется по этому параметру!
//	------------------------------------------------------------------
BYTE CAmf::GetObjectAttr(LPDWORD lpSize,LPDWORD lpAlloc,LPSTR lpObjName,WORD wObjNum)
{
	if(!IsAmfOpen)return GOA_NO_AMF;	// AMF файл не открыт
	if(!LPAMFHEADER->fcon)return GOA_NO_OBJ;	// AMF файл не содержит объектов

	if((!wObjNum)||(wObjNum>LPAMFHEADER->fcon)){
		if(!strlen(lpObjName))return GOA_BAD_PARAMETERS;
		wObjNum=GetObjectNum(lpObjName);
		if(!wObjNum)return GOA_CANT_GET_NUM;
	}

	if(!GetObjSizeAlloc(lpSize,lpAlloc,wObjNum))return GOA_CANT_GET_ATTR;

	return GOA_OK;
}

///////////
HANDLE CAmf::GetHandle(void)
{
	return hAmfFile;
}
