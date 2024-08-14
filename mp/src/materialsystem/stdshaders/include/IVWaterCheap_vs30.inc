// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class IVWaterCheap_vs30_Static_Index
{
	unsigned int m_nBLEND : 2;
#ifdef _DEBUG
	bool m_bBLEND : 1;
#endif	// _DEBUG
public:
	void SetBLEND( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLEND = i;
#ifdef _DEBUG
		m_bBLEND = true;
#endif	// _DEBUG
	}

	IVWaterCheap_vs30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
		m_nBLEND = 0;
#ifdef _DEBUG
		m_bBLEND = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bBLEND );
		return ( 1 * m_nBLEND ) + 0;
	}
};

#define shaderStaticTest_IVWaterCheap_vs30 vsh_forgot_to_set_static_BLEND


class IVWaterCheap_vs30_Dynamic_Index
{
public:
	IVWaterCheap_vs30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_IVWaterCheap_vs30 1
