//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "mathlib/VMatrix.h"
#include "ivdev_lightshafts_helper.h"
#include "convar.h"
#include "cpp_shader_constant_register_map.h"

// Auto generated inc files
#include "ivdev_lightshafts_vs30.inc"
#include "ivdev_lightshafts_ps30.inc"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


void InitParamsLightShafts( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, LightShaftsVars_t &info )
{
	// Set material flags
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	SET_FLAGS( MATERIAL_VAR_NOCULL );
	SET_PARAM_INT_IF_NOT_DEFINED( info.m_nCookieFrameNum, 0 );
	SET_PARAM_FLOAT_IF_NOT_DEFINED( info.m_nTime, 0.0f );
}

void InitLightShafts( CBaseVSShader *pShader, IMaterialVar** params, LightShaftsVars_t &info )
{
	// Load textures
	if ( (info.m_nCookieTexture != -1) && params[info.m_nCookieTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nCookieTexture );
	}

	if ( (info.m_nShadowDepthTexture != -1) && params[info.m_nShadowDepthTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nShadowDepthTexture );
	}

	if ( (info.m_nNoiseTexture != -1) && params[info.m_nNoiseTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nNoiseTexture );
	}
}

void DrawLightShafts( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					  IShaderShadow* pShaderShadow, LightShaftsVars_t &info, VertexCompressionType_t vertexCompression )
{
	SHADOW_STATE
	{
		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat( flags, 0, NULL, 0 ); // no texture coordinates needed
		
		DECLARE_STATIC_VERTEX_SHADER( ivdev_lightshafts_vs30 );
		SET_STATIC_VERTEX_SHADER( ivdev_lightshafts_vs30 );
	
		DECLARE_STATIC_PIXEL_SHADER( ivdev_lightshafts_ps30 );
		
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER( ivdev_lightshafts_ps30 );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );					// Cookie texture
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );					// Shadow depth texture
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
		pShaderShadow->SetShadowDepthFiltering( SHADER_SAMPLER1 );

		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );					// Screen-space noise map for shadow filtering
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, false );

		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );					// Projective noise map used for non-uniform atmospherics
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false );

		pShaderShadow->EnableSRGBWrite( true );

		pShader->EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );		// Additive blending
		pShaderShadow->EnableAlphaWrites( false );
	}
	DYNAMIC_STATE
	{
		DECLARE_DYNAMIC_VERTEX_SHADER( ivdev_lightshafts_vs30 );
		SET_DYNAMIC_VERTEX_SHADER( ivdev_lightshafts_vs30 );

		//
		// Read material vars into relevant members of flashlightState...kinda icky and verbose...
		//
		VMatrix worldToTexture;
		FlashlightState_t flashlightState;

		if ( (info.m_nWorldToTexture != -1) && params[info.m_nWorldToTexture]->IsDefined() )
		{
			const VMatrix &mat = params[info.m_nWorldToTexture]->GetMatrixValue();
			worldToTexture = mat;
		}

		if ( (info.m_nFlashlightColor != -1) && params[info.m_nFlashlightColor]->IsDefined() )
		{
			params[info.m_nFlashlightColor]->GetVecValue( &(flashlightState.m_Color[0]), 4 );
		}
		else
		{
			flashlightState.m_Color[0] = flashlightState.m_Color[1] = flashlightState.m_Color[2] = flashlightState.m_Color[3] = 1.0f;
		}

		// Pre-modulate with intensity factor
		if ( (info.m_nVolumetricIntensity != -1) && params[info.m_nVolumetricIntensity]->IsDefined() )
		{
			float flVolumetricIntensity = params[info.m_nVolumetricIntensity]->GetFloatValue();
			flashlightState.m_Color[0] *= flVolumetricIntensity;
			flashlightState.m_Color[1] *= flVolumetricIntensity;
			flashlightState.m_Color[2] *= flVolumetricIntensity;
		}

		SetFlashLightColorFromState( flashlightState, pShaderAPI, false, PSREG_FLASHLIGHT_COLOR );

		if ( (info.m_nAttenFactors != -1) && params[info.m_nAttenFactors]->IsDefined() )
		{
			float v[4];
			params[info.m_nAttenFactors]->GetVecValue( v, 4 );
			flashlightState.m_fConstantAtten = v[0];
			flashlightState.m_fLinearAtten = v[1];
			flashlightState.m_fQuadraticAtten = v[2];
			//flashlightState.m_FarZAtten = v[3];
		}

		if ( (info.m_nOriginFarZ != -1) && params[info.m_nOriginFarZ]->IsDefined() )
		{
			float v[4];
			params[info.m_nOriginFarZ]->GetVecValue( v, 4 );
			flashlightState.m_vecLightOrigin[0] = v[0];
			flashlightState.m_vecLightOrigin[1] = v[1];
			flashlightState.m_vecLightOrigin[2] = v[2];
			flashlightState.m_FarZ = v[3];
		}

		if ( (info.m_nQuatOrientation != -1) && params[info.m_nQuatOrientation]->IsDefined() )
		{
			params[info.m_nQuatOrientation]->GetVecValue( flashlightState.m_quatOrientation.Base(), 4 );
		}

		if ( (info.m_nShadowFilterSize != -1) && params[info.m_nShadowFilterSize]->IsDefined() )
		{
			flashlightState.m_flShadowFilterSize = params[info.m_nShadowFilterSize]->GetFloatValue();
		}

		if ( (info.m_nShadowAtten != -1) && params[info.m_nShadowAtten]->IsDefined() )
		{
			flashlightState.m_flShadowAtten = params[info.m_nShadowAtten]->GetFloatValue();
		}

		if ( (info.m_nShadowJitterSeed != -1) && params[info.m_nShadowJitterSeed]->IsDefined() )
		{
			flashlightState.m_flShadowJitterSeed = params[info.m_nShadowJitterSeed]->GetFloatValue();
		}

		if ( (info.m_nEnableShadows != -1) && params[info.m_nEnableShadows]->IsDefined() )
		{
			flashlightState.m_bEnableShadows = ( params[info.m_nEnableShadows]->GetIntValue() != 0 );
		}

		if ( (info.m_nCookieTexture != -1) && params[info.m_nCookieTexture]->IsDefined() )
		{
			ITexture *pCookieTexture = params[info.m_nCookieTexture]->GetTextureValue();
			int nFrameNumber = params[info.m_nCookieFrameNum]->GetIntValue();
			pShader->BindTexture( SHADER_SAMPLER0, pCookieTexture, nFrameNumber );
		}

		ITexture *pFlashlightDepthTexture = NULL;
		if( (info.m_nShadowDepthTexture != -1) && params[info.m_nShadowDepthTexture]->IsDefined() &&
			 g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows )
		{
			pFlashlightDepthTexture = params[info.m_nShadowDepthTexture]->GetTextureValue();
			pShader->BindTexture( SHADER_SAMPLER1, pFlashlightDepthTexture );

			pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_SHADOW_NOISE_2D );
		}

		if( (info.m_nNoiseTexture != -1) && params[info.m_nNoiseTexture]->IsDefined() )
		{
			ITexture *pNoiseTexture = params[info.m_nNoiseTexture]->GetTextureValue();
			pShader->BindTexture( SHADER_SAMPLER3, pNoiseTexture );
		}

		//
		// Now that we've packed the flashlightState structure from our material vars, we can set constants and draw as normal
		//

		float atten[4], pos[4], tweaks[4], packedParams[4], noiseScroll[4];
		atten[0] = flashlightState.m_fConstantAtten;		// Set the flashlight attenuation factors
		atten[1] = flashlightState.m_fLinearAtten;
		atten[2] = flashlightState.m_fQuadraticAtten;
		//atten[3] = flashlightState.m_FarZAtten;
		pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

		pos[0] = flashlightState.m_vecLightOrigin[0];		// Set the flashlight origin
		pos[1] = flashlightState.m_vecLightOrigin[1];
		pos[2] = flashlightState.m_vecLightOrigin[2];
		pos[3] = flashlightState.m_FarZ;
		pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1);	// steps on rim boost

		//IV Note: Params Extra Section
		float temp_noise_parm = (info.m_nNoiseStrength != -1) && params[info.m_nNoiseStrength]->IsDefined() ? params[info.m_nNoiseStrength]->GetFloatValue() : 1;
		int temp_num_planes = (info.m_nNumPlanes != -1) && params[info.m_nNumPlanes]->IsDefined() ? params[info.m_nNumPlanes]->GetIntValue() : 1;
		float temp_flashlight_time = (info.m_nFlashlightTime != -1) && params[info.m_nFlashlightTime]->IsDefined() ? params[info.m_nFlashlightTime]->GetFloatValue() : 1;

		// Coefficient for projective noise
		packedParams[0] = temp_noise_parm;
		packedParams[1] = 64.0f / (float)temp_num_planes;
		packedParams[2] = 0.0f;
		packedParams[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant( 0, packedParams, 1 );

		// Directions for projective noise
		noiseScroll[0] = fmodf(temp_flashlight_time * 0.043f *  0.394, 1.0f);	// UV offset for noise in red
		noiseScroll[1] = fmodf(temp_flashlight_time * 0.043f *  0.919, 1.0f);
		noiseScroll[2] = fmodf(temp_flashlight_time * 0.039f * -0.781, 1.0f);	// UV offset for noise in green
		noiseScroll[3] = fmodf(temp_flashlight_time * 0.039f *  0.625, 1.0f);
		pShaderAPI->SetPixelShaderConstant( 1, noiseScroll, 1 );

		// Tweaks associated with a given flashlight
		tweaks[0] = ShadowFilterFromState( flashlightState );
		tweaks[1] = ShadowAttenFromState( flashlightState );
		pShader->HashShadow2DJitter( flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
		pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, worldToTexture.Base(), 4 );

		matrix3x4_t identityMatrix;
		SetIdentityMatrix(identityMatrix);
		pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, identityMatrix.Base(), 4);

		// Dimensions of screen, used for screen-space noise map sampling
		float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
		int nWidth, nHeight;
		pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );

		int nTexWidth, nTexHeight;
		pShaderAPI->GetStandardTextureDimensions( &nTexWidth, &nTexHeight, TEXTURE_SHADOW_NOISE_2D );

		vScreenScale[0] = (float) nWidth  / nTexWidth;
		vScreenScale[1] = (float) nHeight / nTexHeight;

		pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );

		DECLARE_DYNAMIC_PIXEL_SHADER( ivdev_lightshafts_ps30 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, flashlightState.m_bEnableShadows && ( pFlashlightDepthTexture != NULL ) );
		SET_DYNAMIC_PIXEL_SHADER( ivdev_lightshafts_ps30 );
	}
	pShader->Draw();
}
