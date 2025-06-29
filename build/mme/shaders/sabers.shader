gfx/effects/sabers/red_glow
{
	cull	disable
    {
        map gfx/effects/sabers/red_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/red_line
{
	cull	disable
    {
        map gfx/effects/sabers/red_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/orange_glow
{
	cull	disable
    {
        map gfx/effects/sabers/orange_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/orange_line
{
	cull	disable
    {
        map gfx/effects/sabers/orange_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/yellow_glow
{
	cull	disable
    {
        map gfx/effects/sabers/yellow_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/yellow_line
{
	cull	disable
    {
        map gfx/effects/sabers/yellow_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/green_glow
{
	cull	disable
    {
        map gfx/effects/sabers/green_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/green_line
{
	cull	disable
    {
        map gfx/effects/sabers/green_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/blue_glow
{
	cull	disable
    {
        map gfx/effects/sabers/blue_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/blue_line
{
	cull	disable
    {
        map gfx/effects/sabers/blue_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sabers/purple_glow
{
	cull	disable
    {
        map gfx/effects/sabers/purple_glow2.tga
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/sabers/purple_line
{
	cull	disable
    {
        map gfx/effects/sabers/purple_line.tga
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
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
    }
    {
        clampmap gfx/effects/sabers/blurcore
        blendFunc GL_ONE GL_ONE
        rgbGen identity
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