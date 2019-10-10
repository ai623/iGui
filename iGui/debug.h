#pragma once
#include <Windows.h>
#include <crtdbg.h>
#include <string>

namespace iCommon {
	struct Debug {
		bool isOn = true;
		Debug() = default;
		Debug(bool on) { isOn = on; }

		void operator() (std::string str);
		void error(std::string str);
		void error(std::string str, HRESULT errorCode);
		bool hasMemoryLeaks();
	};
}
