//====== Copyright (c) 1996-2024, Ivan Suvorov and Valve Corporation, All rights reserved. =====
//
// Purpose: IV Director Controller. Based on Alien Swarm Director Controller.
//
//=============================================================================
#include "cbase.h"
#include "iv_director_control.h"
#include "iv_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(iv_director_control, CIV_Director_Control);

BEGIN_DATADESC(CIV_Director_Control)
	DEFINE_KEYFIELD(m_bWanderersStartEnabled, FIELD_BOOLEAN,	"wanderers"),
	DEFINE_KEYFIELD(m_bHordesStartEnabled, FIELD_BOOLEAN,	"hordes"),
	DEFINE_KEYFIELD(m_iDirectorSpawnTableType, FIELD_INTEGER, "spawntabletype"),
	DEFINE_KEYFIELD(m_iDirectorMinCommonNPCS, FIELD_INTEGER, "mincommonspawn"),
	DEFINE_KEYFIELD(m_iDirectorMaxCommonNPCS, FIELD_INTEGER, "maxcommonspawn"),
	DEFINE_KEYFIELD(m_iDirectorMaxSpecialNPCS, FIELD_INTEGER, "maxspecialspawn"),
	DEFINE_KEYFIELD(m_bDirectorSpecialsOnceAdded, FIELD_BOOLEAN, "specialsoncespawned"),
	DEFINE_KEYFIELD(m_fDirectorCommonMinRadius, FIELD_FLOAT, "mincommonspawnradius"),
	DEFINE_KEYFIELD(m_fDirectorCommonMaxRadius, FIELD_FLOAT, "maxcommonspawnradius"),
	DEFINE_KEYFIELD(m_fDirectorSpawnerWanderInterval, FIELD_FLOAT, "directorwanderinterval"),
	DEFINE_KEYFIELD(m_bDirectorHordeFrontState, FIELD_BOOLEAN, "hordefrontstate"),
	DEFINE_INPUTFUNC(FIELD_VOID,	"EnableHordes",	InputEnableHordes),
	DEFINE_INPUTFUNC(FIELD_VOID,	"DisableHordes",	InputDisableHordes),
	DEFINE_INPUTFUNC(FIELD_VOID,	"EnableWanderers",	InputEnableWanderers),
	DEFINE_INPUTFUNC(FIELD_VOID,	"DisableWanderers",	InputDisableWanderers),
	DEFINE_INPUTFUNC(FIELD_VOID,	"StartFinale",	InputStartFinale),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetSpawnTableType", InputSetDirectorSpawnTableType),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMinCommonNPCS", InputSetMinCommonNPCS),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMaxCommonNPCS", InputSetMaxCommonNPCS),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetMaxSpecialNPCS", InputSetMaxSpecialNPCS),
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "SetSpecialNPCSOnceState", InputSetSpecialNPCSOnceState),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetMinCommonNPCSSpawnRadius", InputSetMinCommonNPCSSpawnRadius),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetMaxCommonNPCSSpawnRadius", InputSetMaxCommonNPCSSpawnRadius),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetWanderSpawnInterval", InputSetWanderSpawnInterval),
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "SetFrontHordeState", InputSetDirectorHordeFrontState),
	DEFINE_OUTPUT(m_OnFinaleEventStart, "OnFinaleStart"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::Precache()
{
	BaseClass::Precache();
}

void CIV_Director_Control::OnFinaleStarted(CBasePlayer *pPlayer)
{
	m_OnFinaleEventStart.FireOutput(pPlayer, this);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::InputEnableHordes(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetHordesEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::InputDisableHordes(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetHordesEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::InputEnableWanderers(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetWanderersEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::InputDisableWanderers(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetWanderersEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::InputStartFinale(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->StartFinale();
}

void CIV_Director_Control::InputSetDirectorSpawnTableType(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetSpawnTableType(inputdata.value.Int());
}

void CIV_Director_Control::InputSetMinCommonNPCS(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetMinCommonNPCS(inputdata.value.Int());
}

void CIV_Director_Control::InputSetMaxCommonNPCS(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetMaxCommonNPCS(inputdata.value.Int());
}

void CIV_Director_Control::InputSetMaxSpecialNPCS(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetMaxSpecialNPCS(inputdata.value.Int());
}

void CIV_Director_Control::InputSetSpecialNPCSOnceState(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetSpecialNPCSOnceState(inputdata.value.Bool());
}

void CIV_Director_Control::InputSetMinCommonNPCSSpawnRadius(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetMinCommonNPCSSpawnRadius(inputdata.value.Float());
}

void CIV_Director_Control::InputSetMaxCommonNPCSSpawnRadius(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetMaxCommonNPCSSpawnRadius(inputdata.value.Float());
}

void CIV_Director_Control::InputSetWanderSpawnInterval(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetSpawnerWanderInterval(inputdata.value.Float());
}

void CIV_Director_Control::InputSetDirectorHordeFrontState(inputdata_t &inputdata)
{
	if (!IVDirector())
		return;

	IVDirector()->SetNPCSHordeFrontState(inputdata.value.Bool());
}