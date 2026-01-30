textures/hipshot_m8/ivy_5
{
	qer_editorimage textures/hipshot_m8/ivy_5.tga
	qer_alphafunc greater 0.5
	q3map_shadeangle 90
	surfaceparm alphashadow
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	qer_trans 0.99
   	noPicMip	
	cull none 	
	{
		map textures/hipshot_m8/ivy_5.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc filter 
		depthFunc equal
	}		
}

textures/yavin/temple_vinesalpha234
{
	qer_editorimage textures/yavin/temple_vinesalpha
	qer_alphafunc greater 0.5
	cull none
    {
        map textures/yavin/temple_vinesalpha
        alphaFunc GE128
        depthWrite
    }
    {
        map $lightmap
		rgbGen identity
		blendFunc filter 
		depthFunc equal
    }
}


textures/impdetention/mp_r_symbol_glowy
{
	q3map_backShader textures/impdetention/mp_r_symbol_glow_shape
	q3map_lightimage textures/impdetention/mp_r_symbol_glow
	qer_editorimage textures/impdetention/mp_r_symbol
	surfaceparm trans
    {
        map $lightmap
    }
    {
        map textures/impdetention/mp_r_symbol
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map gfx/effects/chr_white_add_mild
        blendFunc GL_ONE GL_ONE
        tcGen environment
    }
    {
        map textures/impdetention/mp_r_symbol
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
	alphaGen lightingSpecular
    }
    {
        map textures/impdetention/mp_r_symbol_glow
        blendFunc GL_ONE GL_ONE
    }
    {
        map textures/impdetention/mp_r_symbol_glow2
        blendFunc GL_ONE GL_ONE
 	glow
    }
}

textures/impdetention/mp_b_symbol_glowy
{
	q3map_backShader textures/impdetention/mp_b_symbol_glow_shape
	q3map_lightimage textures/impdetention/mp_b_symbol_glow
	qer_editorimage textures/impdetention/mp_b_symbol
	surfaceparm trans
    {
        map $lightmap
    }
    {
        map textures/impdetention/mp_b_symbol
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map gfx/effects/chr_white_add_mild
        blendFunc GL_ONE GL_ONE
        tcGen environment
    }
    {
        map textures/impdetention/mp_b_symbol
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
	alphaGen lightingSpecular
    }
    {
        map textures/impdetention/mp_b_symbol_glow
        blendFunc GL_ONE GL_ONE
    }
    {
        map textures/impdetention/mp_b_symbol_glow2
        blendFunc GL_ONE GL_ONE
 	glow
    }
}



textures/impdetention/mp_r_symbol_glowy_shine
{
	q3map_lightimage textures/impdetention/mp_r_symbol_glow
	qer_editorimage textures/impdetention/mp_r_symbol
	q3map_surfacelight	2000
	q3map_backSplash 0 100
    {
        map $lightmap
    }
    {
        map textures/impdetention/mp_r_symbol
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map gfx/effects/chr_white_add_mild
        blendFunc GL_ONE GL_ONE
        tcGen environment
    }
    {
        map textures/impdetention/mp_r_symbol
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
	alphaGen lightingSpecular
    }
    {
        map textures/impdetention/mp_r_symbol_glow
        blendFunc GL_ONE GL_ONE
    }
    {
        map textures/impdetention/mp_r_symbol_glow2
        blendFunc GL_ONE GL_ONE
 	glow
    }
}

textures/impdetention/mp_b_symbol_glowy_shine
{
	q3map_lightimage textures/impdetention/mp_b_symbol_glow
	qer_editorimage textures/impdetention/mp_b_symbol
	q3map_surfacelight	2000
	q3map_backSplash 0 100
    {
        map $lightmap
    }
    {
        map textures/impdetention/mp_b_symbol
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map gfx/effects/chr_white_add_mild
        blendFunc GL_ONE GL_ONE
        tcGen environment
    }
    {
        map textures/impdetention/mp_b_symbol
        blendFunc GL_SRC_ALPHA GL_ONE
        detail
	alphaGen lightingSpecular
    }
    {
        map textures/impdetention/mp_b_symbol_glow
        blendFunc GL_ONE GL_ONE
    }
    {
        map textures/impdetention/mp_b_symbol_glow2
        blendFunc GL_ONE GL_ONE
 	glow
    }
}


