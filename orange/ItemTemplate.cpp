#include "stdafx.h"
#include ".\\ItemTemplate.h"
#include ".\\DataBase.h"

CItemTemplate ItemTemplate;

CItemTemplate::CItemTemplate()
{
	this->template_container.clear();
}

bool CItemTemplate::Load()
{
	QSqlQuery q;
	MainDB.Lock();
	if(!q.exec("SELECT `type`, `slot`, `skill`, `width`, `height`, `option`, `drop`, `name`, `level`, `reqlevel`, `durability`, `SPECIAL`, `strength`, `dexterity`, `energy`, `vitality`, `leadership`, `set_option`, `CLASS_ATTR` FROM `item_template`;"))
	{
		printf_s("Something wrong with `item_template`.\n");
		return false;
	}
	MainDB.Unlock();
	uint16 index = 0xFF;
	ITEM_TEMPLATE it;
	mtx.lock();
	while(q.next())
	{
		it.type = q.value(0).toUInt();
		it.slot = q.value(1).toInt();
		it.skill = q.value(2).toUInt();
		it.width = q.value(3).toUInt();
		it.height = q.value(4).toUInt();
		it.option = q.value(5).toBool();
		it.drop = q.value(6).toBool();
		it.name = q.value(7).toString().toStdString();
		it.level = q.value(8).toUInt();
		it.reqlevel = q.value(9).toUInt();
		it.durability = q.value(10).toUInt();
		it.SPECIAL = q.value(11).toULongLong();
		it.strength = q.value(12).toUInt();
		it.dexterity = q.value(13).toUInt();
		it.energy = q.value(14).toUInt();
		it.vitality = q.value(15).toUInt();
		it.leadership = q.value(16).toUInt();
		it.set_option = q.value(17).toUInt();
		it.CLASS_ATTR = q.value(18).toUInt();
		this->template_container.insert(MapType::value_type(it.type, it));
		ZeroMemory(&it, sizeof(ITEM_TEMPLATE));
	}
	mtx.unlock();
	return true;
}

const ITEM_TEMPLATE* CItemTemplate::Get(uint16 type)
{
	if(type >= 0)
	{
		this->mtx.lock();
		this->mtx.unlock();
		MapType::iterator it = this->template_container.find(type);
		if(it != this->template_container.end())
		{
			return &(it->second);
		}
	}
	return NULL;
}

bool CItemTemplate::Reload()
{
	this->template_container.clear();
	return (this->Load());
}