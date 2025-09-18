gfx/damage/bodyburnmark1
{
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	q3map_nolightmap
    {
        clampmap gfx/damage/burnmark1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.8
    }
    {
        clampmap gfx/damage/bodyburnmark_glow
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 0.6 0.1 0 0.2
        tcMod stretch sin 1 0.01 0 0.2
		rgbMult ( 2.0 2.0 2.0 )
    }
}

gfx/damage/bodybigburnmark1
{
	qer_editorimage	gfx/damage/bodyburnmark_glow
	surfaceparm	nomarks
	surfaceparm	trans
	polygonOffset
	q3map_nolightmap
    {
        clampmap gfx/damage/bodyburnmark2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.9
    }
}


gfx/damage/saberburnmark
{
	polygonOffset
    {
        clampmap gfx/damage/saberburnmark
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

gfx/damage/saberglowmark
{
	polygonOffset
    {
        clampmap gfx/damage/saberburnmark2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
    {
        clampmap gfx/damage/saberglowmark
        blendFunc GL_ONE GL_ONE
        rgbGen wave noise 1 0.5 0 2
		rgbMult ( 2.0 2.0 2.0 )
    }
}

gfx/damage/saberglowmark2
{
	polygonOffset
    {
        map textures/common/caps
    }
    {
        map textures/common/caps_glow
        blendFunc GL_ONE GL_ONE
        rgbGen wave noise 1 0.5 0 2
		rgbMult ( 2.0 2.0 2.0 )
    }
}
