#include "cbase.h"
#include "iv_spawn_manager.h"
#include "ai_network.h"
#include "ai_waypoint.h"
#include "ai_node.h"
#include "ai_basenpc.h"
#include "iv_director.h"
#include "triggers.h"
#include "datacache/imdlcache.h"
#include "ai_link.h"
#include "ai_pathfinder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CIV_Director_Path_Utils_NPC : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CIV_Director_Path_Utils_NPC, CAI_BaseNPC);

	void	Precache(void);
	void	Spawn(void);
	float	MaxYawSpeed(void) { return 90.0f; }
	Class_T Classify(void) { return CLASS_NONE; }
};

LINK_ENTITY_TO_CLASS(iv_pathfinder_npc, CIV_Director_Path_Utils_NPC);

CHandle<CIV_Director_Path_Utils> g_hIVDirectorPathfinder;

void CIV_Director_Path_Utils_NPC::Spawn()
{
	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS);

	SetModel("models/headcrabclassic.mdl");
	NPCInit();

	UTIL_SetSize(this, NAI_Hull::Mins(HULL_TINY), NAI_Hull::Maxs(HULL_TINY));
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_SOLID);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	AddEffects(EF_NODRAW);
	SetMoveType(MOVETYPE_STEP);
	m_takedamage = DAMAGE_NO;
}

void CIV_Director_Path_Utils_NPC::Precache()
{
	PrecacheModel("models/headcrabclassic.mdl");
}


CIV_Director_Path_Utils g_IVDirectorPathfinder;
CIV_Director_Path_Utils* IVDirectorPathUtils() { return &g_IVDirectorPathfinder; }

CIV_Director_Path_Utils_NPC* CIV_Director_Path_Utils::GetPathfinderNPC()
{
	if (m_hPathfinderNPC.Get() == NULL)
	{
		m_hPathfinderNPC = dynamic_cast<CIV_Director_Path_Utils_NPC*>(CBaseEntity::Create("iv_pathfinder_npc", vec3_origin, vec3_angle, NULL));
	}

	return m_hPathfinderNPC.Get();
}

AI_Waypoint_t *CIV_Director_Path_Utils::BuildRoute(const Vector &vStart, const Vector &vEnd,
	CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType)
{
	if (!GetPathfinderNPC())
		return NULL;

	m_pLastRoute = GetPathfinderNPC()->GetPathfinder()->BuildRoute(vStart, vEnd, pTarget, goalTolerance, curNavType, true);

	return m_pLastRoute;
}

Vector g_vecPathStart = vec3_origin;

void CIV_Director_Path_Utils::DeleteRoute(AI_Waypoint_t *pWaypointList)
{
	while (pWaypointList)
	{
		AI_Waypoint_t *pPrevWaypoint = pWaypointList;
		pWaypointList = pWaypointList->GetNext();
		delete pPrevWaypoint;
	}
}

void iv_director_path_start_f()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer || !IVDirectorPathUtils())
		return;

	g_vecPathStart = pPlayer->GetAbsOrigin();
}
ConCommand iv_director_path_start("iv_director_path_start", iv_director_path_start_f, "mark start of pathfinding test", FCVAR_CHEAT);

void CIV_Director_Path_Utils::DebugDrawRoute(const Vector &vecStartPos, AI_Waypoint_t *pWaypoint)
{
	Vector vecLastPos = vecStartPos;
	Msg(" Pathstart = %f %f %f\n", VectorExpand(g_vecPathStart));
	while (pWaypoint)
	{
		Msg("  waypoint = %f %f %f\n", VectorExpand(pWaypoint->GetPos()));
		DebugDrawLine(vecLastPos + Vector(0, 0, 10), pWaypoint->GetPos() + Vector(0, 0, 10), 255, 0, 255, true, 30.0f);
		vecLastPos = pWaypoint->GetPos();
		pWaypoint = pWaypoint->GetNext();
	}
}

void iv_director_path_end_f()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer || !IVDirectorPathUtils())
		return;

	Vector vecPathEnd = pPlayer->GetAbsOrigin();
	debugoverlay->AddBoxOverlay(g_vecPathStart, Vector(-10, -10, -10), Vector(10, 10, 10), vec3_angle, 255, 0, 0, 255, 30.0f);
	debugoverlay->AddBoxOverlay(vecPathEnd, Vector(-10, -10, -10), Vector(10, 10, 10), vec3_angle, 255, 255, 0, 255, 30.0f);

	AI_Waypoint_t *pWaypoint = IVDirectorPathUtils()->BuildRoute(g_vecPathStart, vecPathEnd, NULL, 100, NAV_NONE);
	if (!pWaypoint)
	{
		Msg("Failed to find route\n");
		return;
	}

	IVDirectorPathUtils()->DebugDrawRoute(g_vecPathStart, pWaypoint);
}
ConCommand iv_director_path_end("iv_director_path_end", iv_director_path_end_f, "mark end of pathfinding test", FCVAR_CHEAT);


CIV_Director_Spawn_Manager g_Spawn_Manager;
CIV_Director_Spawn_Manager* IVDirectorSpawnManager() { return &g_Spawn_Manager; }

extern ConVar iv_director_debug;

ConVar iv_spawning_enabled("iv_spawning_enabled", "1", FCVAR_CHEAT, "Director Spawner Enable/Disable State");
ConVar iv_horde_min_distance("iv_horde_min_distance", "800", FCVAR_CHEAT, "Minimum distance away from the marines the horde can spawn");
ConVar iv_horde_max_distance("iv_horde_max_distance", "1500", FCVAR_CHEAT, "Maximum distance away from the marines the horde can spawn");
ConVar iv_max_alien_batch("iv_max_alien_batch", "10", FCVAR_CHEAT, "Max number of aliens spawned in a horde batch");
ConVar iv_batch_interval("iv_batch_interval", "5", FCVAR_CHEAT, "Time between successive batches spawning in the same spot");
ConVar iv_candidate_interval("iv_candidate_interval", "1.0", FCVAR_CHEAT, "Interval between updating candidate spawning nodes");
ConVar iv_director_player_near_distance("iv_director_player_near_distance", "1500", FCVAR_CHEAT, "Near Distance to Allow Horde Spawning");


// ==================================
// == Master list of NPC classes ==
// ==================================

// DEPRICATED NOTE!!!
// NOTE: If you add new entries to this list, update the asw_spawner choices in swarm.fgd.
//       Do not rearrange the order or you will be changing what spawns in all the maps.

IV_Director_NPC_Class_Entry g_NPCs_Classes_Zombies[] =
{
	IV_Director_NPC_Class_Entry("npc_headcrab", HULL_TINY),
	IV_Director_NPC_Class_Entry("npc_headcrab_fast", HULL_TINY),
	IV_Director_NPC_Class_Entry("npc_zombie", HULL_HUMAN),
	IV_Director_NPC_Class_Entry("npc_fastzombie", HULL_HUMAN)
};

IV_Director_NPC_Class_Entry g_NPCs_Classes_Zombies_Bosses[] =
{
	IV_Director_NPC_Class_Entry("npc_poisonzombie", HULL_WIDE_HUMAN)
};

CIV_Director_Spawn_Manager::CIV_Director_Spawn_Manager()
{
	m_nAwakeCommonNPCs = 0;
	m_nAwakeSpecialNPCs = 0;
	m_pDefinedHordeClass = &g_NPCs_Classes_Zombies[0];
}

CIV_Director_Spawn_Manager::~CIV_Director_Spawn_Manager()
{

}

IV_Spawn_Classes_Types g_NPCs_Selected_Type = IV_Spawn_Classes_Types::Zombie;

void CIV_Director_Spawn_Manager::IV_Set_Spawn_Table(int sended_table_index)
{
	g_NPCs_Selected_Type = (IV_Spawn_Classes_Types)sended_table_index;
}

int CIV_Director_Spawn_Manager::GetNumNPCClasses()
{
	if (g_NPCs_Selected_Type == IV_Spawn_Classes_Types::Zombie)
		return NELEMS(g_NPCs_Classes_Zombies);
	else
		return 0;
}

IV_Director_NPC_Class_Entry* CIV_Director_Spawn_Manager::GetNPCClass(int i)
{
	Assert(i >= 0 && i < GetNumNPCClasses());

	if (g_NPCs_Selected_Type == IV_Spawn_Classes_Types::Zombie)
		return &g_NPCs_Classes_Zombies[i];
	else
		return NULL;
}

int CIV_Director_Spawn_Manager::GetNumNPCClassesSpecial()
{
	if (g_NPCs_Selected_Type == IV_Spawn_Classes_Types::Zombie)
		return NELEMS(g_NPCs_Classes_Zombies_Bosses);
	else
		return 0;
}

IV_Director_NPC_Class_Entry* CIV_Director_Spawn_Manager::GetNPCClassSpecial(int i)
{
	Assert(i >= 0 && i < GetNumNPCClassesSpecial());

	if (g_NPCs_Selected_Type == IV_Spawn_Classes_Types::Zombie)
		return &g_NPCs_Classes_Zombies_Bosses[i];
	else
		return NULL;
}

bool CIV_Director_Spawn_Manager::IS_NPC_Class_Special(const char *npc_class_name)
{
	for (int i = 0; i < GetNumNPCClassesSpecial(); i++)
	{
		IV_Director_NPC_Class_Entry *temp_npc_class = GetNPCClassSpecial(i);

		if (!Q_stricmp(npc_class_name, temp_npc_class->m_pszNPCClass))
			return true;
	}

	return false;
}

void CIV_Director_Spawn_Manager::LevelInitPreEntity()
{
	m_nAwakeCommonNPCs = 0;
	m_nAwakeSpecialNPCs = 0;
	// init NPC classes
	for (int i = 0; i < GetNumNPCClasses(); i++)
	{
		GetNPCClass(i)->m_iszNPCClass = AllocPooledString(GetNPCClass(i)->m_pszNPCClass);
	}
}

void CIV_Director_Spawn_Manager::LevelInitPostEntity()
{
	m_vecHordePosition = vec3_origin;
	m_angHordeAngle = vec3_angle;
	m_batchInterval.Invalidate();
	m_CandidateUpdateTimer.Invalidate();
	m_iHordeToSpawn = 0;
	m_iNPCsToSpawn = 0;

	m_northCandidateNodes.Purge();
	m_southCandidateNodes.Purge();

	//FindEscapeTriggers();
}

void CIV_Director_Spawn_Manager::OnNPCWokeUp(CAI_BaseNPC *pNPC)
{
	m_nAwakeCommonNPCs++;
	if (pNPC && IS_NPC_Class_Special(pNPC->GetClassname()))
	{
		m_nAwakeSpecialNPCs++;
	}
}

void CIV_Director_Spawn_Manager::OnNPCSleeping(CAI_BaseNPC *pNPC)
{
	m_nAwakeCommonNPCs--;
	if (pNPC && IS_NPC_Class_Special(pNPC->GetClassname()))
	{
		m_nAwakeSpecialNPCs--;
	}
}

// finds all trigger_multiples linked to asw_objective_escape entities
/*void CIV_Director_Spawn_Manager::FindEscapeTriggers()
{
	m_EscapeTriggers.Purge();

	// go through all escape objectives
	CBaseEntity* pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname( pEntity, "asw_objective_escape" )) != NULL )
	{
		CASW_Objective_Escape* pObjective = dynamic_cast<CASW_Objective_Escape*>(pEntity);
		if ( !pObjective )
			continue;

		const char *pszEscapeTargetName = STRING( pObjective->GetEntityName() );

		CBaseEntity* pOtherEntity = NULL;
		while ( (pOtherEntity = gEntList.FindEntityByClassname( pOtherEntity, "trigger_multiple" )) != NULL )
		{
			CTriggerMultiple *pTrigger = dynamic_cast<CTriggerMultiple*>( pOtherEntity );
			if ( !pTrigger )
				continue;

			bool bAdded = false;
			CBaseEntityOutput *pOutput = pTrigger->FindNamedOutput( "OnTrigger" );
			if ( pOutput )
			{
				CEventAction *pAction = pOutput->GetFirstAction();
				while ( pAction )
				{
					if ( !Q_stricmp( STRING( pAction->m_iTarget ), pszEscapeTargetName ) )
					{
						bAdded = true;
						m_EscapeTriggers.AddToTail( pTrigger );
						break;
					}
					pAction = pAction->m_pNext;
				}
			}

			if ( !bAdded )
			{
				pOutput = pTrigger->FindNamedOutput( "OnStartTouch" );
				if ( pOutput )
				{
					CEventAction *pAction = pOutput->GetFirstAction();
					while ( pAction )
					{
						if ( !Q_stricmp( STRING( pAction->m_iTarget ), pszEscapeTargetName ) )
						{
							bAdded = true;
							m_EscapeTriggers.AddToTail( pTrigger );
							break;
						}
						pAction = pAction->m_pNext;
					}
				}
			}
			
		}
	}
	Msg("IV Director Spawn manager found %d escape triggers\n", m_EscapeTriggers.Count() );
}*/


void CIV_Director_Spawn_Manager::Update()
{
	if ( m_iHordeToSpawn > 0 )
	{
		if ( m_vecHordePosition != vec3_origin && ( !m_batchInterval.HasStarted() || m_batchInterval.IsElapsed() ) )
		{
			int random_spawn_index = RandomInt(0, GetNumNPCClasses());
			m_pDefinedHordeClass = GetNPCClass(random_spawn_index);

			int iToSpawn = MIN(m_iHordeToSpawn, iv_max_alien_batch.GetInt());
			int iSpawned = SpawnNPCBatch(m_pDefinedHordeClass, iToSpawn, m_vecHordePosition, m_angHordeAngle, 0);
			m_iHordeToSpawn -= iSpawned;
			if ( m_iHordeToSpawn <= 0 )
			{
				IVDirector()->OnHordeFinishedSpawning();
				m_vecHordePosition = vec3_origin;
			}
			else if ( iSpawned == 0 )			// if we failed to spawn any NPC's, then try to find a new horde location
			{
				if ( iv_director_debug.GetBool() )
				{
					Msg( "Horde failed to spawn any NPC's, trying new horde position.\n" );
				}
				if (!FindHordePosition(m_pDefinedHordeClass->m_nHullType))		// if we failed to find a new location, just abort this horde
				{
					m_iHordeToSpawn = 0;
					IVDirector()->OnHordeFinishedSpawning();
					m_vecHordePosition = vec3_origin;
				}
			}
			m_batchInterval.Start( iv_batch_interval.GetFloat() );
		}
		else if ( m_vecHordePosition == vec3_origin )
		{
			Msg( "Warning: Had horde to spawn but no position, clearing.\n" );
			m_iHordeToSpawn = 0;
			IVDirector()->OnHordeFinishedSpawning();
		}
	}

	if ( iv_director_debug.GetBool() )
	{
		engine->Con_NPrintf( 14, "SM: Batch interval: %f pos = %f %f %f\n", m_batchInterval.HasStarted() ? m_batchInterval.GetRemainingTime() : -1, VectorExpand( m_vecHordePosition ) );		
	}

	if ( m_iNPCsToSpawn > 0 )
	{
		if ( SpawnNPCAtRandomNode() )
			m_iNPCsToSpawn--;
	}
}

void CIV_Director_Spawn_Manager::AddNPC()
{
	// don't stock up more than 10 wanderers at once
	if ( m_iNPCsToSpawn > 10 )
		return;

	m_iNPCsToSpawn++;
}

bool CIV_Director_Spawn_Manager::SpawnNPCAtRandomNode()
{
	int random_spawn_index = RandomInt(0, GetNumNPCClasses());
	IV_Director_NPC_Class_Entry* temp_selected_npc_class = GetNPCClass(random_spawn_index);

	UpdateCandidateNodes(temp_selected_npc_class->m_nHullType);

	// decide if the NPC is going to come from behind or in front
	bool bNorth = RandomFloat() < 0.7f;
	if ( m_northCandidateNodes.Count() <= 0 )
	{
		bNorth = false;
	}
	else if ( m_southCandidateNodes.Count() <= 0 )
	{
		bNorth = true;
	}

	CUtlVector<int> &candidateNodes = bNorth ? m_northCandidateNodes : m_southCandidateNodes;

	if ( candidateNodes.Count() <= 0 )
		return false;

	Vector vecMins, vecMaxs;
	GetNPCBounds(temp_selected_npc_class, vecMins, vecMaxs);

	int iMaxTries = 1;
	for (int i=0; i<iMaxTries; i++)
	{
		int iChosen = RandomInt( 0, candidateNodes.Count() - 1);
		CAI_Node *pNode = GetNetwork()->GetNode(candidateNodes[iChosen]);
		if (!pNode)
			continue;

		float fldistance = 0;
		CBasePlayer *pPlayer = UTIL_GetNearestPlayerSimple(pNode->GetPosition(temp_selected_npc_class->m_nHullType), &fldistance);
		if (!pPlayer)
			return false;

		// check if there's a route from this node to the Player(s)
		AI_Waypoint_t *pRoute = IVDirectorPathUtils()->BuildRoute(pNode->GetPosition(temp_selected_npc_class->m_nHullType), pPlayer->GetAbsOrigin(), NULL, 100);
		if (!pRoute)
		{
			if (iv_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D( pNode->GetOrigin(), 10.0f, 255, 128, 0, true, 20.0f );
			}
			continue;
		}
		
		Vector vecSpawnPos = pNode->GetPosition(temp_selected_npc_class->m_nHullType) + Vector(0, 0, 32);
		if (ValidSpawnPoint(vecSpawnPos, vecMins, vecMaxs, true, iv_director_player_near_distance.GetFloat()))
		{
			if (SpawnNPCAt(temp_selected_npc_class, vecSpawnPos, vec3_angle))
			{
				if (iv_director_debug.GetBool())
				{
					NDebugOverlay::Cross3D(vecSpawnPos, 25.0f, 255, 255, 255, true, 20.0f);
					float fldistance = 0;
					CBasePlayer *pPlayer = UTIL_GetNearestPlayerSimple(vecSpawnPos, &fldistance);
					if (pPlayer)
					{
						NDebugOverlay::Line(pPlayer->GetAbsOrigin(), vecSpawnPos, 64, 64, 64, true, 60.0f);
					}
				}
				DeleteRoute(pRoute);
				return true;
			}
		}
		else
		{
			if (iv_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D(vecSpawnPos, 25.0f, 255, 0, 0, true, 20.0f);
			}
		}
		DeleteRoute(pRoute);
	}

	return false;
}

bool CIV_Director_Spawn_Manager::AddHorde(int iHordeSize)
{
	m_iHordeToSpawn = iHordeSize;

	if (m_vecHordePosition == vec3_origin)
	{
		if (!FindHordePosition(m_pDefinedHordeClass->m_nHullType))
		{
			Msg("Error: Failed to find horde position\n");
			return false;
		}
		else
		{
			if (iv_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D(m_vecHordePosition, 50.0f, 255, 128, 0, true, 60.0f);
				float fldistance = 0;
				CBasePlayer *pPlayer = UTIL_GetNearestPlayerSimple(m_vecHordePosition, &fldistance);
				if (pPlayer)
				{
					NDebugOverlay::Line(pPlayer->GetAbsOrigin(), m_vecHordePosition, 255, 128, 0, true, 60.0f);
				}
			}
		}
	}
	return true;
}

CAI_Network* CIV_Director_Spawn_Manager::GetNetwork()
{
	return g_pBigAINet;
}

void CIV_Director_Spawn_Manager::UpdateCandidateNodes(int sended_hull)
{
	// don't update too frequently
	if (m_CandidateUpdateTimer.HasStarted() && !m_CandidateUpdateTimer.IsElapsed())
		return;

	m_CandidateUpdateTimer.Start(iv_candidate_interval.GetFloat());

	if (!GetNetwork() || !GetNetwork()->NumNodes())
	{
		m_vecHordePosition = vec3_origin;
		Msg("Error: Can't spawn hordes as this map has no node network\n");
		return;
	}

	int total_players_count = gpGlobals->maxClients;

	if (total_players_count <= 0)
	{
		Msg("Error: Connected Players List is Invalid or Invalid Players Count!!!\n");
		return;
	}

	Vector vecSouthPlayer = vec3_origin;
	Vector vecNorthPlayer = vec3_origin;
	for (int i = 1; i <= total_players_count; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer || pPlayer->GetHealth() <= 0)
			continue;

		if (vecSouthPlayer == vec3_origin || vecSouthPlayer.y > pPlayer->GetAbsOrigin().y)
		{
			vecSouthPlayer = pPlayer->GetAbsOrigin();
		}
		if (vecNorthPlayer == vec3_origin || vecNorthPlayer.y < pPlayer->GetAbsOrigin().y)
		{
			vecNorthPlayer = pPlayer->GetAbsOrigin();
		}
	}

	if (vecSouthPlayer == vec3_origin || vecNorthPlayer == vec3_origin)		// no live Players
		return;
	
	int iNumNodes = GetNetwork()->NumNodes();
	m_northCandidateNodes.Purge();
	m_southCandidateNodes.Purge();

	for (int i = 0; i < iNumNodes; i++)
	{
		CAI_Node *pNode = GetNetwork()->GetNode(i);
		if (!pNode || pNode->GetType() != NODE_GROUND)
			continue;

		Vector vecPos = pNode->GetPosition(sended_hull);
		
		// find the nearest Player to this node
		float flDistance = 0;
		CBasePlayer *pPlayer = UTIL_GetNearestPlayerSimple(vecPos, &flDistance);
		if (!pPlayer)
			return;

		if (flDistance > iv_horde_max_distance.GetFloat() || flDistance < iv_horde_min_distance.GetFloat())
			continue;

		// check node isn't in an exit trigger
		bool bInsideEscapeArea = false;
		for (int d = 0; d < m_EscapeTriggers.Count(); d++)
		{
			if (m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds(vecPos))
			{
				bInsideEscapeArea = true;
				break;
			}
		}

		if (bInsideEscapeArea)
			continue;

		if (vecPos.y >= vecSouthPlayer.y)
		{
			if (iv_director_debug.GetInt() == 3)
			{
				NDebugOverlay::Box(vecPos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 32, 32, 128, 10, 60.0f);
			}
			m_northCandidateNodes.AddToTail(i);
		}
		if (vecPos.y <= vecNorthPlayer.y)
		{
			m_southCandidateNodes.AddToTail(i);
			if (iv_director_debug.GetInt() == 3)
			{
				NDebugOverlay::Box(vecPos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 128, 32, 32, 10, 60.0f);
			}
		}
	}
}

bool CIV_Director_Spawn_Manager::FindHordePosition(int sended_hull)
{
	// need to find a suitable place from which to spawn a horde
	// this place should:
	//   - be far enough away from the marines so the whole horde can spawn before the Players get there
	//   - should have a clear path to the Players
	
	UpdateCandidateNodes(sended_hull);

	// decide if the horde is going to come from behind or in front
	bool bNorth = RandomFloat() < 0.7f;
	if (m_northCandidateNodes.Count() <= 0)
	{
		bNorth = false;
	}
	else if (m_southCandidateNodes.Count() <= 0)
	{
		bNorth = true;
	}

	CUtlVector<int> &candidateNodes = bNorth ? m_northCandidateNodes : m_southCandidateNodes;

	if (candidateNodes.Count() <= 0)
	{
		if (iv_director_debug.GetBool())
		{
			Msg("Failed to find horde pos as there are no candidate nodes\n");
		}
		return false;
	}

	int iMaxTries = 3;
	for (int i = 0; i < iMaxTries; i++)
	{
		int iChosen = RandomInt(0, candidateNodes.Count() - 1);
		CAI_Node *pNode = GetNetwork()->GetNode(candidateNodes[iChosen]);
		if (!pNode)
			continue;

		float flDistance = 0;
		CBasePlayer *pPlayer = UTIL_GetNearestPlayerSimple(pNode->GetPosition(sended_hull), &flDistance);
		if (!pPlayer)
		{
			if (iv_director_debug.GetBool())
			{
				Msg("Failed to find horde pos as there is no nearest Player\n");
			}
			return false;
		}

		// check if there's a route from this node to the Player(s)
		AI_Waypoint_t *pRoute = IVDirectorPathUtils()->BuildRoute(pNode->GetPosition(sended_hull), pPlayer->GetAbsOrigin(), NULL, 100);
		if (!pRoute)
		{
			if (iv_director_debug.GetInt() >= 2)
			{
				Msg("Discarding horde node %d as there's no route.\n", iChosen);
			}
			continue;
		}
		
		m_vecHordePosition = pNode->GetPosition(sended_hull) + Vector(0, 0, 32);

		// spawn facing the nearest Player
		Vector vecDir = pPlayer->GetAbsOrigin() - m_vecHordePosition;
		vecDir.z = 0;
		vecDir.NormalizeInPlace();
		VectorAngles( vecDir, m_angHordeAngle );

		if (iv_director_debug.GetInt() >= 2)
		{
			Msg("Accepting horde node %d.\n", iChosen);
		}
		DeleteRoute(pRoute);
		return true;
	}

	if (iv_director_debug.GetBool())
	{
		Msg("Failed to find horde pos as we tried 3 times to build routes to possible locations, but failed\n");
	}

	return false;
}

bool CIV_Director_Spawn_Manager::LineBlockedByGeometry(const Vector &vecSrc, const Vector &vecEnd)
{
	trace_t tr;
	UTIL_TraceLine(vecSrc,
		vecEnd, MASK_SOLID_BRUSHONLY, 
		NULL, COLLISION_GROUP_NONE, &tr);

	return (tr.fraction != 1.0f);
}

bool CIV_Director_Spawn_Manager::GetNPCBounds(IV_Director_NPC_Class_Entry *sended_npc_class, Vector &vecMins, Vector &vecMaxs)
{
	vecMins = NAI_Hull::Mins(sended_npc_class->m_nHullType);
	vecMaxs = NAI_Hull::Maxs(sended_npc_class->m_nHullType);
	return true;
}

// spawn a group of NPC's at the target point
int CIV_Director_Spawn_Manager::SpawnNPCBatch(IV_Director_NPC_Class_Entry *sended_npc_class, int iNumNPCs, const Vector &vecPosition, const QAngle &angFacing, float flPlayersBeyondDist)
{
	int iSpawned = 0;
	bool bCheckGround = true;
	Vector vecMins = NAI_Hull::Mins(sended_npc_class->m_nHullType);
	Vector vecMaxs = NAI_Hull::Maxs(sended_npc_class->m_nHullType);

	float flNPCWidth = vecMaxs.x - vecMins.x;
	float flNPCDepth = vecMaxs.y - vecMins.y;

	// spawn one in the middle
	if (ValidSpawnPoint(vecPosition, vecMins, vecMaxs, bCheckGround, flPlayersBeyondDist))
	{
		if (SpawnNPCAt(sended_npc_class, vecPosition, angFacing))
			iSpawned++;
	}

	// try to spawn a 5x5 grid of NPC's, starting at the centre and expanding outwards
	Vector vecNewPos = vecPosition;
	for (int i = 1; i <= 5 && iSpawned < iNumNPCs; i++)
	{
		QAngle angle = angFacing;
		angle[YAW] += RandomFloat(-20, 20);
		// spawn NPC's along top of box
		for (int x = -i; x <= i && iSpawned < iNumNPCs; x++)
		{
			vecNewPos = vecPosition;
			vecNewPos.x += x * flNPCWidth;
			vecNewPos.y -= i * flNPCDepth;
			if (!LineBlockedByGeometry(vecPosition, vecNewPos) && ValidSpawnPoint(vecNewPos, vecMins, vecMaxs, bCheckGround, flPlayersBeyondDist))
			{
				if (SpawnNPCAt(sended_npc_class, vecNewPos, angle))
					iSpawned++;
			}
		}

		// spawn NPC's along bottom of box
		for (int x = -i; x <= i && iSpawned < iNumNPCs; x++)
		{
			vecNewPos = vecPosition;
			vecNewPos.x += x * flNPCWidth;
			vecNewPos.y += i * flNPCDepth;
			if (!LineBlockedByGeometry(vecPosition, vecNewPos) && ValidSpawnPoint(vecNewPos, vecMins, vecMaxs, bCheckGround, flPlayersBeyondDist))
			{
				if (SpawnNPCAt(sended_npc_class, vecNewPos, angle))
					iSpawned++;
			}
		}

		// spawn NPC's along left of box
		for (int y = -i + 1; y<i && iSpawned < iNumNPCs; y++)
		{
			vecNewPos = vecPosition;
			vecNewPos.x -= i * flNPCWidth;
			vecNewPos.y += y * flNPCDepth;
			if (!LineBlockedByGeometry(vecPosition, vecNewPos) && ValidSpawnPoint(vecNewPos, vecMins, vecMaxs, bCheckGround, flPlayersBeyondDist))
			{
				if (SpawnNPCAt(sended_npc_class, vecNewPos, angle))
					iSpawned++;
			}
		}

		// spawn NPC's along right of box
		for (int y = -i + 1; y<i && iSpawned < iNumNPCs; y++)
		{
			vecNewPos = vecPosition;
			vecNewPos.x += i * flNPCWidth;
			vecNewPos.y += y * flNPCDepth;
			if (!LineBlockedByGeometry(vecPosition, vecNewPos) && ValidSpawnPoint(vecNewPos, vecMins, vecMaxs, bCheckGround, flPlayersBeyondDist))
			{
				if (SpawnNPCAt(sended_npc_class, vecNewPos, angle))
					iSpawned++;
			}
		}
	}

	return iSpawned;
}

CBaseEntity* CIV_Director_Spawn_Manager::SpawnNPCAt(IV_Director_NPC_Class_Entry *sended_npc_class, const Vector& vecPos, const QAngle &angle)
{	
	CBaseEntity	*pEntity = NULL;	
	pEntity = CreateEntityByName(sended_npc_class->m_pszNPCClass);
	CAI_BaseNPC	*pNPC = dynamic_cast<CAI_BaseNPC*>(pEntity);

	if (pNPC)
	{
		pNPC->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	}

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = angle;
	angles.x = 0.0;
	angles.z = 0.0;	
	pEntity->SetAbsOrigin(vecPos);	
	pEntity->SetAbsAngles(angles);
	UTIL_DropToFloor(pEntity, MASK_SOLID);

	// have headcrabs unburrow by default, so we don't worry so much about them spawning onscreen
	/*if (!Q_strcmp(sended_npc_class->m_pszNPCClass, "npc_headcrab"))
	{
		pNPC->StartBurrowed();
		pNPC->SetUnburrowIdleActivity(NULL_STRING);
		pNPC->SetUnburrowActivity(NULL_STRING);
	}*/

	DispatchSpawn(pEntity);	
	pEntity->Activate();	

	// give our NPC's the orders
	float flNearPlayerDist = 0;
	CBasePlayer* pPlayer = UTIL_GetNearestPlayerSimple(pNPC->GetAbsOrigin(), &flNearPlayerDist);
	pNPC->UpdateEnemyMemory(pPlayer, pPlayer->GetAbsOrigin());

	return pEntity;
}

bool CIV_Director_Spawn_Manager::ValidSpawnPoint(const Vector &vecPosition, const Vector &vecMins, const Vector &vecMaxs, bool bCheckGround, float flPlayerNearDistance)
{
	// check if we can fit there
	trace_t tr;
	UTIL_TraceHull( vecPosition,
		vecPosition + Vector( 0, 0, 1 ),
		vecMins,
		vecMaxs,
		MASK_NPCSOLID,
		NULL,
		COLLISION_GROUP_NONE,
		&tr );

	if( tr.fraction != 1.0 )
		return false;

	// check there's ground underneath this point
	if ( bCheckGround )
	{
		UTIL_TraceHull( vecPosition + Vector( 0, 0, 1 ),
			vecPosition - Vector( 0, 0, 64 ),
			vecMins,
			vecMaxs,
			MASK_NPCSOLID,
			NULL,
			COLLISION_GROUP_NONE,
			&tr );

		if( tr.fraction == 1.0 )
			return false;
	}

	if (flPlayerNearDistance > 0)
	{
		int total_players_count = gpGlobals->maxClients;

		if (total_players_count <= 0)
			return false;

		float distance = 0.0f;
		for (int i = 1; i <= total_players_count; i++)
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

			if (!pPlayer)
				continue;

			if (pPlayer->GetHealth() > 0)
			{
				distance = pPlayer->GetAbsOrigin().DistTo(vecPosition);
				if (distance < flPlayerNearDistance)
				{
					return false;
				}
			}
		}
	}

	return true;
}

void CIV_Director_Spawn_Manager::DeleteRoute( AI_Waypoint_t *pWaypointList )
{
	while ( pWaypointList )
	{
		AI_Waypoint_t *pPrevWaypoint = pWaypointList;
		pWaypointList = pWaypointList->GetNext();
		delete pPrevWaypoint;
	}
}

bool CIV_Director_Spawn_Manager::SpawnRandomHeadcrab()
{
	int iNumNodes = g_pBigAINet->NumNodes();
	if (iNumNodes < 6)
		return false;

	int nHull = HULL_TINY;
	CUtlVector<CIV_Director_Open_Area*> aAreas;
	for (int i = 0; i < 6; i++)
	{
		CAI_Node *pNode = NULL;
		int nTries = 0;
		while (nTries < 5 && (!pNode || pNode->GetType() != NODE_GROUND))
		{
			pNode = g_pBigAINet->GetNode(RandomInt( 0, iNumNodes));
			nTries++;
		}
		
		if (pNode)
		{
			CIV_Director_Open_Area *pArea = FindNearbyOpenArea(pNode->GetOrigin(), HULL_MEDIUM);
			if (pArea && pArea->m_nTotalLinks > 30)
			{
				// test if there's room to spawn a headcrab at that spot
				if ( ValidSpawnPoint(pArea->m_pNode->GetPosition(nHull), NAI_Hull::Mins(nHull), NAI_Hull::Maxs(nHull), true, false))
				{
					aAreas.AddToTail(pArea);
				}
				else
				{
					delete pArea;
				}
			}
		}
		// stop searching once we have 3 acceptable candidates
		if (aAreas.Count() >= 3)
			break;
	}

	// find area with the highest connectivity
	CIV_Director_Open_Area *pBestArea = NULL;
	for ( int i = 0; i < aAreas.Count(); i++ )
	{
		CIV_Director_Open_Area *pArea = aAreas[i];
		if ( !pBestArea || pArea->m_nTotalLinks > pBestArea->m_nTotalLinks )
		{
			pBestArea = pArea;
		}
	}

	if (pBestArea)
	{
		CBaseEntity *pNPC = SpawnNPCAt(&g_NPCs_Classes_Zombies[0], pBestArea->m_pNode->GetPosition(nHull), RandomAngle(0, 360));
		CAI_BaseNPC *pSpawnableNPC = dynamic_cast<CAI_BaseNPC*>(pNPC);
		if (pSpawnableNPC)
		{
			float near_distance = 0;
			CBasePlayer *temp_nearest_player = UTIL_GetNearestPlayerSimple(pNPC->GetAbsOrigin(), &near_distance);
			pSpawnableNPC->UpdateEnemyMemory(temp_nearest_player, temp_nearest_player->GetAbsOrigin());
		}
		aAreas.PurgeAndDeleteElements();
		return true;
	}

	aAreas.PurgeAndDeleteElements();
	return false;
}

Vector TraceToGround( const Vector &vecPos )
{
	trace_t tr;
	UTIL_TraceLine(vecPos + Vector( 0, 0, 100 ), vecPos, MASK_NPCSOLID, NULL, COLLISION_GROUP_NPC, &tr);
	return tr.endpos;
}

bool CIV_Director_Spawn_Manager::SpawnRandomFastHeadcrabs(int nFastHeadcrabs)
{
	int iNumNodes = g_pBigAINet->NumNodes();
	if (iNumNodes < 6)
		return false;

	int nHull = HULL_TINY;
	CUtlVector<CIV_Director_Open_Area*> aAreas;
	for (int i = 0; i < 6; i++)
	{
		CAI_Node *pNode = NULL;
		int nTries = 0;
		while (nTries < 5 && (!pNode || pNode->GetType() != NODE_GROUND))
		{
			pNode = g_pBigAINet->GetNode(RandomInt( 0, iNumNodes));
			nTries++;
		}

		if (pNode)
		{
			CIV_Director_Open_Area *pArea = FindNearbyOpenArea(pNode->GetOrigin(), HULL_MEDIUM);
			if (pArea && pArea->m_nTotalLinks > 30)
			{
				// test if there's room to spawn a fast headcrab at that spot
				if (ValidSpawnPoint( pArea->m_pNode->GetPosition(nHull), NAI_Hull::Mins(nHull), NAI_Hull::Maxs(nHull), true, false))
				{
					aAreas.AddToTail(pArea);
				}
				else
				{
					delete pArea;
				}
			}
		}
		// stop searching once we have 3 acceptable candidates
		if (aAreas.Count() >= 3)
			break;
	}

	// find area with the highest connectivity
	CIV_Director_Open_Area *pBestArea = NULL;
	for (int i = 0; i < aAreas.Count(); i++)
	{
		CIV_Director_Open_Area *pArea = aAreas[i];
		if (!pBestArea || pArea->m_nTotalLinks > pBestArea->m_nTotalLinks)
		{
			pBestArea = pArea;
		}
	}

	if (pBestArea)
	{
		for (int i = 0; i < nFastHeadcrabs; i++)
		{
			CBaseEntity *pNPC = SpawnNPCAt(&g_NPCs_Classes_Zombies[1], TraceToGround(pBestArea->m_pNode->GetPosition(nHull)), RandomAngle(0, 360));
			CAI_BaseNPC *pNPCSpawnable = dynamic_cast<CAI_BaseNPC*>(pNPC);
			if (pNPCSpawnable)
			{
				float near_distance = 0;
				CBasePlayer *temp_nearest_player = UTIL_GetNearestPlayerSimple(pNPC->GetAbsOrigin(), &near_distance);
				pNPCSpawnable->UpdateEnemyMemory(temp_nearest_player, temp_nearest_player->GetAbsOrigin());
			}
			if (iv_director_debug.GetBool() && pNPC)
			{
				Msg("Spawned Fast Headcrab at %f %f %f\n", pNPC->GetAbsOrigin());
				NDebugOverlay::Cross3D(pNPC->GetAbsOrigin(), 8.0f, 255, 0, 0, true, 20.0f);
			}
		}
		aAreas.PurgeAndDeleteElements();
		return true;
	}

	aAreas.PurgeAndDeleteElements();
	return false;
}

// heuristic to find reasonably open space - searches for areas with high node connectivity
CIV_Director_Open_Area* CIV_Director_Spawn_Manager::FindNearbyOpenArea(const Vector &vecSearchOrigin, int nSearchHull)
{
	CBaseEntity *pStartEntity = gEntList.FindEntityByClassname(NULL, "info_player_start");
	int iNumNodes = g_pBigAINet->NumNodes();
	CAI_Node *pHighestConnectivity = NULL;
	int nHighestLinks = 0;
	for (int i=0 ; i<iNumNodes; i++)
	{
		CAI_Node *pNode = g_pBigAINet->GetNode(i);
		if (!pNode || pNode->GetType() != NODE_GROUND)
			continue;

		Vector vecPos = pNode->GetOrigin();
		float flDist = vecPos.DistTo(vecSearchOrigin);
		if (flDist > 400.0f)
			continue;

		// discard if node is too near start location
		if (pStartEntity && vecPos.DistTo( pStartEntity->GetAbsOrigin() ) < 1400.0f)  // NOTE: assumes all start points are clustered near one another
			continue;

		// discard if node is inside an escape area
		bool bInsideEscapeArea = false;
		for (int d=0; d<m_EscapeTriggers.Count(); d++)
		{
			if (m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds(vecPos))
			{
				bInsideEscapeArea = true;
				break;
			}
		}
		if (bInsideEscapeArea)
			continue;

		// count links that drones could follow
		int nLinks = pNode->NumLinks();
		int nValidLinks = 0;
		for (int k = 0; k < nLinks; k++)
		{
			CAI_Link *pLink = pNode->GetLinkByIndex(k);
			if (!pLink)
				continue;

			if (!(pLink->m_iAcceptedMoveTypes[nSearchHull] & bits_CAP_MOVE_GROUND))
				continue;

			nValidLinks++;
		}
		if ( nValidLinks > nHighestLinks )
		{
			nHighestLinks = nValidLinks;
			pHighestConnectivity = pNode;
		}
		if (iv_director_debug.GetBool())
		{
			NDebugOverlay::Text( vecPos, UTIL_VarArgs( "%d", nValidLinks ), false, 10.0f );
		}
	}

	if (!pHighestConnectivity)
		return NULL;

	// now, starting at the new node, find all nearby nodes with a minimum connectivity
	CIV_Director_Open_Area *pArea = new CIV_Director_Open_Area();
	pArea->m_vecOrigin = pHighestConnectivity->GetOrigin();
	pArea->m_pNode = pHighestConnectivity;
	int nMinLinks = nHighestLinks * 0.3f;
	nMinLinks = MAX( nMinLinks, 4 );

	pArea->m_aAreaNodes.AddToTail( pHighestConnectivity );
	if (iv_director_debug.GetBool())
	{
		Msg( "minLinks = %d\n", nMinLinks );
	}
	pArea->m_nTotalLinks = 0;
	for ( int i=0 ; i<iNumNodes; i++ )
	{
		CAI_Node *pNode = g_pBigAINet->GetNode( i );
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		Vector vecPos = pNode->GetOrigin();
		float flDist = vecPos.DistTo( pArea->m_vecOrigin );
		if ( flDist > 400.0f )
			continue;

		// discard if node is inside an escape area
		bool bInsideEscapeArea = false;
		for ( int d=0; d<m_EscapeTriggers.Count(); d++ )
		{
			if ( m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds( vecPos ) )
			{
				bInsideEscapeArea = true;
				break;
			}
		}
		if ( bInsideEscapeArea )
			continue;

		// count links that drones could follow
		int nLinks = pNode->NumLinks();
		int nValidLinks = 0;
		for ( int k = 0; k < nLinks; k++ )
		{
			CAI_Link *pLink = pNode->GetLinkByIndex( k );
			if ( !pLink )
				continue;

			if ( !( pLink->m_iAcceptedMoveTypes[nSearchHull] & bits_CAP_MOVE_GROUND ) )
				continue;

			nValidLinks++;
		}
		if ( nValidLinks >= nMinLinks )
		{
			pArea->m_aAreaNodes.AddToTail( pNode );
			pArea->m_nTotalLinks += nValidLinks;
		}
	}
	// highlight and measure bounds
	Vector vecAreaMins = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector vecAreaMaxs = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	
	for ( int i = 0; i < pArea->m_aAreaNodes.Count(); i++ )
	{
		VectorMin(vecAreaMins, pArea->m_aAreaNodes[i]->GetOrigin(), vecAreaMins);
		VectorMax(vecAreaMaxs, pArea->m_aAreaNodes[i]->GetOrigin(), vecAreaMaxs);
		
		if (iv_director_debug.GetBool())
		{
			if ( i == 0 )
			{
				NDebugOverlay::Cross3D( pArea->m_aAreaNodes[i]->GetOrigin(), 20.0f, 255, 255, 64, true, 10.0f );
			}
			else
			{
				NDebugOverlay::Cross3D( pArea->m_aAreaNodes[i]->GetOrigin(), 10.0f, 255, 128, 0, true, 10.0f );
			}
		}
	}

	Vector vecArea = ( vecAreaMaxs - vecAreaMins );
	float flArea = vecArea.x * vecArea.y;

	if (iv_director_debug.GetBool())
	{
		Msg("area mins = %f %f %f\n", VectorExpand(vecAreaMins));
		Msg("area maxs = %f %f %f\n", VectorExpand(vecAreaMaxs));
		NDebugOverlay::Box(vec3_origin, vecAreaMins, vecAreaMaxs, 255, 128, 128, 10, 10.0f);	
		Msg("Total links = %d Area = %f\n", pArea->m_nTotalLinks, flArea);
	}

	return pArea;
}

// creates a batch of NPC's at the mouse cursor
void iv_director_fast_headcrabs_batch_f(const CCommand& args)
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// find spawn point
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	trace_t tr;
	Vector forward;

	AngleVectors(pPlayer->EyeAngles(), &forward);
	UTIL_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * 300.0f, MASK_SOLID,
		pPlayer, COLLISION_GROUP_NONE, &tr);
	if ( tr.fraction != 0.0 )
	{
		// trace to the floor from this spot
		Vector vecSrc = tr.endpos;
		tr.endpos.z += 12;
		UTIL_TraceLine( vecSrc + Vector(0, 0, 12),
			vecSrc - Vector( 0, 0, 512 ) ,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr);
		
		IVDirectorSpawnManager()->SpawnNPCBatch(&g_NPCs_Classes_Zombies[1], 25, tr.endpos, vec3_angle);
	}
	
	CBaseEntity::SetAllowPrecache(allowPrecache);
}
static ConCommand iv_director_fast_headcrabs_batch("iv_director_fast_headcrabs_batch", iv_director_fast_headcrabs_batch_f, "Creates a batch of Fast Headcrabs at the cursor", FCVAR_GAMEDLL | FCVAR_CHEAT);


void iv_director_horde_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		Msg("supply horde size!\n");
		return;
	}
	if (!IVDirectorSpawnManager()->AddHorde(atoi(args[1])))
	{
		Msg("Failed to add horde\n");
	}
}
static ConCommand iv_director_horde("iv_director_horde", iv_director_horde_f, "Creates a horde of NPC's somewhere nearby", FCVAR_GAMEDLL | FCVAR_CHEAT);


CON_COMMAND_F(iv_director_spawn_headcrab, "Spawns a headcrab somewhere randomly in the map", FCVAR_CHEAT)
{
	IVDirectorSpawnManager()->SpawnRandomHeadcrab();
}

CON_COMMAND_F(iv_director_spawn_fast_headcrabs, "Spawns a group of Fast Headcrabs somewhere randomly in the map", FCVAR_CHEAT)
{
	IVDirectorSpawnManager()->SpawnRandomFastHeadcrabs(RandomInt( 3, 5 ));
}