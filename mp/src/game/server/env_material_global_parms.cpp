//========= Created by Ivan Suvorov and THS inc 2024. ============//
//
// Purpose: Material System Global Parms In Game Control.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//------------------------------------------------------------------------------
// Purpose : Material Global Parms entity
//------------------------------------------------------------------------------
class CMatGlobalParmsControl : public CBaseEntity
{
public:
	DECLARE_CLASS(CMatGlobalParmsControl, CBaseEntity);

	CMatGlobalParmsControl();

	void Spawn(void);
	bool KeyValue(const char *szKeyName, const char *szValue);
	int  UpdateTransmitState();

	virtual int	ObjectCaps(void) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	CNetworkVar(bool, m_bForceEnablePhongOnBrushes);
	CNetworkVar(bool, m_bForceEnableAnisotopicReflection);
};

LINK_ENTITY_TO_CLASS(env_material_global_parms, CMatGlobalParmsControl);

BEGIN_DATADESC(CMatGlobalParmsControl)

DEFINE_KEYFIELD(m_bForceEnablePhongOnBrushes, FIELD_BOOLEAN, "forcephongonbrushes"),
DEFINE_KEYFIELD(m_bForceEnableAnisotopicReflection, FIELD_BOOLEAN, "forceanisotopicreflection"),

	// Inputs
	//IV Note: Empty for Now!!!

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST_NOBASE(CMatGlobalParmsControl, DT_MatGlobalParmsControl)
SendPropBool(SENDINFO(m_bForceEnablePhongOnBrushes)),
SendPropBool(SENDINFO(m_bForceEnableAnisotopicReflection)),
END_SEND_TABLE()


CMatGlobalParmsControl::CMatGlobalParmsControl()
{
	m_bForceEnablePhongOnBrushes = false;
	m_bForceEnableAnisotopicReflection = false;
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CMatGlobalParmsControl::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState(FL_EDICT_ALWAYS);
}

bool CMatGlobalParmsControl::KeyValue(const char *szKeyName, const char *szValue)
{
	return BaseClass::KeyValue( szKeyName, szValue );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CMatGlobalParmsControl::Spawn(void)
{
	Precache();
	SetSolid(SOLID_NONE);
}
