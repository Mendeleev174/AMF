/////////////////////////////////////////////////////////////////////
//		AMF Technology (возвращаемые значения и макросы)

#define GetRandom( min, max ) ((rand() % (int)(((max) + 1) - (min))) + (min))
#define MAX_VER_AMF		2				// максимальная поддерживаемая версия AMF
#define DATA_BUFFER_SIZE 1048576		// размер буфера для перекачки основных массивов данных

/////////// CAmf::Open
#define O_RW_OK			NULL			// OK (открыт для чтения-записи)
#define O_R_OK			1				// OK (открыт только для чтения)
#define O_OPENED		2				// уже открыт
#define O_ERROR_OPEN	3				// нельзя открыть AMF файл
#define O_CANT_CREATE	4				// нет доступа к файлу
#define O_NOT_AMF		5				// открытый файл не является носителем AMF технологии

/////////// CAmf::CreateAmf
#define CA_OK			NULL			// OK
#define CA_CANT_WRITE	1				// нельзя записать что-либо в AMF файл
#define CA_NO_MEMORY	2				// ошибка по причине нехватки памяти

/////////// CAmf::Xoring
#define X_OK			NULL			// OK
#define X_CANT_WRITE	1				// нельзя записать что-либо в AMF файл
#define X_NO_MEMORY		2				// недостаточно памяти
#define X_ZERO_SIZE		3				// недопустимое значение

/////////// CAmf::CheckID
#define CI_OK			NULL			// OK
#define CI_NO_MEMORY	1				// недостаточно памяти
#define CI_CANT_READ	2				// нельзя прочитать ID
#define CI_NOT_AMF		3				// файл не AMF

/////////// CAmf::DeXoring
#define DX_OK			NULL			// OK
#define DX_NO_MEMORY	1				// недостаточно памяти
#define DX_CANT_READ	2				// нельзя прочитать что-либо из AMF файла
#define DX_ZERO_SIZE	3				// недопустимое значение

/////////// CAmf::GetInfo
#define GI_OK			NULL			// OK
#define GI_AMFNOTOPENED	1				// AMF файл не открыт

/////////// CAmf::Add
#define A_OK			NULL			// OK
#define A_ALREADY_GET	1				// AMF файл уже содержит объект с таким именем
#define A_NO_MEM		2				// недостаточно памяти
#define A_CANT_CREATE_TEMP 3			// нельзя создать временный файл (с путём)
#define A_CANT_WRITE	4				// нельзя записать что-либо
#define A_CANT_OPEN_ADD 5				// нельзя открыть добавляемый файл
#define A_ZERO_ADD		6				// нулевой размер добавляемого файла
#define A_LOGPATH_TOOBIG 7				// генерируемый логический путь больше допустимого размера (60 байт)(только для AMF v2)
#define A_NO_FILES		8				// файлы не найдены
#define A_NO_AMF		9				// AMF файл не открыт
#define A_AMF_READ_ONLY 10				// AMF файл только для чтения

/////////// overloaded CAmf::AddEach
#define AE_OK			NULL			// OK
#define AE_ALREADY_GET	1				// AMF файл уже содержит объект с таким именем
#define AE_NO_MEM		2				// недостаточно памяти
#define AE_CANT_CREATE_TEMP 3			// нельзя создать временный файл (с путём)
#define AE_CANT_WRITE	4				// нельзя записать что-либо
#define AE_CANT_OPEN_ADD 5				// нельзя открыть добавляемый файл
#define AE_ZERO_ADD		6				// нулевой размер добавляемого файла
#define AE_LOGPATH_TOOBIG 7				// генерируемый логический путь больше допустимого размера (60 байт)(только для AMF v2)

/////////// CAmf::DataPump
#define DP_OK			NULL			// OK
#define DP_NO_MEM		1				// нельзя выделить требуемый блок памяти
#define DP_CANT_WRITE	2				// нельзя записать что-либо в файл

/////////// CAmf::ExtractObject
#define EO_OK			NULL			// OK
#define EO_ZERO_OBJ		1				// не указан объект для извлечения
#define EO_CANT_GET_SIZEALLOC 2			// нельзя узнать размер и расположение объекта
#define EO_CANT_GET_OBJNAME 3			// нельзя узнать имя объекта
#define EO_CANT_CREATE	4				// нельзя создать файл
#define EO_NO_MEM		5				// нехватка памяти для перекачки данных
#define EO_CANT_WRITE	6				// нельзя записать что-либо в файл (скорее всего нет месат на диске)

/////////// CAmf::Extract
#define E_OK			NULL			// OK
#define E_ZERO_OBJ		1				// не указан объект для извлечения
#define E_CANT_GET_SIZEALLOC 2			// нельзя узнать размер и расположение объекта
#define E_CANT_GET_OBJNAME 3			// нельзя узнать имя объекта
#define E_CANT_CREATE	4				// нельзя создать файл
#define E_NO_MEM		5				// нехватка памяти для перекачки данных
#define E_CANT_WRITE	6				// нельзя записать что-либо в файл (скорее всего нет месат на диске)
#define E_CANT_CREATE_DIR 7				// нельзя создать каталог
#define E_NO_AMF		8				// AMF файл не открыт
#define E_NO_OBJ		9				// текущий AMF файл не содержит объекты
#define E_OBJ_NOTFOUND	10				// объект не найден

/////////// CAmf::Delete
#define D_OK			NULL			// OK
#define DO_CANT_CREATE_TEMP 1			// нельзя создать TEMP файл
#define DO_CANT_WRITE	2				// нельзя записать что-либо в файл
#define DO_NO_MEM		3				// нехватка памяти для перекачки данных
#define DO_CANT_GET_SIZEALLOC 4			// нельзя узнать размер и расположение объекта
#define D_NO_AMF		5				// AMF файл не открыт
#define D_NO_OBJ		6				// текущий AMF файл не содержит объекты
#define D_AMF_READ_ONLY 7				// текущий AMF файл открыт только для чтения
#define D_OBJ_NOTFOUND	8				// объект не найден

/////////// CAmf::DeleteObj
#define DO_OK			NULL			// OK
#define DO_CANT_CREATE_TEMP 1			// нельзя создать TEMP файл
#define DO_CANT_WRITE	2				// нельзя записать что-либо в файл
#define DO_NO_MEM		3				// нехватка памяти для перекачки данных
#define DO_CANT_GET_SIZEALLOC 4			// нельзя узнать размер и расположение объекта

/////////// CAmf::GetObjectAttr
#define GOA_OK			NULL			// OK
#define GOA_NO_AMF		1				// AMF файл не открыт
#define GOA_NO_OBJ		2				// текущий AMF файл не содержит объекты
#define GOA_BAD_PARAMETERS 3			// нет ни одного корректного параметра
#define GOA_CANT_GET_NUM 4				// нельзя получить номер объекта
#define GOA_CANT_GET_ATTR 5				// нельзя получить атрибуты объекта
