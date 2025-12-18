#include "cg_local.h"

#if !defined(CG_LIGHTS_H_INC)
	#include "cg_lights.h"
#endif

static	clightstyle_t	cl_lightstyle[MAX_LIGHT_STYLES];
color5f_t		cl_lightstyle_mod[MAX_LIGHT_STYLES];
static	int				lastofs;

/*
================
FX_ClearLightStyles
================
*/
void CG_ClearLightStyles (void)
{
	int	i;

	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;

	for(i=0;i<MAX_LIGHT_STYLES*3;i++)
	{
		CG_SetLightstyle (i);
		cl_lightstyle_mod[i][0] = 1.0f;
		cl_lightstyle_mod[i][1] = 1.0f;
		cl_lightstyle_mod[i][2] = 1.0f;
		cl_lightstyle_mod[i][3] = 1.0f;
		cl_lightstyle_mod[i][4] = 1.0f; // [4] is magnitude to multiply rgb with
	}
}

/*
================
FX_RunLightStyles
================
*/
void CG_RunLightStyles (void)
{
	int		ofs;
	int		i;
	clightstyle_t	*ls;

	ofs = cg.time / 50;
//	if (ofs == lastofs)
//		return;
	lastofs = ofs;

	for (i=0,ls=cl_lightstyle ; i<MAX_LIGHT_STYLES ; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = ls->value[3] = 255;
		}
		else if (ls->length == 1)
		{
			ls->value[0] = ls->map[0][0];
			ls->value[1] = ls->map[0][1];
			ls->value[2] = ls->map[0][2];
			ls->value[3] = 255; //ls->map[0][3];
		}
		else
		{
			ls->value[0] = ls->map[ofs%ls->length][0];
			ls->value[1] = ls->map[ofs%ls->length][1];
			ls->value[2] = ls->map[ofs%ls->length][2];
			ls->value[3] = 255; //ls->map[ofs%ls->length][3];
		}
		ls->value[0] *= cl_lightstyle_mod[i][0] * cl_lightstyle_mod[i][4];
		ls->value[1] *= cl_lightstyle_mod[i][1] * cl_lightstyle_mod[i][4];
		ls->value[2] *= cl_lightstyle_mod[i][2] * cl_lightstyle_mod[i][4];
		ls->value[3] *= cl_lightstyle_mod[i][3];
		trap_R_SetLightStyle(i, ls->value);
	}
}

const float onedividedby255 = 1.0f / 255.0f;

void CG_SetLightstyle (int i)
{
	const char	*s;
	int			j, k;

	s = CG_ConfigString( i+CS_LIGHT_STYLES );
	j = strlen (s);
	if (j >= MAX_QPATH)
	{
		Com_Error (ERR_DROP, "svc_lightstyle length=%i", j);
	}

	cl_lightstyle[(i/3)].length = j;
	for (k=0 ; k<j ; k++)
	{
		cl_lightstyle[(i/3)].map[k][(i%3)] = (float)(s[k]-'a')/(float)('z'-'a') * 255.0;
		cl_lightstyle[(i / 3)].map[k][(i % 3)] = 255.0f * sRGBToLinear(cl_lightstyle[(i / 3)].map[k][(i % 3)] * onedividedby255);
	}
}
