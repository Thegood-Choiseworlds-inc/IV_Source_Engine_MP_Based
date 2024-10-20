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
	DEFINE_KEYFIELD(m_bDirectorControlsSpawners, FIELD_BOOLEAN,	"controlspawners"),
	DEFINE_KEYFIELD(m_iDirectorSpawnTableType, FIELD_INTEGER, "spawntabletype"),
	DEFINE_INPUTFUNC(FIELD_VOID,	"EnableHordes",	InputEnableHordes),
	DEFINE_INPUTFUNC(FIELD_VOID,	"DisableHordes",	InputDisableHordes),
	DEFINE_INPUTFUNC(FIELD_VOID,	"EnableWanderers",	InputEnableWanderers),
	DEFINE_INPUTFUNC(FIELD_VOID,	"DisableWanderers",	InputDisableWanderers),
	DEFINE_INPUTFUNC(FIELD_VOID,	"StartFinale",	InputStartFinale),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetSpawnTableType", InputSetDirectorSpawnTableType),
	DEFINE_OUTPUT(m_OnEscapeRoomStart, "OnEscapeRoomStart"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CIV_Director_Control::Precache()
{
	BaseClass::Precache();
}

void CIV_Director_Control::OnEscapeRoomStart(CBasePlayer *pPlayer)
{
	m_OnEscapeRoomStart.FireOutput(pPlayer, this);
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