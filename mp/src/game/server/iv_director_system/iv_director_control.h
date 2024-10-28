//====== Copyright (c) 1996-2024, Ivan Suvorov and Valve Corporation, All rights reserved. =====
//
// Purpose: IV Director Controller. Based on Alien Swarm Director Controller.
//
//=============================================================================
#ifndef IV_DIRECTOR_CONTROL_H
#define IV_DIRECTOR_CONTROL_H
#ifdef _WIN32
#pragma once
#endif

class CBasePlayer;

// Allows the level designer to send and recieve hints with the director

class CIV_Director_Control : public CLogicalEntity
{
public:

	DECLARE_CLASS(CIV_Director_Control, CLogicalEntity);
	DECLARE_DATADESC();

	virtual void Precache();

	virtual void OnFinaleStarted(CBasePlayer *pPlayer);

	bool m_bWanderersStartEnabled;
	bool m_bHordesStartEnabled;

	int m_iDirectorSpawnTableType;

	int m_iDirectorMinCommonNPCS;
	int m_iDirectorMaxCommonNPCS;
	int m_iDirectorMaxSpecialNPCS;
	bool m_bDirectorSpecialsOnceAdded;

	float m_fDirectorCommonMinRadius;
	float m_fDirectorCommonMaxRadius;

	bool m_bDirectorHordeFrontState;

private:
	void InputEnableHordes(inputdata_t &inputdata);
	void InputDisableHordes(inputdata_t &inputdata);
	void InputEnableWanderers(inputdata_t &inputdata);
	void InputDisableWanderers(inputdata_t &inputdata);
	void InputStartFinale(inputdata_t &inputdata);

	void InputSetDirectorSpawnTableType(inputdata_t &inputdata);

	void InputSetMinCommonNPCS(inputdata_t &inputdata);
	void InputSetMaxCommonNPCS(inputdata_t &inputdata);
	void InputSetMaxSpecialNPCS(inputdata_t &inputdata);
	void InputSetSpecialNPCSOnceState(inputdata_t &inputdata);

	void InputSetMinCommonNPCSSpawnRadius(inputdata_t &inputdata);
	void InputSetMaxCommonNPCSSpawnRadius(inputdata_t &inputdata);

	void InputSetDirectorHordeFrontState(inputdata_t &inputdata);

	COutputEvent m_OnFinaleEventStart;
};

#endif // IV_DIRECTOR_CONTROL_H