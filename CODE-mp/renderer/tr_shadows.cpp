#include "tr_local.h"


/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES/3];

void R_AddEdgeDef( int i1, int i2, int facing ) {
	int		c;

	c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}


void R_RenderShadowCaps( void ) {
	int		i;
	int		numTris;
	numTris = tess.numIndexes / 3;

	qglBegin(GL_TRIANGLES);
	for (i = 0; i < numTris; i++) {
		int		i1, i2, i3;

		if (!facing[i]) {
			continue;
		}

		i1 = tess.indexes[i * 3 + 0];
		i2 = tess.indexes[i * 3 + 1];
		i3 = tess.indexes[i * 3 + 2];

		qglVertex3fv(tess.xyz[i1]);
		qglVertex3fv(tess.xyz[i2]);
		qglVertex3fv(tess.xyz[i3]);
		qglVertex3fv(tess.xyz[i3 + tess.numVertexes]);
		qglVertex3fv(tess.xyz[i2 + tess.numVertexes]);
		qglVertex3fv(tess.xyz[i1 + tess.numVertexes]);		
	}
	qglEnd();
}

void R_RenderShadowEdges( void ) {
	int		i;

#if 0
	int		numTris;

	// dumb way -- render every triangle's edges
	numTris = tess.numIndexes / 3;

	for ( i = 0 ; i < numTris ; i++ ) {
		int		i1, i2, i3;

		if ( !facing[i] ) {
			continue;
		}

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		qglBegin( GL_TRIANGLE_STRIP );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( tess.xyz[ i1 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i2 ] );
		qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i3 ] );
		qglVertex3fv( tess.xyz[ i3 + tess.numVertexes ] );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( tess.xyz[ i1 + tess.numVertexes ] );
		qglEnd();
	}
#else
	int		c, c2;
	int		j, k;
	int		i2;
	int		c_edges, c_rejected;
	int		hit[2];

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	c_edges = 0;
	c_rejected = 0;

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( hit[ 1 ] == 0 ) {
				//qglBegin( GL_TRIANGLE_STRIP );
				//qglVertex3fv( tess.xyz[ i ] );
				//qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				//qglVertex3fv( tess.xyz[ i2 ] );
				//qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
				//qglEnd();
				
				// changing from GL_TRIANGLE_STRIP appears (?) to alleviate some weird crashes om AMD drivers where RAM usage insanely explodes to tens of GB in a few seconds, as well as invalid memory access crashes...
				qglBegin( GL_TRIANGLES );
				qglVertex3fv( tess.xyz[ i ] );
				qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				qglVertex3fv( tess.xyz[ i2 ] );
				
				qglVertex3fv( tess.xyz[ i2 ] );
				qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
				qglEnd();
				c_edges++;
			} else {
				c_rejected++;
			}
		}
	}
#endif
}

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd( void ) {
	int		i;
	int		numTris;
	vec3_t	lightDir; 
	float	expandLength = 512;
	vec3_t projectorPos; 
	qboolean	zFail = (qboolean)(r_stencilShadowZFail->integer && glConfig.depthClamp);

	// TODO Depth Fail ("Carmack's Reverse")?
	// TODO reject normals that align with the shadow/light direction, so we don't shadow undersides of stuff (looks bad)

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return;
	}

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

	if (g_bRenderZPrepass && !g_bRenderProjectorPrepass) {
		return;
	}

	if (g_bRenderProjectorPrepass && tess.shader != tr.projectorshadowShader) {
		return;
	}
	else if (!g_bRenderProjectorPrepass && tess.shader != tr.shadowShader) {
		return;
	}

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

	if (tess.shader == tr.shadowShader && r_shadows->integer == 4) {
		vec3_t sunDirection;
		if (!r_shadowSunDirOverride->integer) {
			VectorCopy(tr.sunDirection, sunDirection);
		}
		else {
			VectorCopy(tr.shadowSunDirectionOverride, sunDirection);
		}
		if (backEnd.currentEntity == &tr.worldEntity) {
			VectorCopy(sunDirection, lightDir);
		}
		else {
			lightDir[0] = DotProduct(sunDirection, backEnd.currentEntity->e.axis[0]);
			lightDir[1] = DotProduct(sunDirection, backEnd.currentEntity->e.axis[1]);
			lightDir[2] = DotProduct(sunDirection, backEnd.currentEntity->e.axis[2]);
			VectorNormalize(lightDir);
		}
	}

	//expandLength = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane + 64 + 50; // TA: let's go distance to shadowplane, plus playerheight, plus a bit extra so some angles are covered. don't go 512 units like in the original so we don't cast shadows through 200 walls. TODO restrict normals too so we don't draw on undersides of geometry we stand on. meh, doesnt rly work.

	// project vertexes away from light direction
	if (g_bRenderProjectorPrepass) {
		expandLength = 9999;
		if (backEnd.currentEntity == &tr.worldEntity) {
			VectorCopy(tr.projector.pos, projectorPos);
		}
		else {
			vec3_t tmp;
			VectorSubtract(tr.projector.pos, backEnd.currentEntity->e.origin, tmp);

			projectorPos[0] = DotProduct(tmp, backEnd.currentEntity->e.axis[0]);
			projectorPos[1] = DotProduct(tmp, backEnd.currentEntity->e.axis[1]);
			projectorPos[2] = DotProduct(tmp, backEnd.currentEntity->e.axis[2]);
		}
		VectorCopy(projectorPos,lightDir);
		VectorNormalize(lightDir);
		for (i = 0; i < tess.numVertexes; i++) {
			vec3_t projDir;
			VectorSubtract(projectorPos, tess.xyz[i], projDir);
			VectorNormalize(projDir);
			VectorMA(tess.xyz[i], -expandLength, projDir, tess.xyz[i + tess.numVertexes]);
		}
	}
	else
	{
		for (i = 0; i < tess.numVertexes; i++) {
			VectorMA(tess.xyz[i], -expandLength, lightDir, tess.xyz[i + tess.numVertexes]);
		}
	}


	// decide which triangles face the light
	Com_Memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) {
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;
		vec3_t projDir;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		if (g_bRenderProjectorPrepass) {
			vec3_t avg = { 0,0,0 };
			VectorAdd(avg, tess.xyz[i1],avg);
			VectorAdd(avg, tess.xyz[i2],avg);
			VectorAdd(avg, tess.xyz[i3],avg);
			VectorScale(avg, 0.3333333333f, avg);
			VectorSubtract(projectorPos, avg, projDir);
			VectorNormalize(projDir);
			d = DotProduct(normal, projDir);
		}
		else {
			d = DotProduct(normal, lightDir);
		}
		if ( d > 0 ) {
			facing[ i ] = 1;
		} else {
			facing[ i ] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	// draw the silhouette edges

	GL_Bind( tr.whiteImage );
	qglEnable( GL_CULL_FACE );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	qglColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	if (zFail) {
		qglDepthFunc(GL_LESS); // TODO why do i have to do this? I think my caps are being done incorrectly..
		// mirrors have the culling order reversed
		if ( backEnd.viewParms.isMirror ) {
			qglCullFace( GL_BACK );
			qglStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

			R_RenderShadowCaps();
			R_RenderShadowEdges();

			qglCullFace( GL_FRONT );
			qglStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

			R_RenderShadowCaps();
			R_RenderShadowEdges();
		} else {
			qglCullFace( GL_FRONT );
			qglStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

			R_RenderShadowCaps();
			R_RenderShadowEdges();

			qglCullFace( GL_BACK );
			qglStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

			R_RenderShadowCaps();
			R_RenderShadowEdges();
		}
		qglDepthFunc(GL_LEQUAL);
	}
	else {
		// mirrors have the culling order reversed
		if ( backEnd.viewParms.isMirror ) {
			qglCullFace( GL_FRONT );
			qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

			R_RenderShadowEdges();

			qglCullFace( GL_BACK );
			qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

			R_RenderShadowEdges();
		} else {
			qglCullFace( GL_BACK );
			qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

			R_RenderShadowEdges();

			qglCullFace( GL_FRONT );
			qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

			R_RenderShadowEdges();
		}
	}

	// reenable writing to the color buffer
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	qglDisable( GL_CULL_FACE );

	tess.numIndexes = 0;
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void ) {
	if ( r_shadows->integer != 2 && r_shadows->integer != 4 && (!r_fboGLSLProjector || !r_fboGLSLProjector->integer) ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}
	if (g_bRenderZPrepass && !g_bRenderProjectorPrepass) {
		return;
	}

	if (g_bRenderProjectorPrepass) {
		R_FrameBuffer_SetDrawingShadowPrepass(qtrue,qtrue,qfalse);
	}

	qglEnable( GL_STENCIL_TEST );
	if (r_stencilShadowInverse->integer) {
		qglStencilFunc(GL_EQUAL, 0, 255);
	}
	else {
		qglStencilFunc(GL_NOTEQUAL, 0, 255);
	}

	//qglDisable (GL_CLIP_PLANE0);
	R_DeActivateClipPlane(4, qtrue);
	qglDisable (GL_CULL_FACE);

	GL_Bind( tr.whiteImage );

    qglLoadIdentity ();
	
	qglColor3f( tr.stencilShadowColor[0], tr.stencilShadowColor[1], tr.stencilShadowColor[2]);
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	//qglBegin( GL_QUADS );
	//qglVertex3f( -100, 100, -10 );
	//qglVertex3f( 100, 100, -10 );
	//qglVertex3f( 100, -100, -10 );
	//qglVertex3f( -100, -100, -10 );
	//qglEnd ();


	qglBegin( GL_TRIANGLES );
	qglVertex3f( -100, 100, -10 );
	qglVertex3f( 100, 100, -10 );
	qglVertex3f( 100, -100, -10 );

	qglVertex3f(-100, 100, -10);
	qglVertex3f(100, -100, -10);
	qglVertex3f( -100, -100, -10 );
	qglEnd ();

	qglColor4f(1,1,1,1);
	qglDisable( GL_STENCIL_TEST );

	if (g_bRenderProjectorPrepass) {
		R_FrameBuffer_SetDrawingShadowPrepass(qfalse,qfalse,(qboolean)(r_fboGLSLProjector && r_fboGLSLProjector->integer > 1));
	}
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void ) {
	float	*xyz;
	int		i;
	float	h;
	vec3_t	ground;
	vec3_t	light;
	float	groundDist;
	float	d;
	vec3_t	lightDir;

	xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	groundDist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
