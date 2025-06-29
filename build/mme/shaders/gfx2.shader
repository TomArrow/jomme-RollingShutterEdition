clear
{
    {
        map gfx/effects/clear
        blendFunc GL_ONE GL_ONE
    }
}

gfx/effects/stunPass
{
	cull	disable
    {
        map gfx/effects/stunpass
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod scroll -3.5 0
        tcMod scale 1.5 1
    }
}

gfx/effects/solidWhite
{
	nomipmaps
	cull	disable
    {
        map gfx/effects/solidwhite
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/bubble
{
    {
        map gfx/effects/bubble
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/effects/solidWhite_cull
{
	nomipmaps
    {
        map gfx/effects/solidwhite
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sparks1
{
	cull	disable
    {
        map gfx/effects/spark1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sparks2
{
	cull	disable
    {
        map gfx/effects/spark2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/sparks3
{
	cull	disable
    {
        map gfx/effects/spark3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/lightning
{
	cull	disable
    {
        map gfx/effects/lightning
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/whiteFlare
{
	cull	disable
    {
        map gfx/effects/whiteflare
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/whiteFlareMult
{
	cull	disable
    {
        map gfx/effects/whiteflare
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen vertex
    }
}

gfx/effects/glob
{
	cull	disable
    {
        map gfx/effects/glob
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod rotate 3
    }
    {
        map gfx/effects/glob
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod rotate -5
    }
}

gfx/effects/jet3
{
	cull	disable
    {
        map gfx/effects/jet3
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        tcMod scroll 1 0
    }
}

gfx/effects/jet4
{
	cull	disable
    {
        map gfx/effects/jet4
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        tcMod scroll 1 0
    }
}

gfx/effects/flare1
{
	cull	disable
    {
        map gfx/effects/flare1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod rotate 4
    }
}

gfx/effects/meltMark
{
	polygonOffset
	cull	disable
    {
        map gfx/effects/meltmark
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/smokeTrail
{
	cull	disable
    {
        map gfx/effects/smoketrail
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 1 0.7
        tcMod scroll 0 -0.2
    }
    {
        map gfx/effects/smoketrail
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 1 0.3
        tcMod scroll 0 -0.3
    }
}

gfx/effects/shield
{
	cull	disable
    {
        map gfx/effects/shield
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/alpha_smoke
{
	cull	disable
    {
        map gfx/effects/alpha_smoke
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/effects/alpha_smoke2
{
	cull	disable
    {
        map gfx/effects/alpha_smoke2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/effects/flamejet
{
	cull	disable
    {
        map gfx/effects/flamejet
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/effects/whiteFlash
{
	nomipmaps
	sort	nearest
	cull	disable
    {
        map gfx/effects/solidwhite
        blendFunc GL_ONE GL_ONE
        depthFunc disable
        rgbGen vertex
    }
}

gfx/effects/electricShock
{
	cull	disable
    {
        map gfx/effects/electricshock
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/electricShock2
{
	cull	disable
    {
        map gfx/effects/electricshock
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale -1 1
    }
}

gfx/effects/burn
{
	deformvertexes	bulge	0 -0.2 0
    {
        map gfx/effects/burn
        blendFunc GL_ONE GL_ONE
        glow
        tcMod scale 4 3
        tcMod scroll 1 0.6
    }
}

gfx/effects/pool1
{
	cull	disable
    {
        map gfx/effects/pool1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave sin 0.7 0.3 0.6 0
        tcMod scroll 0.03 0
        tcMod scale 5 1
    }
}

gfx/effects/pool2
{
	cull	disable
    {
        map gfx/effects/pool1
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen wave sin 0.7 0.3 0.7 0
        tcMod scroll 0.1 0
        tcMod scale 3 1
    }
}

gfx/effects/pool_static
{
	cull	disable
    {
        map gfx/effects/static2
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen const ( 0.700000 0.700000 0.300000 )
        tcMod scroll -0.02 0
        tcMod scale 7 3
    }
}

gfx/effects/whiteFlare2
{
	cull	disable
    {
        map gfx/effects/whiteflare2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
    }
}

gfx/effects/invin_glow
{
	cull	disable
    {
        map gfx/effects/invin_glow2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scroll 1 0
        tcMod scale 3 1
    }
    {
        map gfx/effects/invin_glow2
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale -3 1
        tcMod scroll -2 0
    }
}

gfx/effects/demp2shell
{
	cull	disable
    {
        map gfx/effects/plasma
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod scale 2 3
        tcMod scroll 1 0.5
    }
    {
        map gfx/effects/plasma
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 3 2
        tcMod scroll 0.5 2.6
    }
}

gfx/misc/electric2
{
	cull	disable
    {
        map gfx/misc/blue_bolt2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scroll 0 -1.5
        tcMod scale -1 2
    }
}

gfx/misc/ripple
{
	cull	disable
    {
        map gfx/misc/ripple
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
    }
}

gfx/misc/shockwave
{
	cull	disable
    {
        map gfx/misc/shockwave
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod scale 2 1
    }
}

gfx/misc/whiteLine2
{
	cull	disable
    {
        map gfx/misc/whiteline2
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
    }
}

gfx/misc/debugArrow
{
	cull	disable
    {
        map gfx/misc/debugarrow
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/misc/debugAmbient
{
	cull	disable
    {
        map gfx/misc/debugambient
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/misc/grayRing
{
	cull	disable
    {
        map gfx/misc/grayring
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen vertex
        tcMod scale 2 1
    }
}

gfx/2d/droid_view
{
	nomipmaps
	cull	disable
    {
        clampmap gfx/2d/droid_view
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen wave random 0.994 0.006 0 1
    }
}

models/players/reborn/boss_torso
{
    {
        map models/players/reborn/boss_torso
        blendFunc GL_ONE GL_ZERO
        rgbGen lightingDiffuse
    }
    {
        map models/players/reborn/boss_torso_s
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
        alphaGen lightingSpecular
    }
}

gfx/world/haze
{
	cull	disable
    {
        map gfx/world/bhaze
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen exactVertex
        tcMod scale 0.006 0.004
        tcMod scroll 0.1 0
    }
}

gfx/world/haze2
{
	cull	disable
    {
        map gfx/world/bhaze
        blendFunc GL_ONE GL_ONE
        glow
        rgbGen exactVertex
        tcMod scale 0.008 0.007
        tcMod scroll 0.08 0.09
    }
}

