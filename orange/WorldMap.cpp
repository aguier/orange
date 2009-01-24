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

#include "stdafx.h"
#include ".\\objectmanager.h"
#include ".\\packets.h"
#include ".\\WorldMap.h"

CWorldMap WorldMap[MAX_MAPS];

CWorldMap::CWorldMap()
{
	ZeroMemory(this->map, sizeof(this->map));
	//this->objects.resize(300);
	//this->objects.reserve(300);
	this->guids.clear();
	this->map_number = -1;
	this->width = -1;
	this->height = -1;
	this->last_update = GetTickCount();
}

void CWorldMap::LoadMap(const char * filename)
{
	FILE* mapfile;
	unsigned char head;
	fopen_s(&mapfile, filename, "rb");
	if(mapfile)
	{
		fread_s(&head, sizeof(head), 1, 1, mapfile);
		fread_s(&this->width, sizeof(this->width), 1, 1, mapfile);
		fread_s(&this->height, sizeof(this->height), 1, 1, mapfile);
		fread_s(this->map, sizeof(this->map), 0x100, 0x100, mapfile);
		fclose(mapfile);
	}
	else
	{
		printf_s("Error: %s notfound.\n", filename);
	}
}

void CWorldMap::UpdateMap()
{
	//this->guids_mutex.lock();
	this->guids.clear();
	ObjManager.mtx.lock();
	for(CObjectManager::MapType::iterator it = ObjManager.container.begin(); it != ObjManager.container.end(); ++it)
	{
		CObject* object = it->second;
		if((object) && ((object->type > OBJECT_EMPTY) && (object->map == this->map_number)))
		{
			this->guids.push_back(object->guid);
		}
	}
	ObjManager.mtx.unlock();
	//this->guids_mutex.unlock();
	//printf_s("Map %d has %d objects.\n", this->map_number, this->objects.size());
}

void CWorldMap::Run()
{
	this->MapThread.start();
}

void CWorldMap::Quit()
{
	this->MapThread.quit();
}

void CMapThread::run()
{
	CWorldMap * map = (CWorldMap*)lpmap;
	while(TRUE)
	{
		if((GetTickCount() - map->last_update) >= 1000)
		{
			map->UpdateMap();
			for(uint32 i = 0; i < map->guids.size(); ++i)
			{
				CObject* object = ObjManager.FindByGuid(map->guids.at(i));
				if((object != NULL) && (object->type == OBJECT_PLAYER))
				{
					map->UpdateViewport(object);
					if((GetTickCount() - ((CPlayer*)object)->last_save_time) >= (5 * 1000 * 60))
					{
						if(((CPlayer*)object)->status == PLAYER_PLAYING)
						{
							((CPlayer*)object)->SavePlayer();
						}
					}
				}
				else if((object != NULL) && (object->type == OBJECT_BOT))
				{
					map->UpdateViewport(object);
					((CBot*)object)->UpdateAI();
				}
			}
			map->last_update = GetTickCount();
		}
		Sleep(100);
	}
}

void CWorldMap::UpdateViewport(CObject* pobj)
{
	std::vector<uint16> view_delete;
	std::vector<uint16> view_create;
	view_delete.clear();
	for(uint32 i = 0; i < pobj->viewport.size(); ++i)
	{
		CObject * object = ObjManager.FindByGuid(pobj->viewport.at(i));
		if((!object) || (object->type < OBJECT_UNIT) || (object->map != pobj->map) || /*!((abs(pobj->x - object->x) <= 9) && (abs(pobj->y - object->y) <= 9))*/ !InFrustum(pobj->x, pobj->y, object->x, object->y))
		{
			printf_s("[VIEWPORT] %d deletes %d from viewport\n", pobj->guid, pobj->viewport.at(i));
			view_delete.push_back(pobj->viewport.at(i));
			pobj->viewport.erase(pobj->viewport.begin() + i);
		}
	}
	for(uint32 i = 0; i < this->guids.size(); ++i)
	{
		CObject * object = ObjManager.FindByGuid(this->guids.at(i));
		if((object) && (object->type > OBJECT_EMPTY) && (!pobj->InViewport(object)) && (object->map == pobj->map) && /*((abs(pobj->x - object->x) <= 9) && (abs(pobj->y - object->y) <= 9))*/ InFrustum(pobj->x, pobj->y, object->x, object->y) && (object != pobj))
		{
			printf_s("[VIEWPORT] %d inserts in his viewport %d\n", pobj->guid, this->guids.at(i));
			view_create.push_back(this->guids.at(i));
			pobj->viewport.push_back(this->guids.at(i));
		}
	}
	
	if(pobj->type != OBJECT_PLAYER)
	{
		return;
	}

	if(!view_delete.empty())
	{
		uint8 destroy_buffer[5000];
		ZeroMemory(destroy_buffer, sizeof(destroy_buffer));

		for(uint32 i = 0; i < view_delete.size(); ++i)
		{
			PMSG_VIEWPORTDESTROY data_destroy;
			data_destroy.NumberH = HIBYTE(view_delete.at(i));
			data_destroy.NumberL = LOBYTE(view_delete.at(i));
			memcpy(&destroy_buffer[sizeof(PBMSG_COUNT) + i * sizeof(PMSG_VIEWPORTDESTROY)], &data_destroy, sizeof(PMSG_VIEWPORTDESTROY));
		}

		PBMSG_COUNT pcount;
		pcount.h.c = 0xC1;
		pcount.h.headcode = GC_VIEW_DESTROY;
		pcount.h.size = sizeof(PBMSG_COUNT) + view_delete.size() * sizeof(PMSG_VIEWPORTDESTROY);
		pcount.count = view_delete.size();
		memcpy(destroy_buffer, &pcount, sizeof(PBMSG_COUNT));
		((CPlayer*)pobj)->Send((unsigned char*)destroy_buffer, pcount.h.size);
	}
	if(!view_create.empty())
	{
		uint8 player_buffer[5000];
		ZeroMemory(player_buffer, sizeof(player_buffer));
		size_t player_count = 0;

		for(uint32 i = 0; i < view_create.size(); ++i)
		{
			CObject * object = ObjManager.FindByGuid(view_create.at(i));
			if(object)
			{
				switch(object->type)
				{
				case OBJECT_UNIT:
					{
						break;
					}
				case OBJECT_PLAYER:
					{
						CPlayer* obj_player = (CPlayer*)object;
						PMSG_VIEWPORTCREATE packet;
						packet.NumberH = HIBYTE(obj_player->guid);
						packet.NumberL = LOBYTE(obj_player->guid);
						if(obj_player->state && !obj_player->teleporting)
						{
							packet.NumberH |= 0x80;
						}
						packet.X = obj_player->x;
						packet.Y = obj_player->y;
						packet.TX = obj_player->target_x;
						packet.TY = obj_player->target_y;
						packet.ViewSkillState = obj_player->viewskillstate;
						packet.DirAndPkLevel = obj_player->dir * 0x10;
						packet.DirAndPkLevel |= obj_player->pklevel & 0xf;
						memcpy(packet.Id, obj_player->name, 10);
						memcpy(packet.CharSet, obj_player->charset, 18);
						memcpy(&player_buffer[sizeof(PWMSG_COUNT) + player_count * sizeof(PMSG_VIEWPORTCREATE)], &packet, sizeof(PMSG_VIEWPORTCREATE));
						player_count++;
						break;
					}
				case OBJECT_BOT:
					{
						CBot* obj_bot = (CBot*)object;
						PMSG_VIEWPORTCREATE packet;
						packet.NumberH = HIBYTE(obj_bot->guid);
						packet.NumberL = LOBYTE(obj_bot->guid);
						if(obj_bot->state && !obj_bot->teleporting)
						{
							packet.NumberH |= 0x80;
						}
						packet.X = obj_bot->x;
						packet.Y = obj_bot->y;
						packet.TX = obj_bot->target_x;
						packet.TY = obj_bot->target_y;
						packet.ViewSkillState = obj_bot->viewskillstate;
						packet.DirAndPkLevel = obj_bot->dir * 0x10;
						packet.DirAndPkLevel |= obj_bot->pklevel & 0xf;
						memcpy(packet.Id, obj_bot->name, 10);
						memcpy(packet.CharSet, obj_bot->charset, 18);
						memcpy(&player_buffer[sizeof(PWMSG_COUNT) + player_count * sizeof(PMSG_VIEWPORTCREATE)], &packet, sizeof(PMSG_VIEWPORTCREATE));
						player_count++;
						break;
					}
				}
			}
		}
		if(player_count > 0)
		{
			size_t size = sizeof(PWMSG_COUNT) + player_count * sizeof(PMSG_VIEWPORTCREATE);
			PWMSG_COUNT packet_player;
			packet_player.h.c = 0xC2;
			packet_player.h.headcode = GC_VIEW_PLAYER;
			packet_player.h.sizeH = HIBYTE(size);
			packet_player.h.sizeL = LOBYTE(size);
			packet_player.count = player_count;
			memcpy(player_buffer, &packet_player, sizeof(PWMSG_COUNT));
			((CPlayer*)pobj)->Send((unsigned char*)player_buffer, size);
		}
	}
}

unsigned char CWorldMap::GetAttr(int x, int y)
{
	return *(&map[0][0] + x + y*256);
}