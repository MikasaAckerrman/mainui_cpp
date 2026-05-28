#ifndef VGUI_DAR_H
#define VGUI_DAR_H

#include <stdlib.h>
#include <string.h>
#include <VGUI.h>

namespace vgui
{

template<class ELEMTYPE> class Dar
{
public:
	Dar()
	{
		_count = 0;
		_capacity = 0;
		_data = null;
	}
	Dar(int initialCapacity)
	{
		_count = 0;
		_capacity = 0;
		_data = null;
		ensureCapacity(initialCapacity);
	}
	~Dar()
	{
		if (_data)
			free(_data);
	}
public:
	void ensureCapacity(int wantedCapacity)
	{
		if (wantedCapacity <= _capacity)
			return;
		int newCapacity = _capacity;
		if (newCapacity == 0)
			newCapacity = 4;
		while (newCapacity < wantedCapacity)
			newCapacity *= 2;
		ELEMTYPE* newData = (ELEMTYPE*)malloc(newCapacity * sizeof(ELEMTYPE));
		if (_data)
		{
			memcpy(newData, _data, _count * sizeof(ELEMTYPE));
			free(_data);
		}
		memset(newData + _count, 0, (newCapacity - _count) * sizeof(ELEMTYPE));
		_data = newData;
		_capacity = newCapacity;
	}
	void setCount(int count)
	{
		ensureCapacity(count);
		_count = count;
	}
	int getCount()
	{
		return _count;
	}
	void addElement(ELEMTYPE elem)
	{
		ensureCapacity(_count + 1);
		_data[_count] = elem;
		_count++;
	}
	void putElement(ELEMTYPE elem)
	{
		addElement(elem);
	}
	void insertElementAt(ELEMTYPE elem, int index)
	{
		if (index < 0) index = 0;
		if (index > _count) index = _count;
		ensureCapacity(_count + 1);
		memmove(_data + index + 1, _data + index, (_count - index) * sizeof(ELEMTYPE));
		_data[index] = elem;
		_count++;
	}
	void setElementAt(ELEMTYPE elem, int index)
	{
		if (index >= 0 && index < _count)
			_data[index] = elem;
	}
	void removeElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
			{
				removeElementAt(i);
				return;
			}
		}
	}
	void removeElementAt(int index)
	{
		if (index < 0 || index >= _count)
			return;
		memmove(_data + index, _data + index + 1, (_count - index - 1) * sizeof(ELEMTYPE));
		_count--;
	}
	void removeAll()
	{
		_count = 0;
	}
	ELEMTYPE operator[](int index)
	{
		if (index < 0 || index >= _count)
			return (ELEMTYPE)0;
		return _data[index];
	}
	bool hasElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
				return true;
		}
		return false;
	}
	int findElement(ELEMTYPE elem)
	{
		for (int i = 0; i < _count; i++)
		{
			if (_data[i] == elem)
				return i;
		}
		return -1;
	}
protected:
	ELEMTYPE* _data;
	int _count;
	int _capacity;
};

}

#endif
