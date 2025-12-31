gfx/effects/shock_ripple
{
	cull	disable
    {
        map gfx/effects/shock_ripple
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale 3 1
        tcMod scroll 0.9 0
    }
    {
        map gfx/effects/shock_ripple
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale 2 1
        tcMod scroll -0.4 0
    }
}

gfx/effects/stun
{
	cull disable
	{
		map gfx/effects/stun
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/sho
{
	cull	disable
    {
        map gfx/effects/sho
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale 1 0.5
        tcMod scroll 0 2.9
    }
    {
        map gfx/effects/sho
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale 1 1
        tcMod scroll 0 -1.8
    }
}

gfx/effects/shock_ball
{
	cull	disable
    {
        map gfx/effects/shock_ball
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
    }
}

gfx/effects/saberDamageGlow
{
	polygonOffset
	cull	disable
	entityMergable
    {
        map gfx/effects/saberdamageglow
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 2.0 2.0 2.0 )
    }
}


gfx/effects/fire2
{
	cull	disable
    {
        map gfx/effects/fire2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/fire3
{
	cull	disable
    {
        map gfx/effects/fire3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/fire4
{
	cull	disable
    {
        map gfx/effects/fire4
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/white_fire1
{
	cull disable
    {
        map gfx/effects/white_fire1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/white_fire2
{
	cull disable
    {
        map gfx/effects/white_fire2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/white_fire1mult
{
	cull disable
    {
        map gfx/effects/white_fire1
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen vertex
    }
}

gfx/effects/white_fire2mult
{
	cull disable
    {
        map gfx/effects/white_fire2
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen vertex
    }
}

gfx/effects/cloakedShader
{
    {
        map gfx/effects/chrome2
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen entity
        tcGen environment
        tcMod scroll 0.3 0.2
        tcMod turb 0.6 0.3 0 0.2
    }
}

gfx/effects/wookie1
{
	cull	disable
    {
        map gfx/effects/wookie1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/wookie2
{
	cull	disable
    {
        map gfx/effects/wookie2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/whiteGlow
{
	nomipmaps
    {
        map gfx/effects/whiteglow
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/blaster_blob
{
	cull	disable
    {
        map gfx/effects/blaster_blob
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
		rgbMult ( 4.0 4.0 4.0 )
		heatMult 16.0
    }
}

gfx/effects/bryar_blob
{
	cull	disable
    {
        map gfx/effects/bryar_blob
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen vertex
		rgbMult ( 4.0 4.0 4.0 )
		heatMult 16.0
    }
}

gfx/effects/forcePush
{
	cull	disable
    {
        map gfx/effects/force_push
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/saberFlare
{
	nomipmaps
    {
        map gfx/effects/saberflare
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
		//rgbMult ( 3.0 3.0 3.0 )
    }
}

gfx/effects/blasterSideFlash
{
	cull	disable
    {
        map gfx/effects/blastersideflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/blasterFrontFlash
{
	cull	disable
    {
        map gfx/effects/blasterfrontflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/bryarSideFlash
{
	cull	disable
    {
        map gfx/effects/bryarsideflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/bryarFrontFlash
{
	cull	disable
    {
        map gfx/effects/bryarfrontflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/greenSideFlash
{
	cull	disable
    {
        map gfx/effects/greensideflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/greenFrontFlash
{
	cull	disable
    {
        map gfx/effects/greenfrontflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/redLine
{
	cull	disable
    {
        map gfx/effects/redline
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/greenShot
{
	cull	disable
    {
        map gfx/effects/green_shot
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/effects/rorangeShot
{
	cull	disable
    {
        map gfx/effects/rorange_shot
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/effects/plume1
{
	cull	disable
    {
        map gfx/effects/plume1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/plume2
{
	cull	disable
    {
        map gfx/effects/plume2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/plume3
{
	cull disable
	{
		map gfx/effects/plume3
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/effects/light_cone
{
	cull	disable
    {
        map gfx/effects/light_cone
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 3 1
        tcMod scroll 0.3 0
    }
    {
        map gfx/effects/light_cone
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
        tcMod scale 4 1
        tcMod scroll -0.1 0
    }
}

gfx/effects/lava
{
	cull	disable
    {
        map gfx/effects/lava
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/effects/lava2
{
	cull	disable
    {
        map gfx/effects/lava2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
        alphaGen vertex
    }
}

gfx/effects/rocket_muz
{
	cull	disable
    {
        map gfx/effects/rocket_muz
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/effects/rocket_muz2
{
	cull	disable
    {
        map gfx/effects/rocket_muz2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/spark
{
	entityMergable
	cull	disable
    {
        map gfx/misc/spark
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 6.0 6.0 6.0 )
    }
}

gfx/misc/spark2
{
	entityMergable
	cull	disable
    {
        map gfx/misc/spark2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 6.0 6.0 6.0 )
    }
}

gfx/misc/spark3
{
	entityMergable
	cull	disable
    {
        map gfx/misc/spark3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 6.0 6.0 6.0 )
    }
}

gfx/misc/steam
{
	cull	disable
    {
        map gfx/misc/steam
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/steam2
{
	cull	disable
    {
        map gfx/misc/steam2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/steam3
{
	cull	disable
    {
        map gfx/misc/steam3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/black_smoke
{
	cull	disable
    {
        map gfx/effects/black_smoke
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/misc/black_smoke2
{
	cull	disable
    {
        map gfx/effects/black_smoke2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/misc/dotfill
{
	cull	disable
    {
        map gfx/misc/dotfill
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/dotfill_a
{
	cull	disable
    {
        map gfx/misc/dotfill_a
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/misc/dots
{
	cull	disable
    {
        map gfx/misc/dots
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/smoke2
{
	cull	disable
    {
        map gfx/effects/smoke2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/misc/personalshield
{
	deformvertexes	bulge	0 0.75 0
    {
        map gfx/effects/p_shield
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen entity
        tcGen environment
        tcMod rotate 200
        tcMod turb 0.6 0.3 0 0.6
    }
    {
        map gfx/effects/p_shield
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen entity
        tcMod rotate -600
        tcMod scale 2 3
    }
}

gfx/misc/ion_shield
{
	deformvertexes	bulge	0 12 0
    {
        map gfx/effects/p_shield
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen entity
        tcGen environment
        tcMod rotate 200
        tcMod turb 0.6 0.3 0 0.6
        tcMod scale 5 8
    }
    {
        map gfx/effects/p_shield
        blendFunc GL_DST_COLOR GL_ONE
        rgbGen entity
        tcMod rotate -600
        tcMod scale 4 6
    }
}

gfx/misc/spark_group
{
	entityMergable
	cull	disable
    {
        map gfx/misc/spark_group
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	rgbMult ( 6.0 6.0 6.0 )
    }
}

gfx/misc/exp01_1
{
	cull	disable
    {
        map gfx/misc/exp01_1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/exp01_2
{
	cull	disable
    {
        map gfx/misc/exp01_2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/exp02_2
{
	cull	disable
    {
        map gfx/misc/exp02_2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/exp02_3
{
	cull	disable
    {
        map gfx/misc/exp02_3
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/test_crackle
{
	cull	disable
    {
        map gfx/misc/test_crackle
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/misc/smallGlassChunk1
{
	cull	disable
    {
        map gfx/misc/small_glass1
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/smallGlassChunk2
{
	cull	disable
    {
        map gfx/misc/small_glass2
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/blueLine
{
	cull	disable
    {
        map gfx/misc/blueline
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/electric
{
	cull	disable
	deformvertexes	bulge	0 2 0
    {
        map gfx/misc/lightning3
        blendFunc GL_ONE GL_ONE
        rgbGen identity
        tcMod scroll 0.5 1
        tcMod scale 3 4.5
    }
}

gfx/misc/fullbodyelectric2
{
	cull	disable
	deformvertexes	bulge	0 0.25 0
    {
        map gfx/misc/lightning3
        blendFunc GL_ONE GL_ONE
        rgbGen identity
        tcMod scroll 0.5 1
        tcMod scale -2 -3.5
    }
}

gfx/misc/lightningFlash
{
	cull	disable
    {
        map gfx/misc/lightningflash
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/misc/dust
{
	cull	disable
    {
        map gfx/misc/dust
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

effects/fire
{
	cull	disable
    {
        map gfx/effects/fire
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/2d/wedge
{
	nomipmaps
	cull	disable
    {
        map gfx/2d/wedge
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/2d/lock
{
	cull	disable
    {
        map gfx/2d/lock
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/2d/insertTick
{
	nomipmaps
	cull	disable
    {
        map gfx/2d/tick
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
    }
}

gfx/2d/cropCircle
{
	nomipmaps
	cull	disable
    {
        map gfx/2d/cropcircle
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        detail
        rgbGen vertex
    }
}

gfx/2d/cropCircle2
{
	nomipmaps
	cull	disable
    {
        map gfx/misc/scanline
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
        tcMod scale 1 10.5
    }
    {
        map gfx/2d/cropcircle2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/2d/cropCircleGlow
{
	nomipmaps
	cull	disable
    {
        map gfx/2d/cropcircleglow
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        rgbGen identity
    }
}

gfx/effects/irid_shield
{
	cull disable
	{
	map gfx/effects/mp_weapon_holo2
	blendFunc GL_ONE GL_ONE
	rgbGen const ( 0.2 0.2 0.2 )
	tcmod scroll 0 -0.2
	tcmod scale 2 6
	}
	{
	map gfx/effects/mp_weapon_holo2
	blendFunc GL_SRC_ALPHA GL_ONE
	rgbGen identity
	alphaGen lightingSpecular
	tcmod scroll 0 -0.3
	tcmod scale 2 10
	}
}

models/map_objects/imp_mine/turret_chair_dmg
{
    {
        map models/map_objects/imp_mine/turret_chair_dmg
        blendFunc GL_ONE GL_ZERO
        rgbGen lightingDiffuse
    }
}

gfx/effects/slime1
{
	cull disable
	{
		map gfx/effects/slime1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/effects/slime2
{
	cull disable
	{
		map gfx/effects/slime2
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/effects/shard
{
	cull disable
	{
		map gfx/effects/shard
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/effects/drained
{
	cull	disable
    {
        map gfx/effects/drained
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
	}
}


gfx/effects/drainedadd
{
	cull	disable
    {
        map gfx/effects/drainedadd
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	}
}


gfx/effects/ripple
{
	cull	disable
    {
        map gfx/effects/ripple
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	}
}

gfx/effects/protectionfield
{
	cull	disable
    {
        map gfx/effects/protectionfield
        blendFunc GL_ONE GL_ONE
        rgbGen vertex
	}
}

gfx/effects/yellow_glow
{
	cull	disable
    {
        map gfx/effects/sabers/yellow_glow.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

gfx/effects/water_splash
{
	cull disable
	{
		map gfx/effects/water_splash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}