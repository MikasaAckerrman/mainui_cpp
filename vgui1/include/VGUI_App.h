#ifndef VGUI_APP_H
#define VGUI_APP_H

#include <VGUI.h>
#include <VGUI_Dar.h>

namespace vgui
{

class Panel;
class SurfaceBase;
class Scheme;
class TickSignal;
class Cursor;

class App
{
public:
	App();
	App(bool externalMain);
	virtual ~App();
public:
	static App* getInstance();
	virtual void start();
	virtual void stop();
	virtual void externalTick();
	virtual bool wasMousePressed(MouseCode code, Panel* panel);
	virtual bool wasMouseDoublePressed(MouseCode code, Panel* panel);
	virtual bool isMouseDown(MouseCode code, Panel* panel);
	virtual bool wasMouseReleased(MouseCode code, Panel* panel);
	virtual bool wasKeyPressed(KeyCode code, Panel* panel);
	virtual bool isKeyDown(KeyCode code, Panel* panel);
	virtual bool wasKeyTyped(KeyCode code, Panel* panel);
	virtual bool wasKeyReleased(KeyCode code, Panel* panel);
	virtual void requestFocus(Panel* panel);
	virtual Panel* getFocus();
	// Clear any focus/capture/arena pointer that references panel. MUST be
	// called when a Panel is destroyed, otherwise the next input event
	// dereferences a freed Panel and crashes.
	virtual void panelDeleted(Panel* panel);
	virtual void setMouseCapture(Panel* panel);
	virtual void setMouseArena(Panel* panel);
	virtual void setMouseArena(int x0, int y0, int x1, int y1, bool enabled);
	virtual void surfaceBaseCreated(SurfaceBase* surfaceBase);
	virtual void surfaceBaseDeleted(SurfaceBase* surfaceBase);
	virtual void setScheme(Scheme* scheme);
	virtual Scheme* getScheme();
	virtual void enableBuildMode();
	virtual long getTimeMillis();
	virtual char getKeyCodeChar(KeyCode code, bool shifted);
	virtual void getKeyCodeText(KeyCode code, char* buf, int buflen);
	virtual int getClipboardTextCount();
	virtual void setClipboardText(const char* text, int textLen);
	virtual int getClipboardText(int offset, char* buf, int bufLen);
	virtual void reset();
	virtual void addTickSignal(TickSignal* s);
	virtual void setCursorPos(int x, int y);
	virtual void getCursorPos(int& x, int& y);
	virtual void internalCursorMoved(int x, int y, SurfaceBase* surfaceBase);
	virtual void internalMousePressed(MouseCode code, SurfaceBase* surfaceBase);
	virtual void internalMouseDoublePressed(MouseCode code, SurfaceBase* surfaceBase);
	virtual void internalMouseReleased(MouseCode code, SurfaceBase* surfaceBase);
	virtual void internalMouseWheeled(int delta, SurfaceBase* surfaceBase);
	virtual void internalKeyPressed(KeyCode code, SurfaceBase* surfaceBase);
	virtual void internalKeyTyped(KeyCode code, SurfaceBase* surfaceBase);
	virtual void internalCharTyped(char ch, SurfaceBase* surfaceBase);
	virtual void internalKeyReleased(KeyCode code, SurfaceBase* surfaceBase);
	virtual void internalSetMouseArena(int x0, int y0, int x1, int y1, bool enabled);
	virtual bool setRegistryString(const char* key, const char* value);
	virtual bool getRegistryString(const char* key, char* value, int valueLen);
	virtual bool setRegistryInteger(const char* key, int value);
	virtual bool getRegistryInteger(const char* key, int& value);
	virtual void setCursorOveride(Cursor* cursor);
	virtual Cursor* getCursorOveride();
	virtual void setMinimumTickMillisInterval(int interval);
	virtual void run();
	virtual void repaintAll();
private:
	void init();
	void initFields(bool externalMain); // shared by both ctors
	void updateMouseFocus(int x, int y, SurfaceBase* surfaceBase);
	void setMouseFocus(Panel* newMouseFocus);
	void platTick();
	void internalTick();
private:
	static App* _instance;
	Panel* _keyFocus;
	Panel* _oldMouseFocus;
	Panel* _mouseFocus;
	Panel* _mouseCapture;
	Panel* _wantedKeyFocus;
	Panel* _mouseArenaPanel;
	Scheme* _scheme;
	Cursor* _cursorOveride;
	Dar<SurfaceBase*> _surfaceBaseDar;
	Dar<TickSignal*> _tickSignalDar;
	bool _running;
	bool _externalMain;
	bool _buildMode;
	bool _wantedBuildMode;
	bool _mousePressed[MOUSE_LAST];
	bool _mouseDoublePressed[MOUSE_LAST];
	bool _mouseDown[MOUSE_LAST];
	bool _mouseReleased[MOUSE_LAST];
	bool _keyPressed[KEY_LAST];
	bool _keyTyped[KEY_LAST];
	bool _keyDown[KEY_LAST];
	bool _keyReleased[KEY_LAST];
	int _cursor[2];
	long _nextTickMillis;
	long _minimumTickMillisInterval;
};

}

#endif
