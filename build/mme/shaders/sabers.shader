gfx/effects/sabers/red_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/red_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/red_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/red_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/orange_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/orange_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/orange_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/orange_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/yellow_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/yellow_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/yellow_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/yellow_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/green_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/green_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/green_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/green_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/blue_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/blue_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/blue_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/blue_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/purple_glow
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/purple_glow2.tga
        blendFunc GL_ONE GL_ONE
		glow
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/purple_line
{
	entityMergable
	cull	disable
    {
        map gfx/effects/sabers/purple_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 12.0 12.0 12.0 )
    }
}

gfx/effects/sabers/saberBlur
{
    cull disable

    {
        clampmap gfx/effects/sabers/blurglow
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
		rgbMult ( 8.0 8.0 8.0 )
    }
    {
        clampmap gfx/effects/sabers/blurcore
        blendFunc GL_ONE GL_ONE
        rgbGen identity
		rgbMult ( 8.0 8.0 8.0 )
    }
}

// used for trip mine

gfx/misc/whiteLine2
{
	cull	twosided
    {
        map gfx/misc/whiteline2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}