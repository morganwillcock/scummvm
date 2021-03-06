/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef SUPERNOVA_SUPERNOVA_H
#define SUPERNOVA_SUPERNOVA_H

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "audio/decoders/raw.h"
#include "common/array.h"
#include "common/events.h"
#include "common/random.h"
#include "common/scummsys.h"
#include "engines/engine.h"
#include "common/file.h"
#include "common/memstream.h"

#include "supernova/console.h"
#include "supernova/graphics.h"
#include "supernova/msn_def.h"
#include "supernova/rooms.h"


namespace Supernova {

#define SAVEGAME_HEADER MKTAG('M','S','N','1')
#define SAVEGAME_VERSION 8

#define SUPERNOVA_DAT "supernova.dat"
#define SUPERNOVA_DAT_VERSION 1


struct ScreenBuffer {
	ScreenBuffer()
		: _x(0)
		, _y(0)
		, _width(0)
		, _height(0)
		, _pixels(NULL)
	{}

	byte *_pixels;
	int _x;
	int _y;
	int _width;
	int _height;
};
class ScreenBufferStack {
public:
	ScreenBufferStack();

	void push(int x, int y, int width, int height);
	void restore();

private:
	ScreenBuffer _buffer[8];
	ScreenBuffer *_last;
};

struct SoundSample {
	SoundSample()
	    : _buffer(NULL)
	    , _length(0)
	{}

	~SoundSample() {
		delete[] _buffer;
	}

	byte *_buffer;
	int _length;
};

class SupernovaEngine : public Engine {
public:
	explicit SupernovaEngine(OSystem *syst);
	~SupernovaEngine();

	virtual Common::Error run();

	Common::RandomSource _rnd;
	GameManager *_gm;
	Console *_console;
	Audio::SoundHandle _soundHandle;
	ScreenBufferStack _screenBuffer;
	byte _mouseNormal[256];
	byte _mouseWait[256];
	MSNImageDecoder *_currentImage;
	SoundSample _soundSamples[kAudioNumSamples];
	Common::MemoryReadStream *_soundMusicIntro;
	Common::MemoryReadStream *_soundMusicOutro;
	int _screenWidth;
	int _screenHeight;
	bool _allowLoadGame;
	bool _allowSaveGame;
	Common::StringArray _gameStrings;
	Common::String _nullString;

	byte _menuBrightness;
	byte _brightness;
	uint _delay;
	bool _messageDisplayed;
	int  _textSpeed;
	int  _textCursorX;
	int  _textCursorY;
	int  _textColor;

	int  textWidth(const char *text);
	int  textWidth(const uint16 key);
	Common::Error loadGameStrings();
	void initData();
	void initPalette();
	void paletteFadeIn();
	void paletteFadeOut();
	void paletteBrightness();
	void updateEvents();
	void playSound(AudioIndex sample);
	void playSoundMod(int filenumber);
	void stopSound();
	void renderImageSection(int section);
	void renderImage(int section);
	bool setCurrentImage(int filenumber);
	void saveScreen(int x, int y, int width, int height);
	void restoreScreen();
	void renderRoom(Room &room);
	void renderMessage(const char *text, MessagePosition position = kMessageNormal);
	void removeMessage();
	void renderText(const char *text, int x, int y, byte color);
	void renderText(const uint16 character, int x, int y, byte color);
	void renderText(const char *text);
	void renderText(const uint16 character);
	void renderBox(int x, int y, int width, int height, byte color);
	void setColor63(byte value);
	bool loadGame(int slot);
	bool saveGame(int slot, const Common::String &description);
	void errorTempSave(bool saving);
	void setTextSpeed();

	const Common::String &getGameString(int idx) const {
		if (idx < 0 || idx >= (int)_gameStrings.size())
			return _nullString;
		return _gameStrings[idx];
	}

	void setGameString(int idx, const Common::String &string) {
		if (idx < 0)
			return;
		while ((int)_gameStrings.size() <= idx)
			_gameStrings.push_back(Common::String());
		_gameStrings[idx] = string;
	}

	int  textWidth(const Common::String &text) {
		if (text.empty())
			return 0;
		return textWidth(text.c_str());
	}
	void renderMessage(StringID stringId, MessagePosition position = kMessageNormal, Common::String var1 = "", Common::String var2 = "") {
		Common::String text = getGameString(stringId);
		if (!var1.empty()) {
			if (!var2.empty())
				text = Common::String::format(text.c_str(), var1.c_str(), var2.c_str());
			else
				text = Common::String::format(text.c_str(), var1.c_str());
		}
		renderMessage(text, position);
	}
	void renderMessage(const Common::String &text, MessagePosition position = kMessageNormal) {
		if (!text.empty())
			renderMessage(text.c_str(), position);
	}
	void renderText(StringID stringId, int x, int y, byte color) {
		renderText(getGameString(stringId), x, y, color);
	}
	void renderText(const Common::String &text, int x, int y, byte color) {
		if (!text.empty())
			renderText(text.c_str(), x, y, color);
	}
	void renderText(StringID stringId) {
		renderText(getGameString(stringId));
	}
	void renderText(const Common::String &text) {
		if (!text.empty())
			renderText(text.c_str());
	}

	Common::MemoryReadStream *convertToMod(const char *filename, int version = 1);

	virtual Common::Error loadGameState(int slot);
	virtual bool canLoadGameStateCurrently();
	virtual Common::Error saveGameState(int slot, const Common::String &desc);
	virtual bool canSaveGameStateCurrently();
	virtual bool hasFeature(EngineFeature f) const;
	virtual void pauseEngineIntern(bool pause);
};

}

#endif
