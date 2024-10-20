#ifndef _INCLUDED_IV_SPAWN_MANAGER_H
#define _INCLUDED_IV_SPAWN_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"

class CAI_Network;
class CTriggerMultiple;
struct AI_Waypoint_t;
class CAI_Node;
class CAI_BaseNPC;

// The spawn manager can spawn NPC's and groups of NPC's

class IV_Director_NPC_Class_Entry
{
public:
	IV_Director_NPC_Class_Entry(const char *szClass, int nHullType) { m_pszNPCClass = szClass; m_nHullType = nHullType; }

	const char *m_pszNPCClass;
	string_t m_iszNPCClass;
	int m_nHullType;
};

class CIV_Director_Open_Area
{
public:
	CIV_Director_Open_Area()
	{
		m_flArea = 0.0f;
		m_nTotalLinks = 0;
		m_vecOrigin = vec3_origin;
		m_pNode = NULL;
	}
	float m_flArea;
	int m_nTotalLinks;
	Vector m_vecOrigin;
	CAI_Node *m_pNode;
	CUtlVector<CAI_Node*> m_aAreaNodes;
};

enum IV_Spawn_Classes_Types
{
	Zombie,
	Combine,
	Antlions
};

class CIV_Director_Spawn_Manager
{
public:
	CIV_Director_Spawn_Manager();
	~CIV_Director_Spawn_Manager();

	void LevelInitPreEntity();
	void LevelInitPostEntity();
	void Update();
	bool AddHorde(int iHordeSize);			// creates a large pack of NPC's somewhere near the Players
	void AddNPC();							// creates a single NPC somewhere near the Players

	void IV_Set_Spawn_Table(int sended_table_index);

	int SpawnNPCBatch(IV_Director_NPC_Class_Entry *sended_npc_class, int iNumNPCS, const Vector &vecPosition, const QAngle &angle, float flPlayersBeyondDist = 0);	
	CBaseEntity* SpawnNPCAt(IV_Director_NPC_Class_Entry *sended_npc_class, const Vector& vecPos, const QAngle &angle);

	bool ValidSpawnPoint( const Vector &vecPosition, const Vector &vecMins, const Vector &vecMaxs, bool bCheckGround = true, float flPlayerNearDistance = 0 );
	bool LineBlockedByGeometry( const Vector &vecSrc, const Vector &vecEnd );
	
	bool GetNPCBounds(IV_Director_NPC_Class_Entry *sended_npc_class, Vector &vecMins, Vector &vecMaxs);

	int GetHordeToSpawn() { return m_iHordeToSpawn; }

	void OnNPCWokeUp(CAI_BaseNPC *pNPC);
	void OnNPCSleeping(CAI_BaseNPC *pNPC);
	int GetAwakeCommonNPCS() { return m_nAwakeCommonNPCs; }
	int GetAwakeSpecialNPCS() { return m_nAwakeSpecialNPCs; }

	int GetNumNPCClasses();
	IV_Director_NPC_Class_Entry* GetNPCClass(int i);
	int GetNumNPCClassesSpecial();
	IV_Director_NPC_Class_Entry* GetNPCClassSpecial(int i);

	// spawns a headcrab somewhere randomly in the map
	bool SpawnRandomHeadcrab();
	bool SpawnRandomFastHeadcrabs(int nFastHeadcrabs);

private:
	void UpdateCandidateNodes(int sended_hull);
	bool FindHordePosition(int sended_hull);
	CAI_Network* GetNetwork();
	bool SpawnNPCAtRandomNode();
	//void FindEscapeTriggers();
	void DeleteRoute(AI_Waypoint_t *pWaypointList);

	bool IS_NPC_Class_Special(const char *npc_class_name);

	// finds an area with good node connectivity.  Caller should take ownership of the CASW_Open_Area instance.
	CIV_Director_Open_Area* FindNearbyOpenArea(const Vector &vecSearchOrigin, int nSearchHull);

	CountdownTimer m_batchInterval;
	Vector m_vecHordePosition;
	QAngle m_angHordeAngle;
	int m_iHordeToSpawn;
	int m_iNPCsToSpawn;

	int m_nAwakeCommonNPCs;
	int m_nAwakeSpecialNPCs;
	IV_Director_NPC_Class_Entry *m_pDefinedHordeClass;

	// maintaining a list of possible nodes to spawn aliens from
	CUtlVector<int> m_northCandidateNodes;
	CUtlVector<int> m_southCandidateNodes;
	CountdownTimer m_CandidateUpdateTimer;

	typedef CHandle<CTriggerMultiple> TriggerMultiple_t;
	CUtlVector<TriggerMultiple_t> m_EscapeTriggers;
};

CIV_Director_Spawn_Manager* IVDirectorSpawnManager();

class CIV_Director_Path_Utils_NPC;

class CIV_Director_Path_Utils
{
public:
	AI_Waypoint_t *BuildRoute(const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType = NAV_NONE);
	void DeleteRoute(AI_Waypoint_t *pWaypointList);
	AI_Waypoint_t *GetLastRoute() { return m_pLastRoute; }
	void DebugDrawRoute(const Vector &vecStartPos, AI_Waypoint_t *pWaypoint);

private:
	CIV_Director_Path_Utils_NPC* GetPathfinderNPC();

	CHandle<CIV_Director_Path_Utils_NPC> m_hPathfinderNPC;
	AI_Waypoint_t *m_pLastRoute;
};

CIV_Director_Path_Utils* IVDirectorPathUtils();

#endif // _INCLUDED_IV_SPAWN_MANAGER_H