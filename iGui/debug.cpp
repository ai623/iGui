#include "debug.h"


namespace iCommon {
	void Debug::operator()(std::string str)
	{
		MessageBoxA(NULL, str.c_str(), "Message", MB_OK);
	}

	void Debug::error(std::string str)
	{
		ValidateRect(NULL, NULL);
		MessageBoxA(NULL, str.c_str(), NULL, MB_OK);
		exit(-1);
	}

	void Debug::error(std::string str, HRESULT errorCode)
	{
		ValidateRect(NULL, NULL);
		MessageBoxA(NULL, (str + ", Error code: " + std::to_string(errorCode)).c_str(), NULL, MB_OK);
		exit(-1);
	}
}

