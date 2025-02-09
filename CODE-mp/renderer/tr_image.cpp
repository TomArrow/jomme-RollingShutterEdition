// tr_image.c

#define SUPPORT_PNG

#ifdef SUPPORT_PNG
//#include <png.h>
//#include "png.h"
#include "../libpng/png.h"
#include "../zlib/zlib.h"
//#pragma comment (lib, "libpng.lib")
//#pragma comment (lib, "zlib.lib")
#endif

// TODO Use the Z_ alloc functions, but need to write a realloc for it...
//#define STBI_MALLOC(sz)           Z_Malloc(sz,TAG_TEMP_WORKSPACE)
//#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
//#define STBI_FREE(p)              Z_Free(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


#include "tr_local.h"
#include "glext.h"
#include <limits>
#include <algorithm>

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <map>
#pragma warning (pop)
using namespace std;


/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#define JPEG_INTERNALS
#include "../jpeg-9a/include/jpeglib.h"
//#include "../png/png.h"
#include "../png/rpng.h"

#ifndef DEDICATED
static void LoadTGA( const char *name, byte **pic, int *width, int *height );
static void LoadJPG( const char *name, byte **pic, int *width, int *height );

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

extern cvar_t* r_fbo;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

//#define FILE_HASH_SIZE		1024	// actually the shader code still needs this (from another module, great),
//static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;
	for ( i = 0; i < bufSize/4; i++ ) {
		buffer[i*4+0] = s_gammatable[buffer[i*4+0]];
		buffer[i*4+1] = s_gammatable[buffer[i*4+1]];
		buffer[i*4+2] = s_gammatable[buffer[i*4+2]];
		buffer[i*4+3] = s_gammatable[buffer[i*4+3]];
	}
}

typedef struct {
	char *name;
	int	minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};


// makeup a nice clean, consistant name to query for and file under, for map<> usage...
//
static char *GenerateImageMappingName( const char *name )
{
	static char sName[MAX_QPATH];
	int		i=0;
	char	letter;
	
	while (name[i] != '\0' && i<MAX_QPATH-1) 
	{
		letter = tolower(name[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		sName[i++] = letter;
	}
	sName[i]=0;
	
	return &sName[0];
}





/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int		i;
	image_t	*glt;

	for ( i=0 ; i< 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		ri.Printf (PRINT_ALL, "bad filter name\n");
		for ( i=0 ; i< 6 ; i++ ) {
			ri.Printf( PRINT_ALL, "%s\n",modes[i].name);
			}
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	   				 R_Images_StartIteration();
	while ( (glt   = R_Images_GetNextIteration()) != NULL)
	{
		if ( glt->mipmap ) {
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			if(glConfig.textureFilterAnisotropicAvailable) {
				if(r_ext_texture_filter_anisotropic->integer) {
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
				} else {
					qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				}
			}
		}
	}
}

static float R_BytesPerTex (int format)
{
	switch ( format ) {
	case 1:
		//"I    " 
		return 1;
		break;
	case 2:
		//"IA   " 
		return 2;
		break;
	case 3:
		//"RGB  " 
		return glConfig.colorBits/8.0f;
		break;
	case 4:
		//"RGBA " 
		return glConfig.colorBits/8.0f;
		break;
		
	case GL_RGBA4:
		//"RGBA4" 
		return 2;
		break;
	case GL_RGB5:
		//"RGB5 " 
		return 2;
		break;
		
	case GL_RGBA8:
		//"RGBA8" 
		return 4;
		break;
	case GL_RGB8:
		//"RGB8" 
		return 4;
		break;
		
	case GL_RGB4_S3TC:
		//"S3TC " 
		return 0.33333f;
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		//"DXT1 " 
		return 0.33333f;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		//"DXT5 " 
		return 1;
		break;
	default:
		//"???? " 
		return 4;
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
float R_SumOfUsedImages( qboolean bUseFormat ) 
{	
	int	total = 0;
	image_t *pImage;

					  R_Images_StartIteration();
	while ( (pImage = R_Images_GetNextIteration()) != NULL)
	{
		if ( pImage->frameUsed == tr.frameCount- 1 ) {//it has already been advanced for the next frame, so...
			if (bUseFormat)
			{
				float  bytePerTex = R_BytesPerTex (pImage->internalFormat);
				total += bytePerTex * (pImage->uploadWidth * pImage->uploadHeight);
			}
			else
			{
				total += pImage->uploadWidth * pImage->uploadHeight;
			}
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int		i=0;
	image_t	*image;
	int		texels=0;
	float	texBytes = 0.0f;
	const char *yesno[] = {"no ", "yes"};

	ri.Printf (PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n");

	int iNumImages = R_Images_StartIteration();
	while ( (image = R_Images_GetNextIteration()) != NULL)
	{
		texels   += image->uploadWidth*image->uploadHeight;
		texBytes += image->uploadWidth*image->uploadHeight * R_BytesPerTex (image->internalFormat);
		ri.Printf (PRINT_ALL,  "%4i: %4i %4i  %s   %d   ",
			i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU );
		switch ( image->internalFormat ) {
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC " );
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			ri.Printf( PRINT_ALL, "DXT1 " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri.Printf( PRINT_ALL, "DXT5 " );
			break;
		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;
		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;
		default:
			ri.Printf( PRINT_ALL, "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			ri.Printf( PRINT_ALL, "rept " );
			break;
		case GL_CLAMP:
			ri.Printf( PRINT_ALL, "clmp " );
			break;
		case GL_CLAMP_TO_EDGE:
			ri.Printf( PRINT_ALL, "clpE " );
			break;
		default:
			ri.Printf( PRINT_ALL, "%4i ", image->wrapClampMode );
			break;
		}
		
		ri.Printf( PRINT_ALL, "%s\n", image->imgName );
		i++;
	}
	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, "      -w-- -h-- -mm- -TMU- -if- wrap --name-------\n");
	ri.Printf (PRINT_ALL, " %i total texels (not including mipmaps)\n", texels );
	ri.Printf (PRINT_ALL, " %.2fMB total texture mem (not including mipmaps)\n", texBytes/1048576.0f );
	ri.Printf (PRINT_ALL, " %i total images\n\n", iNumImages );
}

//=======================================================================




/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range

templatized now. Not very efficient for non-bytes bc it can't use the gamma table.
But oh well.
================
*/
template<class T >
void R_LightScaleTexture (T *inNative, int inwidth, int inheight, qboolean only_gamma )
{
	//Important: in variable must be the all pixel type to preserve functionality
	typedef T pixelType_t[4];
	pixelType_t* in = (pixelType_t*)inNative;

	// Just quickly save max possible value of this into variable
#pragma push_macro("max") // I HATE HATE HATE HATE this. Why did they have to do a max macro...
#undef max
	constexpr float maxValue = std::numeric_limits<T>::max();
#pragma pop_macro("max")

	//T overBrightBitsMultiplier = pow(2,tr.overbrightBits);
	T overBrightBitsMultiplier = (T)tr.overbrightBitsMultiplier;

	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			T	*p;

			p = (T *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				/// ... uh oh. not good for float! honestly we need to do away with this stuff at some point...
				if constexpr (std::is_same<T,byte>::value) {
					p[0] = s_gammatable[p[0]];
					p[1] = s_gammatable[p[1]];
					p[2] = s_gammatable[p[2]];
				}
				else if constexpr(std::is_floating_point<T>::value) {
					// just straight up do the math...
					// Someday we're just gonna straight up get rid of all this nonsense. We have a float FBO for god's sake!
					// And we're using sRGB. Scaling that around would result in ridiculousness...
					p[0] = overBrightBitsMultiplier* pow(p[0], 1.0f / r_gamma->value);
					p[1] = overBrightBitsMultiplier* pow(p[1], 1.0f / r_gamma->value);
					p[2] = overBrightBitsMultiplier* pow(p[2], 1.0f / r_gamma->value);
				}
				else {
					// Integer type
					p[0] = std::clamp<T>(overBrightBitsMultiplier * maxValue* pow(p[0]/ maxValue, 1.0f / r_gamma->value),0,maxValue);
					p[1] = std::clamp<T>(overBrightBitsMultiplier * maxValue*pow(p[1]/ maxValue, 1.0f / r_gamma->value), 0, maxValue);
					p[2] = std::clamp<T>(overBrightBitsMultiplier * maxValue*pow(p[2]/ maxValue, 1.0f / r_gamma->value), 0, maxValue);
				}
			}
		}
	}
	else
	{
		int		i, c;
		T	*p;

		p = (T *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				if constexpr (std::is_same<T, byte>::value) {
					p[0] = s_intensitytable[p[0]];
					p[1] = s_intensitytable[p[1]];
					p[2] = s_intensitytable[p[2]];
				}
				else if constexpr (std::is_floating_point<T>::value) {
					// just straight up do the math...
					// Someday we're just gonna straight up get rid of all this nonsense. We have a float FBO for god's sake!
					// And we're using sRGB. Scaling that around would result in ridiculousness...
					p[0] = p[0]*r_intensity->value;
					p[1] = p[1]*r_intensity->value;
					p[2] = p[2]*r_intensity->value;
				}
				else {
					// Integer type
					p[0] = std::clamp<T>(p[0] * r_intensity->value, 0, maxValue);
					p[1] = std::clamp<T>(p[1] * r_intensity->value, 0, maxValue);
					p[2] = std::clamp<T>(p[2] * r_intensity->value, 0, maxValue);
				}
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				if constexpr (std::is_same<T, byte>::value) {
					p[0] = s_gammatable[s_intensitytable[p[0]]];
					p[1] = s_gammatable[s_intensitytable[p[1]]];
					p[2] = s_gammatable[s_intensitytable[p[2]]];
				}
				else if constexpr (std::is_floating_point<T>::value) {
					// just straight up do the math...
					// Someday we're just gonna straight up get rid of all this nonsense. We have a float FBO for god's sake!
					// And we're using sRGB. Scaling that around would result in ridiculousness...
					p[0] = overBrightBitsMultiplier * pow(p[0] * r_intensity->value, 1.0f / r_gamma->value);
					p[1] = overBrightBitsMultiplier * pow(p[1] * r_intensity->value, 1.0f / r_gamma->value);
					p[2] = overBrightBitsMultiplier * pow(p[2] * r_intensity->value, 1.0f / r_gamma->value);
				}
				else {
					// Integer type
					p[0] = std::clamp<T>(overBrightBitsMultiplier * maxValue * pow(p[0] / maxValue * r_intensity->value, 1.0f / r_gamma->value), 0, maxValue);
					p[1] = std::clamp<T>(overBrightBitsMultiplier * maxValue * pow(p[1] / maxValue * r_intensity->value, 1.0f / r_gamma->value), 0, maxValue);
					p[2] = std::clamp<T>(overBrightBitsMultiplier * maxValue * pow(p[2] / maxValue * r_intensity->value, 1.0f / r_gamma->value), 0, maxValue);
				}
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter

Templatized now. A bit hacky but ought to work?
================
*/
template<class T>
static void R_MipMap2( T *in, int inWidth, int inHeight ) {
	int			i, j, k;
	T		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;

	typedef T onePixel_t[4]; // lol. bc we wanna seek by pixels, not by individual channels.
	onePixel_t* temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (onePixel_t*)ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 *sizeof(T) );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (T *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((T *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((T *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((T *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((T *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((T *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((T *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((T *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((T *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((T *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((T *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((T *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 *sizeof(T) );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture

Turned into template so it can for example to float mipmapping or 16 bit mipmapping or whatever.
Makes a slight distinction between floating point and integer. (can't bitshift float)
================
*/
template<class T>
static void R_MipMap (T *in, int width, int height) {
	int		i, j;
	T	*out;
	int		row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			if constexpr (std::is_floating_point<T>::value) {
				out[0] = (in[0] + in[4]) /2.0;
				out[1] = (in[1] + in[5]) /2.0;
				out[2] = (in[2] + in[6]) /2.0;
				out[3] = (in[3] + in[7]) /2.0;
			}
			else {
				out[0] = (in[0] + in[4]) >> 1;
				out[1] = (in[1] + in[5]) >> 1;
				out[2] = (in[2] + in[6]) >> 1;
				out[3] = (in[3] + in[7]) >> 1;
			}
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			if constexpr (std::is_floating_point<T>::value) {
				out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) / 4.0;
				out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) / 4.0;
				out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) / 4.0;
				out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) / 4.0;
			} else {
				out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
				out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
				out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
				out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
			}
		}
	}
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels

Templatized.
==================
*/
template<class T>
static void R_BlendOverTexture( T *data, int pixelCount, byte blend[4] ) {
	int		i;
	/*
	if constexpr (std::is_same<T,byte>::value) {

		int		inverseAlpha;
		int		premult[3];

		inverseAlpha = 255 - blend[3];
		premult[0] = blend[0] * blend[3];
		premult[1] = blend[1] * blend[3];
		premult[2] = blend[2] * blend[3];

		for (i = 0; i < pixelCount; i++, data += 4) {
			data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
			data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
			data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
		}
	}
	else {*/


		if constexpr (std::is_floating_point<T>::value) {

			T blendScaled[4];
			blendScaled[0] = blend[1] / 255.0;
			blendScaled[1] = blend[2] / 255.0;
			blendScaled[2] = blend[3] / 255.0;
			blendScaled[3] = blend[4] / 255.0;

			T inverseAlpha = 1.0 - blendScaled[3];
			T premult[3];
			premult[0] = blendScaled[0] * blendScaled[3];
			premult[1] = blendScaled[1] * blendScaled[3];
			premult[2] = blendScaled[2] * blendScaled[3];

			for (i = 0; i < pixelCount; i++, data += 4) {
				data[0] = (data[0] * inverseAlpha + premult[0])/2.0; // Not 100% sure this is correct...
				data[1] = (data[1] * inverseAlpha + premult[1])/2.0;
				data[2] = (data[2] * inverseAlpha + premult[2])/2.0;
			}
		}
		else {
			// Integer type
			// Should end up being the old code basically if sizeof(T) == 1

			constexpr int shiftPremult = 8 * (sizeof(T) - 1);
			typedef std::conditional<sizeof(T) <= 2, int, int64_t>::type premultInt_t; // We need sizeof(T)+1 bytes here. because it's T multiplied by 255

			int				inverseAlpha;
			premultInt_t	premult[3];

			inverseAlpha = 255 - blend[3];
			premult[0] = ((premultInt_t) blend[3] << shiftPremult) * blend[0];
			premult[1] = ((premultInt_t) blend[3] << shiftPremult) * blend[1];
			premult[2] = ((premultInt_t) blend[3] << shiftPremult) * blend[2];

			for (i = 0; i < pixelCount; i++, data += 4) {
				data[0] = (data[0] * inverseAlpha + premult[0]) >> 9; // Stays at 9. 8 for 255 factor. 1 for averaging both values.
				data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
				data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
			}
		}
	//}
	
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};





class CStringComparator
{
public:
	bool operator()(const char *s1, const char *s2) const { return(strcmp(s1, s2) < 0); } 
};

typedef map <LPCSTR, image_t *, CStringComparator>	AllocatedImages_t;
													AllocatedImages_t AllocatedImages;
													AllocatedImages_t::iterator itAllocatedImages;
int giTextureBindNum = 1024;	// will be set to this anyway at runtime, but wtf?


// return = number of images in the list, for those interested
//
int R_Images_StartIteration(void)
{
	itAllocatedImages = AllocatedImages.begin();
	return AllocatedImages.size();
}

image_t *R_Images_GetNextIteration(void)
{
	if (itAllocatedImages == AllocatedImages.end())
		return NULL;

	image_t *pImage = (*itAllocatedImages).second;
	++itAllocatedImages;
	return pImage;
}

// clean up anything to do with an image_t struct, but caller will have to clear the internal to an image_t struct ready for either struct free() or overwrite...
//
// (avoid using ri.xxxx stuff here in case running on dedicated)
//
static void R_Images_DeleteImageContents( image_t *pImage )
{
	assert(pImage);	// should never be called with NULL
	if (pImage)
	{
		if (qglDeleteTextures) {	//won't have one if we switched to dedicated.
			qglDeleteTextures( 1, &pImage->texnum );
		}
		Z_Free(pImage);
	}
}





/*
===============
Upload32

===============
*/
extern qboolean charSet;
//static void Upload32( unsigned *data, 
template<class T>
static void Upload32( T *picData, 
						 int img_width, int img_height, 
						 qboolean mipmap, 
						 qboolean picmip, 
						 qboolean isLightmap,
						 qboolean allowTC,
						 int *pformat, 
						 int *pUploadWidth, int *pUploadHeight,TextureBitsPerChannel bpc )
{
	int			samples;
	int			i, c;
	T			*scan;
	float		rMax = 0, gMax = 0, bMax = 0;
	int			width = img_width;
	int			height = img_height; 


	int sourceDataFormat = GL_UNSIGNED_BYTE;
	switch (bpc) {
	case BPC_32FLOAT:
		sourceDataFormat = GL_FLOAT;
		break;
	case BPC_32BIT:
		sourceDataFormat = GL_UNSIGNED_INT;
		break;
	case BPC_16BIT:
		sourceDataFormat = GL_UNSIGNED_SHORT;
		break;
	case BPC_8BIT:
	default:
		sourceDataFormat = GL_UNSIGNED_BYTE;
		break;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		for(i = 0; i < r_picmip->integer; i++) {

			R_MipMap(picData, width, height);
			width >>= 1;
			height >>= 1;
			if (width < 1) {
				width = 1;
			}
			if (height < 1) {
				height = 1;
			}
		}
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( width > glConfig.maxTextureSize	|| height > glConfig.maxTextureSize ) {
		// Call templatized mipmap.
		R_MipMap(picData, width, height);
		width >>= 1;
		height >>= 1;
	}

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((T *)picData);
	samples = 3;
	for ( i = 0; i < c; i++ )
	{
		if ( scan[i*4+0] > rMax )
		{
			rMax = scan[i*4+0];
		}
		if ( scan[i*4+1] > gMax )
		{
			gMax = scan[i*4+1];
		}
		if ( scan[i*4+2] > bMax )
		{
			bMax = scan[i*4+2];
		}
		if constexpr (std::is_floating_point<T>::value) {
			if (scan[i * 4 + 3] != 1.0)
			{
				samples = 4;
				break;
			}
		}
		else {
#pragma push_macro("max") // I HATE HATE HATE HATE this. Why did they have to do a max macro...
#undef max
			if (scan[i * 4 + 3] != std::numeric_limits<T>::max())
			{
				samples = 4;
				break;
			}
#pragma pop_macro("max")
		}
		
	}

	bool isByte = std::is_same<T, byte>::value; // I don't think we should use texture compression for anything but byte

	// select proper internal format
	if ( samples == 3 )
	{
		if ( glConfig.textureCompression == TC_S3TC && allowTC && isByte)
		{
			*pformat = GL_RGB4_S3TC;
		}
		else if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC && isByte)
		{	// Compress purely color - no alpha
			if ( r_texturebits->integer == 16 ) {
				*pformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;	//this format cuts to 16 bit
			}
			else {//if we aren't using 16 bit then, use 32 bit compression
				*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
		}
		else if ( isLightmap && r_texturebitslm->integer > 0)
		{
			// Allow different bit depth when we are a lightmap
			if ( r_texturebitslm->integer == 16 && isByte)
			{
				*pformat = GL_RGB5;
			}
			else if ( r_texturebitslm->integer == 32 && isByte)
			{
				*pformat = GL_RGB8;
			}
			else if (bpc == BPC_32FLOAT) //floating point lightmap?  doubt its even possible but whatever.
			{
				*pformat = GL_RGB16F;
			}
		}
		else if ( r_texturebits->integer == 16 && isByte)
		{
			*pformat = GL_RGB5;
		}
		else if ( r_texturebits->integer == 32 && isByte)
		{
			//*pformat = GL_RGB8;
			*pformat = GL_SRGB8;
		}
		else if (bpc == BPC_32FLOAT) 
		{
			// TODO For legacy gamma mode, allow 16 bit textures too.
			// Cant for the srgb stuff because opengl doesnt offer a 16 bit srgb internalformat... sadders.

			// 32 bit float is already linear so no need to worry about sRGB
			// Internally we will use 16 bit float to save memory. More isn't necessary anyway for a texture...
			*pformat = GL_RGB16F;
		}
		else {
			//*pformat = 3;
			*pformat = GL_SRGB8;
		}
	}
	else if ( samples == 4 )
	{
		if ( glConfig.textureCompression == TC_S3TC_DXT && allowTC && isByte)
		{	// Compress both alpha and color
			*pformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else if ( r_texturebits->integer == 16 && isByte)
		{
			*pformat = GL_RGBA4;
		}
		else if ( r_texturebits->integer == 32 && isByte)
		{
			//*pformat = GL_RGBA8;
			*pformat = GL_SRGB8_ALPHA8;
		}
		else if (bpc == BPC_32FLOAT)
		{
			// TODO For legacy gamma mode, allow 16 bit textures too.
			// Cant for the srgb stuff because opengl doesnt offer a 16 bit srgb internalformat... sadders.

			// 32 bit float is already linear so no need to worry about sRGB
			// Internally we will use 16 bit float to save memory. More isn't necessary anyway for a texture...
			*pformat = GL_RGBA16F;
		}
		else
		{
			//*pformat = 4;
			*pformat = GL_SRGB8_ALPHA8;
		}
	}

	*pUploadWidth = width;
	*pUploadHeight = height;

	// copy or resample data as appropriate for first MIP level
	if (!mipmap)
	{
		qglTexImage2D (GL_TEXTURE_2D, 0, *pformat, width, height, 0, GL_RGBA, sourceDataFormat, picData);
		goto done;
	}

	R_LightScaleTexture (picData, width, height, (qboolean)!mipmap );

	qglTexImage2D (GL_TEXTURE_2D, 0, *pformat, width, height, 0, GL_RGBA, sourceDataFormat, picData );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (width > 1 || height > 1)
		{
			R_MipMap( picData, width, height );
			width >>= 1;
			height >>= 1;
			if (width < 1)
				width = 1;
			if (height < 1)
				height = 1;
			miplevel++;

			if ( r_colorMipLevels->integer ) 
			{
				R_BlendOverTexture( picData, width * height, mipBlendColors[miplevel] );
			}

			qglTexImage2D (GL_TEXTURE_2D, miplevel, *pformat, width, height, 0, GL_RGBA, sourceDataFormat, picData );
		}
	}
done:

	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		if(r_ext_texture_filter_anisotropic->integer && glConfig.textureFilterAnisotropicAvailable) {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
		}
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();
}


static void GL_ResetBinds(void)
{
	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) 
	{
		if ( qglActiveTextureARB ) 
		{
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} 
		else 
		{
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}


// special function used in conjunction with "devmapbsp"...
//
// (avoid using ri.xxxx stuff here in case running on dedicated)
//
void R_Images_DeleteLightMaps(void)
{
	qboolean bEraseOccured = qfalse;
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); bEraseOccured?itImage:++itImage)
	{			
		bEraseOccured = qfalse;

		image_t *pImage = (*itImage).second;
		
		if (pImage->imgName[0] == '*' && strstr(pImage->imgName,"lightmap"))	// loose check, but should be ok
		{
			R_Images_DeleteImageContents(pImage);
			AllocatedImages.erase(itImage++);
			bEraseOccured = qtrue;
		}
	}

	GL_ResetBinds();
}

// special function currently only called by Dissolve code...
//
void R_Images_DeleteImage(image_t *pImage)
{		
	// Even though we supply the image handle, we need to get the corresponding iterator entry...
	//
	AllocatedImages_t::iterator itImage = AllocatedImages.find(pImage->imgName);
	if (itImage != AllocatedImages.end())
	{		
		R_Images_DeleteImageContents(pImage);
		AllocatedImages.erase(itImage);
	}
	else
	{
		assert(0);
	}
}

// called only at app startup, vid_restart, app-exit
//
void R_Images_Clear(void)
{		
	image_t *pImage;
	//	int iNumImages = 
	   				  R_Images_StartIteration();
	while ( (pImage = R_Images_GetNextIteration()) != NULL)
	{
		R_Images_DeleteImageContents(pImage);
	}

	AllocatedImages.clear();

	giTextureBindNum = 1024;
}


void RE_RegisterImages_Info_f( void )
{
	image_t *pImage	= NULL;
	int iImage		= 0;
	int iTexels		= 0;

	int iNumImages	= R_Images_StartIteration();
	while ( (pImage	= R_Images_GetNextIteration()) != NULL)
	{
		ri.Printf( PRINT_ALL, "%d: (%4dx%4dy) \"%s\"",iImage, pImage->uploadWidth, pImage->uploadHeight, pImage->imgName);
		ri.Printf( PRINT_DEVELOPER, ", levused %d",pImage->iLastLevelUsedOn);
		ri.Printf( PRINT_ALL, "\n");

		iTexels += pImage->uploadWidth * pImage->uploadHeight;
		iImage++;
	}
	ri.Printf( PRINT_ALL, "%d Images. %d (%.2fMB) texels total, (not including mipmaps)\n",iNumImages, iTexels, (float)iTexels / 1024.0f / 1024.0f);
	ri.Printf( PRINT_DEVELOPER, "RE_RegisterMedia_GetLevel(): %d",RE_RegisterMedia_GetLevel());
}


// implement this if you need to, do a find for the caller. I don't need it though, so far.
//
//void		RE_RegisterImages_LevelLoadBegin(const char *psMapName);


// currently, this just goes through all the images and dumps any not referenced on this level...
//
qboolean RE_RegisterImages_LevelLoadEnd(void)
{
	ri.Printf( PRINT_DEVELOPER, "RE_RegisterImages_LevelLoadEnd():\n");

//	int iNumImages = AllocatedImages.size();	// more for curiosity, really.

	qboolean bEraseOccured = qfalse;
	for (AllocatedImages_t::iterator itImage = AllocatedImages.begin(); itImage != AllocatedImages.end(); bEraseOccured?itImage:++itImage)
	{			
		bEraseOccured = qfalse;

		image_t *pImage = (*itImage).second;

		// don't un-register system shaders (*fog, *dlight, *white, *default), but DO de-register lightmaps ("*<mapname>/lightmap%d")
		if (pImage->imgName[0] != '*' || strchr(pImage->imgName,'/'))
		{
			// image used on this level?
			//
			if ( pImage->iLastLevelUsedOn != RE_RegisterMedia_GetLevel() )
			{
				// nope, so dump it...
				//
				ri.Printf( PRINT_DEVELOPER, "Dumping image \"%s\"\n",pImage->imgName);

				R_Images_DeleteImageContents(pImage);
				AllocatedImages.erase(itImage++);
				bEraseOccured = qtrue;
			}
		}
	}


	// this check can be deleted AFAIC, it seems to be just a quake thing...
	//
//	iNumImages = R_Images_StartIteration();
//	if (iNumImages > MAX_DRAWIMAGES)
//	{
//		ri.Printf( PRINT_WARNING, "Level uses %d images, old limit was MAX_DRAWIMAGES (%d)\n", iNumImages, MAX_DRAWIMAGES);
//	}

	ri.Printf( PRINT_DEVELOPER, "RE_RegisterImages_LevelLoadEnd(): Ok\n");	

	GL_ResetBinds();

	return bEraseOccured;
}



// returns image_t struct if we already have this, else NULL. No disk-open performed 
//	(important for creating default images).
//
// This is called by both R_FindImageFile and anything that creates default images...
//
static image_t *R_FindImageFile_NoLoad(const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode )
{	
	if (!name) {
		return NULL;
	}

	char *pName = GenerateImageMappingName(name);

	//
	// see if the image is already loaded
	//
	AllocatedImages_t::iterator itAllocatedImage = AllocatedImages.find(pName);
	if (itAllocatedImage != AllocatedImages.end())
	{	
		image_t *pImage = (*itAllocatedImage).second;

		// the white image can be used with any set of parms, but other mismatches are errors...
		//
		if ( strcmp( pName, "*white" ) ) {
			if ( pImage->mipmap != mipmap ) {
				ri.Printf( PRINT_WARNING, "WARNING: reused image %s with mixed mipmap parm\n", pName );
			}
			if ( pImage->allowPicmip != allowPicmip ) {
				ri.Printf( PRINT_WARNING, "WARNING: reused image %s with mixed allowPicmip parm\n", pName );
			}
			if ( pImage->wrapClampMode != glWrapClampMode ) {
				ri.Printf( PRINT_WARNING, "WARNING: reused image %s with mixed glWrapClampMode parm\n", pName );
			}
		}
			  
		pImage->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

		return pImage;
	}

	return NULL;
}



/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImage( const char *name, const textureImage_t *picWrap, int width, int height, 
					   qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode ) {
	image_t		*image;
	qboolean	isLightmap = qfalse;

	if (strlen(name) >= MAX_QPATH ) {
		ri.Error (ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}

	if(glConfig.clampToEdgeAvailable && glWrapClampMode == GL_CLAMP) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	if (name[0] == '*')
	{
		const char *psLightMapNameSearchPos = strrchr(name,'/');
		if (  psLightMapNameSearchPos && !strncmp( psLightMapNameSearchPos+1, "lightmap", 8 ) ) {
			isLightmap = qtrue;
		}
	}

	if ( (width&(width-1)) || (height&(height-1)) )
	{
		ri.Error( ERR_FATAL, "R_CreateImage: %s dimensions (%i x %i) not power of 2!\n",name,width,height);
	}

	image = R_FindImageFile_NoLoad(name, mipmap, allowPicmip, allowTC, glWrapClampMode );
	if (image) {
		return image;
	}

	image = (image_t*) ri.Malloc( sizeof( image_t ), TAG_IMAGE_T, qtrue );
//	memset(image,0,sizeof(*image));	// qtrue above does this 
	
	image->texnum = 1024 + giTextureBindNum++;	// ++ is of course staggeringly important...

	// record which map it was used on...
	//
	image->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;

	Q_strncpyz(image->imgName, name, sizeof(image->imgName));

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	// lightmaps are always allocated on TMU 1
	if ( qglActiveTextureARB && isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	if ( qglActiveTextureARB ) {
		GL_SelectTexture( image->TMU );
	}

	GL_Bind(image);

	// Call templatized mipmap.
	switch (picWrap->bpc) {
	case BPC_32FLOAT:
		Upload32((float*)picWrap->ptr, image->width, image->height,
			image->mipmap,
			allowPicmip,
			isLightmap,
			allowTC,
			&image->internalFormat,
			&image->uploadWidth,
			&image->uploadHeight,picWrap->bpc);
		break;
	case BPC_32BIT:
		Upload32((unsigned int*)picWrap->ptr, image->width, image->height,
			image->mipmap,
			allowPicmip,
			isLightmap,
			allowTC,
			&image->internalFormat,
			&image->uploadWidth,
			&image->uploadHeight, picWrap->bpc);
		break;
	case BPC_16BIT:
		Upload32((unsigned short*)picWrap->ptr, image->width, image->height,
			image->mipmap,
			allowPicmip,
			isLightmap,
			allowTC,
			&image->internalFormat,
			&image->uploadWidth,
			&image->uploadHeight, picWrap->bpc);
		break;
	case BPC_8BIT:
	default:
		Upload32((byte*)picWrap->ptr, image->width, image->height,
			image->mipmap,
			allowPicmip,
			isLightmap,
			allowTC,
			&image->internalFormat,
			&image->uploadWidth,
			&image->uploadHeight, picWrap->bpc); // This is the classical approach.
		break;

	}

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( GL_TEXTURE_2D, 0 );	//jfm: i don't know why this is here, but it breaks lightmaps when there's only 1
	glState.currenttextures[glState.currenttmu] = 0;	//mark it not bound

	if ( image->TMU == 1 ) {
		GL_SelectTexture( 0 );
	}

	LPCSTR psNewName = GenerateImageMappingName(name);
	Q_strncpyz(image->imgName, psNewName, sizeof(image->imgName));
	AllocatedImages[ image->imgName ] = image;

	return image;
}
#endif // !DEDICATED
/*
=========================================================

TARGA LOADING

=========================================================
*/
/*
Ghoul2 Insert Start
*/

bool LoadTGAPalletteImage ( const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader	targa_header;
	byte	*dataStart;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile ( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return false;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	targa_header.colormap_index = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=1 )
	{
		ri.Error (ERR_DROP, "LoadTGAPalletteImage: Only type 1 (uncompressed pallettised) TGA images supported\n");
	}

	if ( targa_header.colormap_type == 0 )
	{
		ri.Error( ERR_DROP, "LoadTGAPalletteImage: colormaps ONLY supported\n" );
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	*pic = (unsigned char *) ri.Malloc (numPixels, TAG_TEMP_WORKSPACE, qfalse );
	if (targa_header.id_length != 0)
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}
	dataStart = buf_p + (targa_header.colormap_length * (targa_header.colormap_size / 4));
	memcpy(*pic, dataStart, numPixels);
	ri.FS_FreeFile (buffer);

	return true;
}

#ifndef DEDICATED
/*
Ghoul2 Insert End
*/
// My TGA loader...
//
//---------------------------------------------------
#pragma pack(push,1)
typedef struct
{
	byte	byIDFieldLength;	// must be 0
	byte	byColourmapType;	// 0 = truecolour, 1 = paletted, else bad
	byte	byImageType;		// 1 = colour mapped (palette), uncompressed, 2 = truecolour, uncompressed, else bad
	word	w1stColourMapEntry;	// must be 0
	word	wColourMapLength;	// 256 for 8-bit palettes, else 0 for true-colour
	byte	byColourMapEntrySize; // 24 for 8-bit palettes, else 0 for true-colour
	word	wImageXOrigin;		// ignored
	word	wImageYOrigin;		// ignored
	word	wImageWidth;		// in pixels
	word	wImageHeight;		// in pixels
	byte	byImagePlanes;		// bits per pixel	(8 for paletted, else 24 for true-colour)
	byte	byScanLineOrder;	// Image descriptor bytes
								// bits 0-3 = # attr bits (alpha chan)
								// bits 4-5 = pixel order/dir
								// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
} TGAHeader_t;
#pragma pack(pop)


// *pic == pic, else NULL for failed.
//
//  returns false if found but had a format error, else true for either OK or not-found (there's a reason for this)
//

void LoadTGA ( const char *name, byte **pic, int *width, int *height)
{
	char sErrorString[1024];
	bool bFormatErrors = false;

	// these don't need to be declared or initialised until later, but the compiler whines that 'goto' skips them.
	//
	byte *pRGBA = NULL;	
	byte *pOut	= NULL;
	byte *pIn	= NULL;


	*pic = NULL;

#define TGA_FORMAT_ERROR(blah) {sprintf(sErrorString,blah); bFormatErrors = true; goto TGADone;}
//#define TGA_FORMAT_ERROR(blah) ri.Error( ERR_DROP, blah );

	//
	// load the file
	//
	byte *pTempLoadedBuffer = 0;
	ri.FS_ReadFile ( ( char * ) name, (void **)&pTempLoadedBuffer);
	if (!pTempLoadedBuffer) {
		return;
	}

	TGAHeader_t *pHeader = (TGAHeader_t *) pTempLoadedBuffer;

	if (pHeader->byColourmapType!=0)
	{	
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );		
	}

	if (pHeader->byImageType != 2 && pHeader->byImageType != 3 && pHeader->byImageType != 10)
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RLE-RGB) images supported\n");		
	}
		
	if (pHeader->w1stColourMapEntry != 0)
	{
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );		
	}

	if (pHeader->wColourMapLength !=0 && pHeader->wColourMapLength != 256)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapLength must be either 0 or 256\n" );
	}

	if (pHeader->byColourMapEntrySize != 0 && pHeader->byColourMapEntrySize != 24)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapEntrySize must be either 0 or 24\n" );
	}

	if ( ( pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32) && (pHeader->byImagePlanes != 8 && pHeader->byImageType != 3))
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ((pHeader->byScanLineOrder&0x30)!=0x00 &&
		(pHeader->byScanLineOrder&0x30)!=0x10 &&
		(pHeader->byScanLineOrder&0x30)!=0x20 &&
		(pHeader->byScanLineOrder&0x30)!=0x30
		)
	{
		TGA_FORMAT_ERROR("LoadTGA: ScanLineOrder must be either 0x00,0x10,0x20, or 0x30\n");		
	}



	// these last checks are so i can use ID's RLE-code. I don't dare fiddle with it or it'll probably break...
	//
	if ( pHeader->byImageType == 10)
	{
		if ((pHeader->byScanLineOrder & 0x30) != 0x00)
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be in bottom-to-top format\n");
		}
		if (pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32)	// probably won't happen, but avoids compressed greyscales?
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be 24 or 32 bit\n");
		}
	}

	// now read the actual bitmap in...
	//
	// Image descriptor bytes
	// bits 0-3 = # attr bits (alpha chan)
	// bits 4-5 = pixel order/dir
	// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
	//
	int iYStart,iXStart,iYStep,iXStep;

	switch(pHeader->byScanLineOrder & 0x30)		
	{
		default:	// default case stops the compiler complaining about using uninitialised vars
		case 0x00:					//	left to right, bottom to top

			iXStart = 0;
			iXStep  = 1;

			iYStart = pHeader->wImageHeight-1;
			iYStep  = -1;

			break;

		case 0x10:					//  right to left, bottom to top

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = pHeader->wImageHeight-1;
			iYStep	= -1;

			break;

		case 0x20:					//  left to right, top to bottom

			iXStart = 0;
			iXStep  = 1;

			iYStart = 0;
			iYStep  = 1;

			break;

		case 0x30:					//  right to left, top to bottom

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = 0;
			iYStep  = 1;

			break;
	}

	// feed back the results...
	//
	if (width)
		*width = pHeader->wImageWidth;
	if (height)
		*height = pHeader->wImageHeight;

	pRGBA	= (byte *) ri.Malloc (pHeader->wImageWidth * pHeader->wImageHeight * 4, TAG_TEMP_WORKSPACE, qfalse);
	*pic	= pRGBA;
	pOut	= pRGBA;
	pIn		= pTempLoadedBuffer + sizeof(*pHeader);

	// I don't know if this ID-thing here is right, since comments that I've seen are at the end of the file, 
	//	with a zero in this field. However, may as well...
	//
	if (pHeader->byIDFieldLength != 0)
		pIn += pHeader->byIDFieldLength;	// skip TARGA image comment

	byte red,green,blue,alpha;

	if ( pHeader->byImageType == 2 || pHeader->byImageType == 3 )	// RGB or greyscale
	{
		for (int y=iYStart, iYCount=0; iYCount<pHeader->wImageHeight; y+=iYStep, iYCount++)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;			
			for (int x=iXStart, iXCount=0; iXCount<pHeader->wImageWidth; x+=iXStep, iXCount++)
			{
				switch (pHeader->byImagePlanes)
				{
					case 8:
						blue	= *pIn++;
						green	= blue;
						red		= blue;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 24:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 32:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						alpha	= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = alpha;
						break;
					
					default:
						assert(0);	// if we ever hit this, someone deleted a header check higher up
						TGA_FORMAT_ERROR("LoadTGA: Image can only have 8, 24 or 32 planes for RGB/greyscale\n");						
						break;
				}
			}		
		}
	}
	else 
	if (pHeader->byImageType == 10)   // RLE-RGB
	{
		// I've no idea if this stuff works, I normally reject RLE targas, but this is from ID's code
		//	so maybe I should try and support it...
		//
		byte packetHeader, packetSize, j;

		for (int y = pHeader->wImageHeight-1; y >= 0; y--)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;
			for (int x=0; x<pHeader->wImageWidth;)
			{
				packetHeader = *pIn++;
				packetSize   = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)         // run-length packet
				{
					switch (pHeader->byImagePlanes) 
					{
						case 24:

							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= 255;
							break;

						case 32:
							
							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= *pIn++;
							break;

						default:
							assert(0);	// if we ever hit this, someone deleted a header check higher up
							TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
							break;
					}
	
					for (j=0; j<packetSize; j++) 
					{
						*pOut++	= red;
						*pOut++	= green;
						*pOut++	= blue;
						*pOut++	= alpha;
						x++;
						if (x == pHeader->wImageWidth)  // run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
				else 
				{	// non run-length packet

					for (j=0; j<packetSize; j++) 
					{
						switch (pHeader->byImagePlanes) 
						{
							case 24:

								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = 255;
								break;

							case 32:
								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								alpha	= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = alpha;
								break;

							default:
								assert(0);	// if we ever hit this, someone deleted a header check higher up
								TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
								break;
						}
						x++;
						if (x == pHeader->wImageWidth)  // pixel packet run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
			}
			breakOut:;
		}
	}

TGADone:

	ri.FS_FreeFile (pTempLoadedBuffer);

	if (bFormatErrors)
	{
		ri.Error( ERR_DROP, "%s( File: \"%s\" )\n",sErrorString,name);
	}
}


static void LoadRadiance( const char *filename, unsigned char **pic, int *width, int *height ) {

	/* More stuff */
	unsigned char* out;
	byte* fbuffer;

	fileHandle_t		h;
	const int len = FS_FOpenFileRead(filename, &h, qfalse);
	if (!h)
	{
		return;
	}

	fbuffer = (byte*)Z_Malloc(len + 4096, TAG_TEMP_WORKSPACE);
	FS_Read(fbuffer, len, h);
	FS_FCloseFile(h);

	int channelCount;

	float* data = stbi_loadf_from_memory(fbuffer, len, width, height, &channelCount, 4);

	if (!data) {
		Com_Printf("Radiance picture failed to load, possible reason: %s.\n", stbi_failure_reason());
		*pic = nullptr;
		return;
	}
	/*
	if (channelCount != 4) {
		Com_Printf("Radiance returned %d channels, I asked for 4!\n", channelCount); 
		stbi_image_free(data);
		*pic = nullptr;
		return;
	}*/

	//Com_Printf("Loaded Radiance image with %d x %d pixels!\n", *width,*height);

	int memoryAmount = (*width) * (*height) * 4 * 4;
	out = (unsigned char*)ri.Malloc(memoryAmount, TAG_TEMP_WORKSPACE, qfalse);

	*pic = out;
	Com_Memcpy(out, data, memoryAmount);

	stbi_image_free(data);

	Z_Free(fbuffer);
}


static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height ) {
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  unsigned char *out;
  byte	*fbuffer;
  byte  *bbuf;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  fileHandle_t		h;
  const int len = FS_FOpenFileRead(filename, &h, qfalse);
  if (!h)
  {
	  return;
  }

  fbuffer = (byte *)Z_Malloc(len + 4096, TAG_TEMP_WORKSPACE);
  FS_Read(fbuffer, len, h);
  FS_FCloseFile(h);

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  //jpeg_stdio_src(&cinfo, fbuffer);
  jpeg_mem_src(&cinfo, fbuffer, len + 4096);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;

  // rww - 9-13-01 [1-26-01-sof2]
  if (cinfo.output_components != 4 && cinfo.output_components != 1) {
	  Com_Printf("JPG %s is unsupported color depth (%d)\n",filename,cinfo.output_components);
  }

  out = (unsigned char *)ri.Malloc(cinfo.output_width*cinfo.output_height*4, TAG_TEMP_WORKSPACE, qfalse );

  *pic = out;
  *width = cinfo.output_width;
  *height = cinfo.output_height;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
	bbuf = ((out+(row_stride*cinfo.output_scanline)));
	buffer = &bbuf;
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
  }

	if (cinfo.output_components == 1)
	{
		byte *pbDest = (*pic + (cinfo.output_width * cinfo.output_height * 4))-1;
		byte *pbSrc  = (*pic + (cinfo.output_width * cinfo.output_height    ))-1;
		int  iPixels = cinfo.output_width * cinfo.output_height;
		
		for (int i=0; i<iPixels; i++)
		{
			byte b = *pbSrc--;
			*pbDest-- = 255;
			*pbDest-- = b;
			*pbDest-- = b;
			*pbDest-- = b;
		}
	}
	else	  
	// clear all the alphas to 255
	{
		int		i, j;
		byte	*buf;

		buf = *pic;

		j = cinfo.output_width * cinfo.output_height * 4;
		for ( i = 3 ; i < j ; i+=4 ) 
		{
			buf[i] = 255;
		}
	}

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
	Z_Free(fbuffer);
  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
}


/* Expanded data destination object for stdio output */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  byte* outfile;		/* target stream */
  int	size;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

void init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  dest->pub.next_output_byte = dest->outfile;
  dest->pub.free_in_buffer = dest->size;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

boolean empty_output_buffer (j_compress_ptr cinfo)
{
  return TRUE;
}


/*
 * Compression initialization.
 * Before calling this, all parameters and a data destination must be set up.
 *
 * We require a write_all_tables parameter as a failsafe check when writing
 * multiple datastreams from the same compression object.  Since prior runs
 * will have left all the tables marked sent_table=TRUE, a subsequent run
 * would emit an abbreviated stream (no tables) by default.  This may be what
 * is wanted, but for safety's sake it should not be the default behavior:
 * programmers should have to make a deliberate choice to emit abbreviated
 * images.  Therefore the documentation and examples should encourage people
 * to pass write_all_tables=TRUE; then it will take active thought to do the
 * wrong thing.
 */
//
//GLOBAL(void)
//jpeg_start_compress (j_compress_ptr cinfo, boolean write_all_tables)
//{
//  if (cinfo->global_state != CSTATE_START)
//    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
//
//  if (write_all_tables)
//    jpeg_suppress_tables(cinfo, FALSE);	/* mark all tables to be written */
//
//  /* (Re)initialize error mgr and destination modules */
//  (*cinfo->err->reset_error_mgr) ((j_common_ptr) cinfo);
//  (*cinfo->dest->init_destination) (cinfo);
//  /* Perform master selection of active modules */
//  jinit_compress_master(cinfo);
//  /* Set up for the first pass */
//  (*cinfo->master->prepare_for_pass) (cinfo);
//  /* Ready for application to drive first pass through jpeg_write_scanlines
//   * or jpeg_write_raw_data.
//   */
//  cinfo->next_scanline = 0;
//  cinfo->global_state = (cinfo->raw_data_in ? CSTATE_RAW_OK : CSTATE_SCANNING);
//}


/*
 * Write some scanlines of data to the JPEG compressor.
 *
 * The return value will be the number of lines actually written.
 * This should be less than the supplied num_lines only in case that
 * the data destination module has requested suspension of the compressor,
 * or if more than image_height scanlines are passed in.
 *
 * Note: we warn about excess calls to jpeg_write_scanlines() since
 * this likely signals an application programmer error.  However,
 * excess scanlines passed in the last valid call are *silently* ignored,
 * so that the application need not adjust num_lines for end-of-image
 * when using a multiple-scanline buffer.
 */

//GLOBAL(JDIMENSION)
//jpeg_write_scanlines (j_compress_ptr cinfo, JSAMPARRAY scanlines,
//		      JDIMENSION num_lines)
//{
//  JDIMENSION row_ctr, rows_left;
//
//  if (cinfo->global_state != CSTATE_SCANNING)
//    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
//  if (cinfo->next_scanline >= cinfo->image_height)
//    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);
//
//  /* Call progress monitor hook if present */
//  if (cinfo->progress != NULL) {
//    cinfo->progress->pass_counter = (long) cinfo->next_scanline;
//    cinfo->progress->pass_limit = (long) cinfo->image_height;
//    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
//  }
//
//  /* Give master control module another chance if this is first call to
//   * jpeg_write_scanlines.  This lets output of the frame/scan headers be
//   * delayed so that application can write COM, etc, markers between
//   * jpeg_start_compress and jpeg_write_scanlines.
//   */
//  if (cinfo->master->call_pass_startup)
//    (*cinfo->master->pass_startup) (cinfo);
//
//  /* Ignore any extra scanlines at bottom of image. */
//  rows_left = cinfo->image_height - cinfo->next_scanline;
//  if (num_lines > rows_left)
//    num_lines = rows_left;
//
//  row_ctr = 0;
//  (*cinfo->main->process_data) (cinfo, scanlines, &row_ctr, num_lines);
//  cinfo->next_scanline += row_ctr;
//  return row_ctr;
//}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static int hackSize;

void term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = dest->size - dest->pub.free_in_buffer;
  hackSize = datacount;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

void jpegDest (j_compress_ptr cinfo, byte* outfile, int size)
{
  my_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = outfile;
  dest->size = size;
}


//void SaveJPG(char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer) {
int SaveJPG( int quality, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size ) {
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
//  unsigned char *out;

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  //out = (unsigned char *)ri.Hunk_AllocateTempMemory(image_width*image_height*4);
//  out_buffer = (unsigned char *)ri.Hunk_AllocateTempMemory(image_width*image_height*4);
  jpegDest(&cinfo, out_buffer, out_size);//image_width*image_height*4);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; 	/* image width and height, in pixels */
  cinfo.image_height = image_height;
//  cinfo.input_components = 4;		/* # of color components per pixel */
//  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  if ( image_type == mmeShotTypeRGB)	{
	  cinfo.input_components = 3;		/* # of color components per pixel */
	  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	  row_stride = image_width * 3;
  } else {
	  cinfo.input_components = 1;
	  cinfo.in_color_space = JCS_GRAYSCALE;
	  row_stride = image_width;
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  if (!mme_jpegDownsampleChroma->integer) {
	  int i;
	  for (i = 0; i < MAX_COMPONENTS; i++) {
		  cinfo.comp_info[i].h_samp_factor = 1;
		  cinfo.comp_info[i].v_samp_factor = 1;
	  }
  } else {
	  int i;
	  for (i = 0; i < MAX_COMPONENTS; i++) {
		  if (cinfo.comp_info[i].component_id == 1) {
			  cinfo.comp_info[i].h_samp_factor = 2;
			  cinfo.comp_info[i].v_samp_factor = 2;
			  break;
		  }
	  }
  }

  if (mme_jpegOptimizeHuffman->integer) cinfo.optimize_coding = 1;

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
//  row_stride = image_width * 4;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & image_buffer[((cinfo.image_height-1)*row_stride)-cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
//  ri.FS_WriteFile( filename, out, hackSize );

//  ri.Hunk_FreeTempMemory(out);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  return hackSize;
  /* And we're done! */
}

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *shortname, textureImage_t* picWrap, int *width, int *height ) {
	int		bytedepth;
	char	name[MAX_QPATH];

	picWrap->ptr = NULL;
	*width = 0;
	*height = 0;


	COM_StripExtension(shortname, name);
	COM_DefaultExtension(name, sizeof(name), ".hdr");
	LoadRadiance(name, &picWrap->ptr, width, height);            // try radiance first
	if (picWrap->ptr) {
		picWrap->bpc = BPC_32FLOAT;
		return;
	}

	COM_StripExtension(shortname,name);
	COM_DefaultExtension(name, sizeof(name), ".jpg");
	LoadJPG( name, &picWrap->ptr, width, height );
	if (picWrap->ptr) {
		picWrap->bpc = BPC_8BIT;
		return;
	}

	COM_StripExtension(shortname,name);
	COM_DefaultExtension(name, sizeof(name), ".png");	
	LoadPNG32( name, &picWrap->ptr, width, height, &bytedepth ); 			// try png first
	if (picWrap->ptr){
		picWrap->bpc = BPC_8BIT;
		return;
	}

	COM_StripExtension(shortname,name);
	COM_DefaultExtension(name, sizeof(name), ".tga");
	LoadTGA( name, &picWrap->ptr, width, height );            // try tga first
	if (picWrap->ptr){
		picWrap->bpc = BPC_8BIT;
		return;
	}

}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode ) {
	image_t	*image;
	int		width, height;
	//byte	*pic;
	textureImage_t	picWrap;

	if (!name 
		|| com_dedicated->integer	// stop ghoul2 horribleness as regards image loading from server
		) 
	{
		return NULL;
	}

	// need to do this here as well as in R_CreateImage, or R_FindImageFile_NoLoad() may complain about
	//	different clamp parms used...
	//
	if(glConfig.clampToEdgeAvailable && glWrapClampMode == GL_CLAMP) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	image = R_FindImageFile_NoLoad(name, mipmap, allowPicmip, allowTC, glWrapClampMode );
	if (image) {
		return image;
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &picWrap, &width, &height );
	if ( picWrap.ptr == NULL ) {                                    // if we dont get a successful load
      return NULL;                                        // bail
	}


	// refuse to find any files not power of 2 dims...
	//
	if ( (width&(width-1)) || (height&(height-1)) )
	{
		ri.Printf( PRINT_ALL, "Refusing to load non-power-2-dims(%d,%d) pic \"%s\"...\n", width,height,name );
		return NULL;
	}

	image = R_CreateImage( ( char * ) name, &picWrap, width, height, mipmap, allowPicmip, allowTC, glWrapClampMode );
	ri.Free( picWrap.ptr );
	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) {
	int		width, height;
	//byte	*pic;
	textureImage_t picWrap;

	R_LoadImage("gfx/2d/dlight", &picWrap, &width, &height);
	if (picWrap.ptr) {                                    
		tr.dlightImage = R_CreateImage("*dlight", &picWrap, width, height, qfalse, qfalse, qfalse, GL_CLAMP );
		Z_Free(picWrap.ptr);
	} else { // if we dont get a successful load
		int		x,y;
		byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
		int		b;

		// make a centered inverse-square falloff blob for dynamic lighting
		for (x=0 ; x<DLIGHT_SIZE ; x++) {
			for (y=0 ; y<DLIGHT_SIZE ; y++) {
				float	d;

				d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
					( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
				b = 4000 / d;
				if (b > 255) {
					b = 255;
				} else if ( b < 75 ) {
					b = 0;
				}
				data[y][x][0] = 
				data[y][x][1] = 
				data[y][x][2] = b;
				data[y][x][3] = 255;			
			}
		}
		picWrap.ptr = (byte*)data;
		picWrap.bpc = BPC_8BIT;
		tr.dlightImage = R_CreateImage("*dlight", &picWrap, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, qfalse, GL_CLAMP );
	}
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}




/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	//byte	*data;
	textureImage_t picWrap;
	picWrap.bpc = BPC_8BIT;
	float	g;
	float	d;
	float	borderColor[4];

	picWrap.ptr = (unsigned char *)ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			picWrap.ptr[(y*FOG_S+x)*4+0] =
			picWrap.ptr[(y*FOG_S+x)*4+1] = 
			picWrap.ptr[(y*FOG_S+x)*4+2] = 255;
			picWrap.ptr[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", &picWrap, FOG_S, FOG_T, qfalse, qfalse, qfalse, GL_CLAMP );
	ri.Hunk_FreeTempMemory(picWrap.ptr);

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	textureImage_t picWrap;
	picWrap.bpc = BPC_8BIT;
	picWrap.ptr = (byte*)data;

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", &picWrap, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, qfalse, GL_REPEAT );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int		x,y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	textureImage_t picWrap;
	picWrap.bpc = BPC_8BIT;
	picWrap.ptr = (byte*)data;

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", &picWrap, 8, 8, qfalse, qfalse, qfalse, GL_REPEAT );

#ifdef JEDIACADEMY_GLOW
	// Create the scene glow image. - AReis
	tr.screenGlow = 1024 + giTextureBindNum++;
	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_TEXTURE_RECTANGLE_EXT );
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.screenGlow );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Create the scene image. - AReis
	tr.sceneImage = 1024 + giTextureBindNum++;
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.sceneImage );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, glConfig.vidWidth, glConfig.vidHeight, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Create the minimized scene blur image.
	if ( r_DynamicGlowWidth->integer > glConfig.vidWidth  )
	{
		r_DynamicGlowWidth->integer = glConfig.vidWidth;
	}
	if ( r_DynamicGlowHeight->integer > glConfig.vidHeight  )
	{
		r_DynamicGlowHeight->integer = glConfig.vidHeight;
	}
	tr.blurImage = 1024 + giTextureBindNum++;
	qglBindTexture( GL_TEXTURE_RECTANGLE_EXT, tr.blurImage );
	qglTexImage2D( GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA16, r_DynamicGlowWidth->integer, r_DynamicGlowHeight->integer, 0, GL_RGB, GL_FLOAT, 0 );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameteri( GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP );
	qglDisable( GL_TEXTURE_RECTANGLE_EXT );
	qglEnable( GL_TEXTURE_2D );
#endif

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++) {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", &picWrap, 8, 8, qfalse, qfalse, qfalse, GL_REPEAT );


	for(x=0;x<32;x++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage(va("*scratch%d",x), &picWrap, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, qfalse, GL_CLAMP );
	}

	if (r_newDLights->integer) {
		tr.dlightImage = R_FindImageFile("gfx/2d/dlight", qtrue, qfalse, qfalse, GL_CLAMP);
	} else {
		R_CreateDlightImage();
	}
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		i, j;
	float	g;
	int		inf;
	int		shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	/*if ( !glConfig.deviceSupportsGamma && !(r_fbo->integer && r_fboOverbright->integer )) {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}*/ // Whatever, I want overbright bits to always work so the capture is right.

	// Overbright bits multiplier is for float stuff.
	// We can't reproduce it perfectly for obvious reasons, but, we can at least have the peak light end up with 
	// the same intensity change
	tr.overbrightBitsMultiplier = 1.0f / R_sRGBToLinear(1.0f/pow(2, tr.overbrightBits));

#ifndef QSDL
	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) 
	{
		//tr.overbrightBits = 0; // No. We want overbrightbits in every case.
	}
#endif

	if ( tr.overbrightBits > 1 ) {
		tr.overbrightBits = 1;
	}

	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;


	if ( r_intensity->value < 1.0f ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0) {
			inf = 0;
		}
		if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (i=0 ; i<256 ; i++) {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma )
	{
		if (!(r_fbo->integer && r_fboOverbright->integer)) {

			GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
		}
	}
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	//memset(hashTable, 0, sizeof(hashTable));	// DO NOT DO THIS NOW (because of image cacheing)	-ste.
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
// (only gets called during vid_restart now (and app exit), not during map load)
//
void R_DeleteTextures( void ) {

	R_Images_Clear();	
	GL_ResetBinds();
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
===============
RE_SplitSkins
input = skinname, possibly being a macro for three skins
return= true if three part skins found
output= qualified names to three skins if return is true, undefined if false
===============
*/
bool RE_SplitSkins(const char* INname, char* skinhead, char* skintorso, char* skinlower)
{	//INname= "models/players/jedi_tf/|head01_skin1|torso01|lower01";
	if (strchr(INname, '|'))
	{
		char name[MAX_QPATH];
		Q_strncpyz(name, INname, sizeof(name));
		char* p = strchr(name, '|');
		*p = 0;
		p++;
		//fill in the base path
		strcpy(skinhead, name);
		strcpy(skintorso, name);
		strcpy(skinlower, name);

		//now get the the individual files

		//advance to second
		char* p2 = strchr(p, '|');
		assert(p2);
		if (!p2)
		{
			return false;
		}
		*p2 = 0;
		p2++;
		strcat(skinhead, p);
		strcat(skinhead, ".skin");


		//advance to third
		p = strchr(p2, '|');
		assert(p);
		if (!p)
		{
			return false;
		}
		*p = 0;
		p++;
		strcat(skintorso, p2);
		strcat(skintorso, ".skin");

		strcat(skinlower, p);
		strcat(skinlower, ".skin");

		return true;
	}
	return false;
}

// given a name, go get the skin we want and return
qhandle_t RE_RegisterIndividualSkin(const char* name, qhandle_t hSkin)
{
	skin_t* skin;
	skinSurface_t* surf;
	char* text, * text_p;
	char* token;
	char			surfName[MAX_QPATH];

	// load and parse the skin file
	ri.FS_ReadFile(name, (void**)&text);
	if (!text) {
#ifndef FINAL_BUILD
		ri.Printf(PRINT_ALL, "WARNING: RE_RegisterSkin( '%s' ) failed to load!\n", name);
#endif
		return 0;
	}

	assert(tr.skins[hSkin]);	//should already be setup, but might be an 3part append

	skin = tr.skins[hSkin];

	text_p = text;
	while (text_p && *text_p) {
		// get surface name
		token = CommaParse(&text_p);
		Q_strncpyz(surfName, token, sizeof(surfName));

		if (!token[0]) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr(surfName);

		if (*text_p == ',') {
			text_p++;
		}

		if (!strncmp(token, "tag_", 4)) {	//these aren't in there, but just in case you load an id style one...
			continue;
		}

		// parse the shader name
		token = CommaParse(&text_p);

		if ( r_loadSkinsJKA->integer == 2 && !strcmp(&surfName[strlen(surfName) - 4], "_off"))
		{
			if (!strcmp(token, "*off"))
			{
				continue;	//don't need these double offs
			}
			surfName[strlen(surfName) - 4] = 0;	//remove the "_off"
		}
		if (sizeof(skin->surfaces) / sizeof(skin->surfaces[0]) <= (unsigned)skin->numSurfaces)
		{
			assert(sizeof(skin->surfaces) / sizeof(skin->surfaces[0]) > skin->numSurfaces);
			ri.Printf(PRINT_ALL, "WARNING: RE_RegisterSkin( '%s' ) more than %u surfaces!\n", name, (unsigned int)(sizeof(skin->surfaces) / sizeof(*(skin->surfaces))));
			break;
		}

		int reDefinitionOf = 0; // see if that name is already defined
		for (int i = 0; i < skin->numSurfaces; i++) {
			if (!Q_stricmp(surfName, skin->surfaces[i]->name)) {
				reDefinitionOf = i;
			}
		}

		if (reDefinitionOf) {
			surf = skin->surfaces[reDefinitionOf];

			if (!surf->turnedOff) { // If it's already turned off, no use parsing the other stuff
				if (!Q_stricmp(token, "*off")) { // Ugly hack to make JKA skins with turned off surfaces work better :P
					surf->shader = R_FindShader("textures/system/nodraw", lightmapsNone, stylesDefault, qtrue);
					surf->turnedOff = qtrue;
				}
				else {
					// We already have one texture for that surface. User shouldn't have defined more than one.
				}
			}

		}
		else {
			surf = skin->surfaces[skin->numSurfaces] = (skinSurface_t*)Hunk_Alloc(sizeof(*skin->surfaces[0]), h_low);
			Q_strncpyz(surf->name, surfName, sizeof(surf->name));

			/*
			if (gServerSkinHack)
			{
				surf->shader = R_FindServerShader( token, lightmapsNone, stylesDefault, qtrue );
			}
			else
			{
			*/
			if (!Q_stricmp(token, "*off")) { // Ugly hack to make JKA skins with turned off surfaces work better :P
				surf->shader = R_FindShader("textures/system/nodraw", lightmapsNone, stylesDefault, qtrue);
				surf->turnedOff = qtrue;
			}
			else {
				surf->shader = R_FindShader(token, lightmapsNone, stylesDefault, qtrue);
				surf->turnedOff = qfalse;
			}
			/*
			}
			*/
			skin->numSurfaces++;
		}

		
	}

	ri.FS_FreeFile(text);


	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	char		*text, *text_p;
	char		*token;
	char		surfName[MAX_QPATH];
	char		nameBuffer[MAX_QPATH + 2]; // +2, so we can detect cases when we exceed MAX_QPATH

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if (r_loadSkinsJKA->integer) // FIXME: Add a new feature to the cgame API to "control" this and disable this "hack"?
	{ // JK2 compatibility hack - (the jk2 cgame code calls RegisterSkin with "modelpath/model_*.skin", but the SplitSkin function expects "modelpath/*", so we have to remove the "model_" and ".skin" if it's appears to be a splitable skin)
		// Doing this in the SplitSkin function isn't an option, as we limited the buffer to MAX_QPATH and increasing it would require changes in a lot more functions.
		const char* pos = strstr(name, "model_|");
		if (pos && strlen(pos) > 7)
		{
			Q_strncpyz(nameBuffer, name, pos - name + 1);
			Q_strcat(nameBuffer, sizeof(nameBuffer), pos + 6);

			pos = strstr(nameBuffer, ".skin");
			if (pos) nameBuffer[strlen(nameBuffer) - strlen(pos)] = 0;
			name = nameBuffer;
		}
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (struct skin_s *)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	if (r_loadSkinsJKA->integer)
	{ // JKA-Style skin-loading
		char skinhead[MAX_QPATH] = { 0 };
		char skintorso[MAX_QPATH] = { 0 };
		char skinlower[MAX_QPATH] = { 0 };
		if (RE_SplitSkins(name, (char*)&skinhead, (char*)&skintorso, (char*)&skinlower))
		{//three part
			hSkin = RE_RegisterIndividualSkin(skinhead, hSkin);
			if (hSkin)
			{
				hSkin = RE_RegisterIndividualSkin(skintorso, hSkin);
				if (hSkin)
				{
					hSkin = RE_RegisterIndividualSkin(skinlower, hSkin);
				}
			}
		}
		else
		{//single skin
			hSkin = RE_RegisterIndividualSkin(name, hSkin);
		}
	}
	else {
		// Original JK2-Style skin-loading
		// If not a .skin file, load as a single shader
		if (strcmp(name + strlen(name) - 5, ".skin")) {
			skin->numSurfaces = 1;
			skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc(sizeof(skin->surfaces[0]), h_low);
			skin->surfaces[0]->shader = R_FindShader(name, lightmapsNone, stylesDefault, qtrue);
			return hSkin;
		}

		// load and parse the skin file
		ri.FS_ReadFile(name, (void**)&text);
		if (!text) {
			return 0;
		}

		text_p = text;
		while (text_p && *text_p) {
			// get surface name
			token = CommaParse(&text_p);
			Q_strncpyz(surfName, token, sizeof(surfName));

			if (!token[0]) {
				break;
			}
			// lowercase the surface name so skin compares are faster
			Q_strlwr(surfName);

			if (*text_p == ',') {
				text_p++;
			}

			if (strstr(token, "tag_")) {
				continue;
			}

			// parse the shader name
			token = CommaParse(&text_p);

			assert(skin->numSurfaces < MD3_MAX_SURFACES);

			surf = skin->surfaces[skin->numSurfaces] = (skinSurface_t*)ri.Hunk_Alloc(sizeof(*skin->surfaces[0]), h_low);
			Q_strncpyz(surf->name, surfName, sizeof(surf->name));
			surf->shader = R_FindShader(token, lightmapsNone, stylesDefault, qtrue);
			skin->numSurfaces++;
		}

		ri.FS_FreeFile(text);


		// never let a skin have 0 shaders
		if (skin->numSurfaces == 0) {
			return 0;		// use default skin
		}
	}

	return hSkin;
}

#endif // !DEDICATED
/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (struct skin_s *)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (skinSurface_t *)ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

#ifndef DEDICATED
/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

#endif // !DEDICATED


/*
============================================================================

MME additions

============================================================================
*/

static int SaveTGA_RLERGBA(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	const unsigned int *inBuf = ( const unsigned int*)image_buffer;
	int dataSize = 0;
	
	for (y=0; y < image_height;y++) {
		int left = image_width;
		/* Prepare for the first block and write the first pixel */
		while ( left > 0 ) {
			/* Search for a block of similar pixels */
			int i, block = left > 128 ? 128 : left;
			unsigned int pixel = inBuf[0];
			/* Check for rle pixels */
			for ( i = 1;i < block;i++) {
				if ( inBuf[i] != pixel)
					break;
			}
			if ( i > 1  ) {
				out[dataSize++] = 0x80 | ( i - 1);
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
				out[dataSize++] = pixel >> 24;
			} else {
				int blockStart = dataSize++;
				/* Write some raw pixels no matter what*/
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
				out[dataSize++] = pixel >> 24;
				pixel = inBuf[1];
				for ( i = 1;i < block;i++) {
					if ( inBuf[i+1] == pixel)
						break;
					out[dataSize++] = pixel >> 16;
					out[dataSize++] = pixel >> 8;
					out[dataSize++] = pixel >> 0;
					out[dataSize++] = pixel >> 24;
					pixel = inBuf[i+1];
				}
				out[blockStart] = i - 1;
			}
			inBuf += i;
			left -= i;
		}
	}
	return dataSize;
}

static int SaveTGA_RLERGB(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	const byte *inBuf = ( const byte*)image_buffer;
	int dataSize = 0;
	
	for (y=0; y < image_height;y++) {
		int left = image_width;
		/* Prepare for the first block and write the first pixel */
		while ( left > 0 ) {
			/* Search for a block of similar pixels */
			int i, block = left > 128 ? 128 : left;
			unsigned int pixel = inBuf[0] | (inBuf[1] << 8) | (inBuf[2] << 16);
			/* Check for rle pixels */
			for ( i = 1;i < block;i++) {
				unsigned int testPixel = inBuf[i*3+0] | (inBuf[i*3+1] << 8) | (inBuf[i*3+2] << 16);
				if ( testPixel != pixel)
					break;
			}
			if ( i > 1  ) {
				out[dataSize++] = 0x80 | ( i - 1);
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
			} else {
				int blockStart = dataSize++;
				/* Write some raw pixels no matter what*/
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
				pixel = inBuf[3] | (inBuf[4] << 8) | (inBuf[5] << 16);
				for ( i = 1;i < block;i++) {
					unsigned int testPixel = inBuf[i*3+3] | (inBuf[i*3+4] << 8) | (inBuf[i*3+5] << 16);
					if ( testPixel == pixel)
						break;
					out[dataSize++] = pixel >> 16;
					out[dataSize++] = pixel >> 8;
					out[dataSize++] = pixel >> 0;
					pixel = testPixel;
				}
				out[blockStart] = i - 1;
			}
			inBuf += i*3;
			left -= i;
		}
	}
	return dataSize;
}

static int SaveTGA_RLEGray(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	unsigned char *inBuf = (unsigned char*)image_buffer;

	int dataSize = 0;

	for (y=0; y < image_height;y++) {
		int left = image_width;
		int diffIndex, diff;
		unsigned char lastPixel, nextPixel;
		lastPixel = *inBuf++;

		diff = 0;
		while (left > 0 ) {
			int c, n;
			if (left >= 2) {
				nextPixel = *inBuf++;
				if (lastPixel == nextPixel) {
					if (diff) {
						out[diffIndex] = diff - 1;
						diff = 0;
					}
					left -= 2;
					c = left > 126 ? 126 : left;
					n = 0;

					while (c) {
						nextPixel = *inBuf++;
						if (lastPixel != nextPixel)
							break;
						c--; n++;
					}
					left -= n;
					out[dataSize++] = 0x80 | (n + 1);
					out[dataSize++] = lastPixel;
					lastPixel = nextPixel;
				} else {
finalDiff:
					left--;
					if (!diff) {
						diff = 1;
						diffIndex = dataSize++;
					} else if (++diff >= 128) {
						out[diffIndex] = diff - 1;
						diff = 0;
					}
					out[dataSize++] = lastPixel;
					lastPixel = nextPixel;
				}
			} else {
				goto finalDiff;
			}
		}
		if (diff) {
			out[diffIndex] = diff - 1;
		}
	}
	return dataSize;
}


/*
===============
SaveTGA
===============
*/
int SaveTGA( int image_compressed, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size ) {
	int i;
	int imagePixels = image_height * image_width;
	int filesize = 18;	// header is here by default
	int bitDepth;
	byte tgaFormat;

	// Fill in the header
	switch (image_type) {
	case mmeShotTypeGray:
		tgaFormat = 3;
		bitDepth = 8;
		break;
	case mmeShotTypeRGB:
		bitDepth = 24;
		tgaFormat = 2;
		break;
	case mmeShotTypeRGBA:
		bitDepth = 32;
		tgaFormat = 2;
		break;
	default:
		return 0;
	}
	if (image_compressed)
		tgaFormat += 8;

	/* Clear the header */
	Com_Memset( out_buffer, 0, filesize );

	out_buffer[2] = tgaFormat;
	out_buffer[12] = image_width & 255;
	out_buffer[13] = image_width >> 8;
	out_buffer[14] = image_height & 255;
	out_buffer[15] = image_height >> 8;
	out_buffer[16] = bitDepth;
	//Alpha/Attribute bits whatever
	out_buffer[17] = bitDepth == 32 ? 8 : 0;

	// Fill output buffer
	if (!image_compressed) { // Plain memcpy
		byte *buftemp = out_buffer+filesize;
		switch (image_type) {
		case mmeShotTypeRGBA:
			for (i = 0; i < imagePixels; i++ ) {
				/* Also handle the RGBA to BGRA conversion here */
				*buftemp++ = image_buffer[2];
				*buftemp++ = image_buffer[1];
				*buftemp++ = image_buffer[0];
				*buftemp++ = image_buffer[3];
				image_buffer += 4;
			}
			filesize += image_width*image_height*4;
			break;
		case mmeShotTypeRGB:
			for (i = 0; i < imagePixels; i++ ) {
				/* Also handle the RGB to BGR conversion here */
				*buftemp++ = image_buffer[2];
				*buftemp++ = image_buffer[1];
				*buftemp++ = image_buffer[0];
				image_buffer += 3;
			}
			filesize += image_width*image_height*3;
			break;
		case mmeShotTypeGray:
			/* Stupid copying of data here but oh well */
			Com_Memcpy( buftemp, image_buffer, image_width*image_height );
			filesize += image_width*image_height;
			break;
		}
	} else {
		switch (image_type) {
		case mmeShotTypeRGB:
			filesize += SaveTGA_RLERGB(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		case mmeShotTypeRGBA:
			filesize += SaveTGA_RLERGBA(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		case mmeShotTypeGray:
			filesize += SaveTGA_RLEGray(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		}
	}
	return filesize;
}

#ifdef SUPPORT_PNG
typedef struct {
	char *buffer;
	unsigned int bufferSize;
	unsigned int bufferUsed;
} PNGWriteData_t;

static void PNG_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	PNGWriteData_t *ioData = (PNGWriteData_t *)png_get_io_ptr( png_ptr );
	if ( ioData->bufferUsed + length < ioData->bufferSize) {
		Com_Memcpy( ioData->buffer + ioData->bufferUsed, data, length );
		ioData->bufferUsed += length;
	}
}

static void PNG_flush_data(png_structp png_ptr) {

}

/* Save PNG */
int SavePNG( int compresslevel, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size ) {
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	png_bytep *row_pointers = 0;
	PNGWriteData_t writeData;
	int i, rowSize;

	writeData.bufferUsed = 0;
	writeData.bufferSize = out_size;
	writeData.buffer = (char *)out_buffer;
	if (!writeData.buffer)
		goto skip_shot;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL, NULL);
	if (!png_ptr)
		goto skip_shot;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto skip_shot;

	/* Finalize the initing of png library */
    png_set_write_fn(png_ptr, &writeData, PNG_write_data, PNG_flush_data );
	if (compresslevel < 0 || compresslevel > Z_BEST_COMPRESSION)
		compresslevel = Z_DEFAULT_COMPRESSION;
	png_set_compression_level(png_ptr, compresslevel );
	
	/* set other zlib parameters */
	png_set_compression_mem_level(png_ptr, 8);
	png_set_compression_strategy(png_ptr,Z_DEFAULT_STRATEGY);
	png_set_compression_window_bits(png_ptr, 15);
	png_set_compression_method(png_ptr, 8);
	png_set_compression_buffer_size(png_ptr, 8192);
	if ( image_type == mmeShotTypeRGB ) {
		rowSize = image_width*3;
		png_set_IHDR(png_ptr, info_ptr, image_width, image_height, 8, 
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr );
	} else if ( image_type == mmeShotTypeRGBA ) {
		rowSize = image_width*4;
		png_set_IHDR(png_ptr, info_ptr, image_width, image_height, 8, 
			PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr );
	} else if ( image_type == mmeShotTypeGray ) {
		rowSize = image_width*1;
		png_set_IHDR(png_ptr, info_ptr, image_width, image_height, 8, 
			PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr );
	}
	/*Allocate an array of scanline pointers*/
	row_pointers=(png_bytep*)malloc(image_height*sizeof(png_bytep));
	for (i=0;i<image_height;i++) {
		row_pointers[i]=(image_buffer+(image_height -1 - i )*rowSize );
	}
	/*tell the png library what to encode.*/
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, 0);

	//PNG_TRANSFORM_PACKSWAP | PNG_TRANSFORM_STRIP_FILLER
skip_shot:
	if (png_ptr)
		png_destroy_write_struct(&png_ptr, &info_ptr);
	if (row_pointers)
		free(row_pointers);
	return writeData.bufferUsed;
}
#else
void SavePNG(const char * filename, const int compresslevel, const int image_width, const int image_height, const byte *image_buffer, const int image_hasalpha) {

}
#endif
