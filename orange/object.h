/*
	Copyright 2008-2009 Ambient.5

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <vector>

enum OBJECT_TYPE
{
	VOID_BOT = -4,
	VOID_EMPTY = -3,
	VOID_UNIT = -2,
	VOID_PLAYER = -1,
	OBJECT_EMPTY = 0,
	OBJECT_UNIT = 1,
	OBJECT_PLAYER = 2,
	OBJECT_BOT = 3,
};

class CObject
{
public:
	short guid;
	unsigned char x;
	unsigned char y;
	uint8 x_old;
	uint8 y_old;
	unsigned char target_x;
	unsigned char target_y;
	unsigned char map;
	unsigned char dir;
	unsigned char action;
	unsigned char state;
	unsigned char viewstate;
	unsigned char viewskillstate;
	bool teleporting;
	OBJECT_TYPE type;

	uint16 level;
	uint16 strength;
	uint16 dexterity;
	uint16 vitality;
	uint16 energy;
	uint16 leadership;
	float life;
	float maxlife;
	float mana;
	float maxmana;
	float shield;
	float maxshield;
	float bp;
	float maxbp;

	std::vector<uint16> viewport;

	CObject();
	bool InViewport(CObject* obj);
	void SendToViewport(unsigned char* buffer, size_t len);
};

#endif