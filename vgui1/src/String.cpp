#include <VGUI_String.h>
#include <string.h>

namespace vgui
{

// Internal free function that uses the replaceable _free_func from vgui.cpp,
// matching allocations done by vgui_strdup which uses _malloc_func.
extern void vgui_internal_free(void* ptr);


String::String()
{
	_text = vgui_strdup("");
}

String::String(const char* text)
{
	_text = vgui_strdup(text ? text : "");
}

String::String(const String& src)
{
	_text = vgui_strdup(src._text ? src._text : "");
}

String::~String()
{
	if (_text)
	{
		vgui_internal_free(_text);
		_text = null;
	}
}

int String::getCount(const char* text)
{
	if (!text)
		return 0;
	return (int)strlen(text);
}

int String::getCount()
{
	return getCount(_text);
}

String String::operator+(String text)
{
	int len1 = getCount();
	int len2 = text.getCount();
	char* buf = new char[len1 + len2 + 1];
	if (_text)
		memcpy(buf, _text, len1);
	if (text._text)
		memcpy(buf + len1, text._text, len2);
	buf[len1 + len2] = 0;
	String result(buf);
	delete[] buf;
	return result;
}

String String::operator+(const char* text)
{
	int len1 = getCount();
	int len2 = text ? (int)strlen(text) : 0;
	char* buf = new char[len1 + len2 + 1];
	if (_text)
		memcpy(buf, _text, len1);
	if (text)
		memcpy(buf + len1, text, len2);
	buf[len1 + len2] = 0;
	String result(buf);
	delete[] buf;
	return result;
}

bool String::operator==(String text)
{
	if (!_text && !text._text)
		return true;
	if (!_text || !text._text)
		return false;
	return strcmp(_text, text._text) == 0;
}

bool String::operator==(const char* text)
{
	if (!_text && !text)
		return true;
	if (!_text || !text)
		return false;
	return strcmp(_text, text) == 0;
}

char String::operator[](int index)
{
	if (!_text || index < 0 || index >= getCount())
		return 0;
	return _text[index];
}

const char* String::getChars()
{
	return _text;
}

void String::test()
{
	// Empty test stub
}

}
