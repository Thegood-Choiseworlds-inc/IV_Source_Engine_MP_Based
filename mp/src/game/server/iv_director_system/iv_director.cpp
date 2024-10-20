#include "cbase.h"
#include "iv_director.h"
#include "iv_spawn_manager.h"
#include "ai_network.h"
#include "iv_director_control.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar iv_director_debug("iv_director_debug", "0", FCVAR_CHEAT, "Displays director status on screen");

extern ConVar iv_intensity_far_range;
extern ConVar iv_spawning_enabled;

ConVar iv_horde_override("iv_horde_override", "0", FCVAR_CHEAT, "Force Override Horde State");
ConVar iv_wanderer_override("iv_wanderer_override", "0", FCVAR_CHEAT, "Force Override Wanderer Spawn State");
ConVar iv_horde_interval_min("iv_horde_interval_min", "45", FCVAR_CHEAT, "Min time between hordes");
ConVar iv_horde_interval_max("iv_horde_interval_max", "65", FCVAR_CHEAT, "Min time between hordes");
ConVar iv_horde_size_min("iv_horde_size_min", "9", FCVAR_CHEAT, "Min horde size");
ConVar iv_horde_size_max("iv_horde_size_max", "14", FCVAR_CHEAT, "Max horde size");

ConVar iv_director_relaxed_min_time("iv_director_relaxed_min_time", "25", FCVAR_CHEAT, "Min time that director stops spawning NPC's");
ConVar iv_director_relaxed_max_time("iv_director_relaxed_max_time", "40", FCVAR_CHEAT, "Max time that director stops spawning NPC's");
ConVar iv_director_peak_min_time("iv_director_peak_min_time", "1", FCVAR_CHEAT, "Min time that director keeps spawning NPC's when player intensity has peaked");
ConVar iv_director_peak_max_time("iv_director_peak_max_time", "3", FCVAR_CHEAT, "Max time that director keeps spawning NPC's when player intensity has peaked");
ConVar iv_interval_min("iv_interval_min", "1.0f", FCVAR_CHEAT, "Director: NPC spawn interval will never go lower than this");
ConVar iv_interval_initial_min("iv_interval_initial_min", "5", FCVAR_CHEAT, "Director: Min time between NPC spawns when first entering spawning state");
ConVar iv_interval_initial_max("iv_interval_initial_max", "7", FCVAR_CHEAT, "Director: Max time between NPC spawns when first entering spawning state");
ConVar iv_interval_change_min("iv_interval_change_min", "0.9", FCVAR_CHEAT, "Director: Min scale applied to NPC spawn interval each spawn");
ConVar iv_interval_change_max("iv_interval_change_max", "0.95", FCVAR_CHEAT, "Director: Max scale applied to NPC spawn interval each spawn");

CIV_Director g_IVDirector;
CIV_Director* IVDirector() { return &g_IVDirector; }

CIV_Director::CIV_Director(void) : CAutoGameSystemPerFrame("CIV_Director")
{
	
}

CIV_Director::~CIV_Director()
{
	
}

bool CIV_Director::Init()
{
	m_bSpawningNPCs = false;
	m_bReachedIntensityPeak = false;
	m_fTimeBetweenNPCs = 0;
	m_NPCSpawnTimer.Invalidate();
	m_SustainTimer.Invalidate();
	m_HordeTimer.Invalidate();
	m_IntensityUpdateTimer.Invalidate();
	m_bInitialWait = true;

	m_bFiredEscapeRoom = false;
	
	m_bHordeInProgress = false;
	m_bFinale = false;

	// take horde/wanderer settings from director control
	CIV_Director_Control* pControl = static_cast<CIV_Director_Control*>(gEntList.FindEntityByClassname(NULL, "iv_director_control"));
	if ( pControl )
	{
		m_bWanderersEnabled = pControl->m_bWanderersStartEnabled;
		m_bHordesEnabled = pControl->m_bHordesStartEnabled;
		m_bDirectorControlsSpawners = pControl->m_bDirectorControlsSpawners;
	}
	else
	{
		m_bWanderersEnabled = false;
		m_bHordesEnabled = false;
		m_bDirectorControlsSpawners = false;
	}

	return true;
}

void CIV_Director::Shutdown()
{

}

void CIV_Director::LevelInitPreEntity()
{
	if (IVDirectorSpawnManager())
	{
		IVDirectorSpawnManager()->LevelInitPreEntity();
	}
}

void CIV_Director::LevelInitPostEntity()
{
	Init();

	if (IVDirectorSpawnManager())
	{
		IVDirectorSpawnManager()->LevelInitPostEntity();
	}
}

void CIV_Director::FrameUpdatePreEntityThink()
{

}

void CIV_Director::FrameUpdatePostEntityThink()
{
	// only think when we're in-game
	/*if ( !ASWGameRules() || ASWGameRules()->GetGameState() != ASW_GS_INGAME )
		return;*/

	UpdateIntensity();

	if (IVDirectorSpawnManager())
	{
		IVDirectorSpawnManager()->Update();
	}

	if ( !iv_spawning_enabled.GetBool() )
		return;

	UpdateHorde();

	UpdateSpawningState();

	bool bWanderersEnabled = m_bWanderersEnabled || iv_wanderer_override.GetBool();
	if ( bWanderersEnabled )
	{
		UpdateWanderers();
	}
}

// increase intensity as NPC's are killed (particularly if they're close to the players)
void CIV_Director::Event_NPCKilled(CBaseEntity *pNPC, const CTakeDamageInfo &info)
{
	if (!pNPC)
		return;

	bool bDangerous = pNPC->Classify() == CLASS_COMBINE_HUNTER;
	bool bVeryDangerous = pNPC->Classify() == CLASS_COMBINE_GUNSHIP;

	int clients_count = gpGlobals->maxClients;

	if (clients_count <= 0)
		return;

	for (int i = 1; i <= clients_count; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetHealth() <= 0)
			continue;

		CIVPlayer_Intensity::IntensityType stress = CIVPlayer_Intensity::MILD;

		if ( bVeryDangerous )
		{
			stress = CIVPlayer_Intensity::EXTREME;
		}
		else if ( bDangerous )
		{
			stress = CIVPlayer_Intensity::HIGH;
		}
		else
		{
			float distance = pPlayer->GetAbsOrigin().DistTo(pNPC->GetAbsOrigin());
			if (distance > iv_intensity_far_range.GetFloat())
			{
				stress = CIVPlayer_Intensity::MILD;
			}
			else
			{
				stress = CIVPlayer_Intensity::MODERATE;
			}
		}

		pPlayer->GetIntensity()->Increase(stress);
	}
}

// increase intensity as players take damage
void CIV_Director::PlayerTookDamage(CBasePlayer *pPlayer, const CTakeDamageInfo &info, bool bFriendlyFire)
{
	if (!pPlayer)
		return;

	// friendly fire doesn't cause intensity increases
	if (bFriendlyFire)
		return;

	float flDamageRatio = info.GetDamage() / pPlayer->GetHealth();

	CIVPlayer_Intensity::IntensityType stress = CIVPlayer_Intensity::MILD;
	if ( flDamageRatio < 0.2f )
	{
		stress = CIVPlayer_Intensity::MODERATE;
	}
	else if ( flDamageRatio < 0.5f )
	{
		stress = CIVPlayer_Intensity::HIGH;
	}
	else
	{
		stress = CIVPlayer_Intensity::EXTREME;
	}

	pPlayer->GetIntensity()->Increase(stress);
}

void CIV_Director::UpdateHorde()
{
	if ( iv_director_debug.GetInt() > 0 )
	{
		if ( m_bHordeInProgress )
		{
			engine->Con_NPrintf( 11, "Horde in progress.  Left to spawn = %d", IVDirectorSpawnManager()->GetHordeToSpawn() );
		}

		engine->Con_NPrintf( 12, "Next Horde due: %f", m_HordeTimer.GetRemainingTime() );

		engine->Con_NPrintf(15, "Awake Common NPC's: %d\n", IVDirectorSpawnManager()->GetAwakeCommonNPCS());
		engine->Con_NPrintf(16, "Awake Special NPC's: %d\n", IVDirectorSpawnManager()->GetAwakeSpecialNPCS());
	}

	bool bHordesEnabled = m_bHordesEnabled || iv_horde_override.GetBool();

	if (!bHordesEnabled || !IVDirectorSpawnManager())
		return;

	if (!m_HordeTimer.HasStarted())
	{
		float flDuration = RandomFloat(iv_horde_interval_min.GetFloat(), iv_horde_interval_max.GetFloat());

		if (m_bFinale)
		{
			flDuration = RandomFloat( 5.0f, 10.0f );
		}
		if (iv_director_debug.GetBool())
		{
			Msg( "Will be spawning a horde in %f seconds\n", flDuration );
		}

		m_HordeTimer.Start(flDuration);
	}
	else if ( m_HordeTimer.IsElapsed() )
	{
		if (IVDirectorSpawnManager()->GetAwakeCommonNPCS() < 25)
		{
			int iNumCommonNPCS = RandomInt( iv_horde_size_min.GetInt(), iv_horde_size_max.GetInt() );

			if (IVDirectorSpawnManager()->AddHorde(iNumCommonNPCS))
			{
				if (iv_director_debug.GetBool())
				{
					Msg("Created horde of size %d\n", iNumCommonNPCS);
				}

				m_bHordeInProgress = true;

				/*if (ASWGameRules())
				{
					ASWGameRules()->BroadcastSound("Spawner.Horde");
				}*/

				m_HordeTimer.Invalidate();
			}
			else
			{
				// if we failed to find a horde position, try again shortly.
				m_HordeTimer.Start(RandomFloat(10.0f, 16.0f));
			}
		}
		else
		{
			// if there are currently too many awake NPC's, then wait 10 seconds before trying again
			m_HordeTimer.Start(10.0f);
		}
	}
}

void CIV_Director::OnHordeFinishedSpawning()
{
	if ( iv_director_debug.GetBool() )
	{
		Msg("Horde finishes spawning\n");
	}
	m_bHordeInProgress = false;
}

void CIV_Director::UpdateSpawningState()
{
	if (m_bFinale)				// in finale, just keep spawning NPC's forever
	{
		m_bSpawningNPCs = true;

		if (iv_director_debug.GetBool())
		{
			engine->Con_NPrintf(8, "%s: %f %s", m_bSpawningNPCs ? "Spawning NPC's" : "Relaxing",
				m_SustainTimer.HasStarted() ? m_SustainTimer.GetRemainingTime() : -1,
				"Finale");
		}

		return;
	}

	//=====================================================================================
	// Main director rollercoaster logic
	//   Spawns NPC's until a peak intensity is reached, then gives the Players a breather
	//=====================================================================================

	if (!m_bSpawningNPCs)			// not spawning NPC's, we're in a relaxed state
	{
		if (!m_SustainTimer.HasStarted())
		{
			if ( GetMaxIntensity() < 1.0f )	// don't start our relax timer until the Players have left the peak
			{
				if ( m_bInitialWait )		// just do a short delay before starting combat at the beginning of a mission
				{
					m_SustainTimer.Start( RandomFloat( 3.0f, 16.0f ) );
					m_bInitialWait = false;
				}
				else
				{
					m_SustainTimer.Start(RandomFloat(iv_director_relaxed_min_time.GetFloat(), iv_director_relaxed_max_time.GetFloat()));
				}
			}
		}
		else if (m_SustainTimer.IsElapsed())		// TODO: Should check their intensity meters are below a certain threshold?  Should probably also not wait if they run too far ahead
		{
			m_bSpawningNPCs = true;
			m_bReachedIntensityPeak = false;
			m_SustainTimer.Invalidate();
			m_fTimeBetweenNPCs = 0;
			m_NPCSpawnTimer.Invalidate();
		}
	}
	else								// we're spawning NPC's
	{
		if (m_bReachedIntensityPeak)
		{
			// hold the peak intensity for a while, then drop back to the relaxed state
			if (!m_SustainTimer.HasStarted())
			{
				m_SustainTimer.Start(RandomFloat(iv_director_peak_min_time.GetFloat(), iv_director_peak_max_time.GetFloat()));
			}
			else if (m_SustainTimer.IsElapsed())
			{
				m_bSpawningNPCs = false;
				m_SustainTimer.Invalidate();
			}
		}
		else
		{
			if ( GetMaxIntensity() >= 1.0f )
			{
				m_bReachedIntensityPeak = true;
			}
		}
	}

	if (iv_director_debug.GetInt() > 0)
	{
		engine->Con_NPrintf(8, "%s: %f %s", m_bSpawningNPCs ? "Spawning NPC's" : "Relaxing",
			m_SustainTimer.HasStarted() ? m_SustainTimer.GetRemainingTime() : -1,
			m_bReachedIntensityPeak ? "Peaked" : "Not peaked" );
	}
}

void CIV_Director::UpdateWanderers()
{
	if (!m_bSpawningNPCs)
	{
		if (iv_director_debug.GetInt() > 0)
		{
			engine->Con_NPrintf(9, "Not spawning regular NPC's");
		}
		return;
	}

	// spawn an NPC every so often
	if (!m_NPCSpawnTimer.HasStarted() || m_NPCSpawnTimer.IsElapsed())
	{
		if (m_fTimeBetweenNPCs == 0)
		{
			// initial time between alien spawns
			m_fTimeBetweenNPCs = RandomFloat(iv_interval_initial_min.GetFloat(), iv_interval_initial_max.GetFloat());
		}
		else
		{
			// reduce the time by some random amount each interval
			m_fTimeBetweenNPCs = MAX(iv_interval_min.GetFloat(),
				m_fTimeBetweenNPCs * RandomFloat(iv_interval_change_min.GetFloat(), iv_interval_change_max.GetFloat()));
		}
		if (iv_director_debug.GetInt() > 0)
		{
			engine->Con_NPrintf(9, "Regular spawn interval = %f", m_fTimeBetweenNPCs);
		}

		m_NPCSpawnTimer.Start(m_fTimeBetweenNPCs);

		if (IVDirectorSpawnManager())
		{
			if (IVDirectorSpawnManager()->GetAwakeCommonNPCS() < 20)
			{
				IVDirectorSpawnManager()->AddNPC();
			}
		}
	}
}

void CIV_Director::SetSpawnTableType(int sended_table_index)
{
	if (!IVDirectorSpawnManager())
		return;

	IVDirectorSpawnManager()->IV_Set_Spawn_Table(sended_table_index);
}

void CIV_Director::StartFinale()
{
	m_bFinale = true;
	m_bHordesEnabled = true;
	m_bWanderersEnabled = true;
	DevMsg("Starting finale\n");

	float flQuickStart = RandomFloat( 2.0f, 5.0f );
	if ( m_HordeTimer.GetRemainingTime() > flQuickStart )
	{
		m_HordeTimer.Start( flQuickStart );
	}
}
