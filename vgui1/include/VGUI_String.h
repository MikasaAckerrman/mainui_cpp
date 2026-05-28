#ifndef VGUI_STRING_H
#define VGUI_STRING_H

#include <VGUI.h>

namespace vgui
{

class String
{
public:
	String();
	String(const char* text);
	String(const String& src);
	~String();
public:
	int getCount();
	static int getCount(const char* text);
	String operator+(String text);
	String operator+(const char* text);
	bool operator==(String text);
	bool operator==(const char* text);
	char operator[](int index);
	const char* getChars();
	static void test();
private:
	char* _text;
};

}

#endif
