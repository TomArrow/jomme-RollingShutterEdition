textures/yavin/grasspatchy_swampsprite
{
	qer_editorimage	textures/yavin/groundjungle
	q3map_nolightmap
	cull	disable
    {
        map textures/yavin/groundjungle
    }
    {
        map gfx/sprites/ss_grass_grasspatchy
            surfaceSprites vertical 32 36 42 500
            ssFademax 1500
            ssFadescale 1
            ssVariance 1 2
            ssWind 0.5
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
}

textures/yavin/grasspatchy_reeds
{
	qer_editorimage	textures/yavin/groundjungle
	q3map_nolightmap
	cull	disable
    {
        map textures/yavin/groundjungle
    }
    {
        map gfx/sprites/ss_grass_grasspatchy
            surfaceSprites vertical 20 24 48 500
            ssFademax 1500
            ssFadescale 1
            ssVariance 1 2
            ssWind 0.5
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
    {
        map gfx/sprites/ss_cattail
            surfaceSprites vertical 20 32 50 500
            ssVariance 1 2.5
            ssWind 0.8
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
}

textures/yavin/grasspatchy_underwater
{
	qer_editorimage	textures/yavin/groundjungle
	q3map_nolightmap
	cull	disable
    {
        map textures/yavin/groundjungle
    }
    {
        map gfx/sprites/ss_hangvine2
            surfaceSprites vertical 10 16 30 300
            ssVariance 1 2
            ssWind 1
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
    {
        clampmap gfx/sprites/ss_bubbles
            surfaceSprites effect 2.5 1 36 400
            ssFademax 700
            ssVariance 0.75 1
            ssFXDuration 2500
            ssFXGrow 2.5 30
            ssFXAlphaRange 0.5 0
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
}

textures/yavin/s_rock1_vines
{
	qer_editorimage	textures/yavin/rockmossy
	q3map_nolightmap
	q3map_onlyvertexlighting
	cull	disable
    {
        map textures/yavin/rockmossy
    }
    {
        map gfx/sprites/ss_hangvine
            surfaceSprites vertical 10 15 42 600
            ssFademax 1400
            ssVariance 2 6
            ssHangDown
            ssWind 0.4
            ssWindidle 0.3
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
    {
        map gfx/sprites/ss_hangvine2
            surfaceSprites vertical 6 12 49 500
            ssFademax 1200
            ssVariance 1 6
            ssHangDown
            ssWind 0.4
            ssWindidle 0.3
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
}

textures/yavin/water1_2sided
{
	qer_editorimage	textures/yavin/water1
	q3map_tesssize	256
	surfaceparm	nonsolid
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.105098 0.147157 0.0431373 ) 128.0
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ZERO
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ZERO
        tcMod turb 0 0.1 0.4 0.09
    }
    {
        clampmap gfx/sprites/rainring2
            surfaceSprites effect 1 1 48 300
            ssFademax 600
            ssVariance 2 1
            ssFaceup
            ssFXDuration 800
            ssFXGrow 10 10
            ssFXAlphaRange 1 0
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
    {
        clampmap gfx/sprites/bubble2
            surfaceSprites effect 0.6 0.45 48 300
            ssFademax 600
            ssVariance 1 1
            ssFXDuration 800
            ssFXGrow 2 2
            ssFXAlphaRange 0.2 0.5
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
    {
        clampmap gfx/sprites/fog
            surfaceSprites effect 20 8 50 200
            ssFademax 1300
            ssFadescale 2
            ssVariance 2 1
            ssWind 10
            ssFXDuration 5000
            ssFXGrow 2 2
            ssFXAlphaRange 0.75 0
        blendFunc GL_ONE GL_ONE
        detail
    }
}

textures/yavin/water1_temple
{
	qer_editorimage	textures/yavin/water1
	q3map_tesssize	256
	surfaceparm	metalsteps
	surfaceparm	nonsolid
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.105098 0.147157 0.0431373 ) 512.0
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ZERO
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
    }
    {
        map textures/yavin/water_test
        blendFunc GL_ONE GL_ONE
        tcMod turb 0 0.1 0.4 0.09
    }
}

textures/yavin/water1_2sided_redux
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 0.05 0.1
        tcMod scale 3 3
    }
}

textures/yavin/water1
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.115098 0.122157 0.0431373 ) 128.0
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 0.05 0.1
        tcMod scale 3 3
    }
}

textures/yavin/gravel
{
	qer_editorimage	textures/yavin/dugdirt
	surfaceparm	slick
	q3map_nolightmap
	q3map_vlight
    {
        map textures/yavin/dugdirt
    }
}

textures/yavin/light
{
	q3map_flare	gfx/misc/flare
	q3map_nolightmap
    {
        map textures/yavin/light
        alphaFunc GE128
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
    }
}

textures/yavin/light_blue
{
	q3map_flare	gfx/misc/flare
	q3map_nolightmap
    {
        map textures/yavin/light_blue
        alphaFunc GE128
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
    }
}

textures/yavin/ycamera
{
	qer_editorimage	textures/yavin/ycamera
    {
        map gfx/effects/decoystatic
        blendFunc GL_ONE GL_ZERO
        rgbGen wave sin 1 0.25 0 1
        tcMod scroll 5 7
        tcMod scale 9 7
    }
    {
        map gfx/effects/decoystatic
        blendFunc GL_ONE GL_ZERO
        rgbGen wave sin 1 0.5 0 1
        tcMod scroll -2 -1
        tcMod scale 55 55
    }
    {
        map textures/yavin/ycamera
        blendFunc GL_ONE GL_SRC_ALPHA
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_SRC_COLOR
    }
    {
        map textures/yavin/ycameraglow
        blendFunc GL_ONE GL_ONE
        rgbGen wave square 0 1 0 1
	glow
    }
}

textures/yavin/yswitch
{
    {
        map $lightmap
    }
    {
        map textures/yavin/yswitch
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/yswitchglow
        blendFunc GL_ONE GL_ONE
        rgbGen wave square 0 1 0 1
	glow
    }
    {
        map textures/yavin/yswitcha
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 1 0.05 0 5
	glow
    }
}

textures/yavin/yswitchon
{
	qer_editorimage	textures/yavin/yswitch
    {
        map $lightmap
    }
    {
        map textures/yavin/yswitch2
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/yswitchglow
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 1 0.2 0 5
	glow
    }
    {
        map textures/yavin/yswitchglow2
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 1 0.05 0 5
	glow
    }
}

textures/yavin/ydoorswitch
{
    {
        map $lightmap
    }
    {
        map textures/yavin/ydoorswitch
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/ydoorswitchglow1
        blendFunc GL_ONE GL_ONE
        rgbGen wave square 1 1 0 1
	glow
    }
    {
        map textures/yavin/ydoorswitchglow1a
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 1 0.05 0 5
	glow
    }
}

textures/yavin/ydoorswitchon
{
	qer_editorimage	textures/yavin/ydoorswitch
    {
        map $lightmap
    }
    {
        map textures/yavin/ydoorswitch
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/ydoorswitchglow2
        blendFunc GL_ONE GL_ONE
        rgbGen wave sin 1 0.05 -0.5 5
	glow
    }
    {
        map textures/yavin/ydoorswitchglow1
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/yeleswitchon
{
	qer_editorimage	textures/yavin/yeleswitch
    {
        map $lightmap
    }
    {
        map textures/yavin/yeleswitch
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/yeleswitchglow
        blendFunc GL_ONE GL_ONE
	glow
    }
    {
        map textures/yavin/yeleswitchglow2
        blendFunc GL_ONE GL_ONE
        rgbGen wave sawtooth 0 1 0 1
	glow
    }
}

textures/yavin/light_yellow
{
	q3map_flare	gfx/misc/flare
    {
        map $lightmap
    }
    {
        map textures/yavin/light_yellow
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/light_yellowglow
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/water1_nofog
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 0.05 0.1
        tcMod scale 3 3
    }
}

textures/yavin/yeleswitch
{
	qer_editorimage	textures/yavin/yeleswitch
    {
        map $lightmap
    }
    {
        map textures/yavin/yeleswitch
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/yeleswitchglow
        blendFunc GL_ONE GL_ONE
        rgbGen wave square 1 0.5 0 1
	glow
    }
}

textures/yavin/rock2
{
	q3map_nolightmap
    {
        map textures/yavin/rock2
    }
}

textures/yavin/crate03
{
    {
        map $lightmap
    }
    {
        map textures/yavin/crate03
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/crate03_glow
        blendFunc GL_ONE GL_ONE
        glow
    }
}

textures/yavin/control01
{
    {
        map $lightmap
    }
    {
        map textures/yavin/control01
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/control01_glw
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/stone_tile2
{
    {
        map $lightmap
    }
    {
        map textures/yavin/stone_tile2
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/stone_tile2_glow
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/temple_vinesalpha
{
    {
        map textures/yavin/temple_vinesalpha
        alphaFunc GE128
        blendFunc GL_SRC_ALPHA GL_ZERO
        depthWrite
    }
    {
        map $lightmap
        blendFunc GL_ONE GL_ZERO
        depthFunc equal
    }
    {
        map textures/yavin/temple_vinesalpha
        blendFunc GL_DST_COLOR GL_ZERO
        depthFunc equal
    }
}

textures/yavin/water_test
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_pass1
        blendFunc GL_DST_COLOR GL_SRC_ALPHA
        tcMod scale 3 3
        tcMod turb 0 0.2 0 0.1
        tcMod scroll 0.1 0.2
    }
}

textures/yavin/water_test2
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scale 3 3
        tcMod scroll 0.05 0.1
    }
}

textures/yavin/water_test3
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
    {
        map textures/yavin/water_test
        blendFunc GL_ONE GL_SRC_COLOR
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test2
        blendFunc GL_ONE_MINUS_SRC_ALPHA GL_ONE
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
}

textures/yavin/water_test4
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
    {
        map textures/yavin/water_test3
        blendFunc GL_ONE GL_SRC_COLOR
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 2 2
    }
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_SRC_COLOR
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
}

textures/yavin/water_test5
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test3
        blendFunc GL_DST_COLOR GL_ONE
        tcMod rotate 2
        tcMod scroll 0.025 0.035
        tcMod scale 3 3
    }
}

textures/yavin/temple_illusion
{
	qer_editorimage	textures/yavin/temple_interiorsmall2
	surfaceparm	nonsolid
    {
        map $lightmap
    }
    {
        map textures/yavin/temple_interiorsmall2
        blendFunc GL_DST_COLOR GL_ZERO
        tcMod turb 0 0.03 0 0.075
    }
}

textures/yavin/slime
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	lava
	surfaceparm	water
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 0.05 0.1
        tcMod scale 3 3
    }
}

textures/yavin/grasspatchy_sprite
{
	qer_editorimage	textures/yavin/grasspatchy3
	q3map_nolightmap
    {
        map textures/yavin/grasspatchy3
    }
    {
        map gfx/sprites/ss_grass_grasspatchy2
            surfaceSprites vertical 32 20 35 800
            ssFademax 1500
            ssFadescale 2
            ssVariance 1 2
            ssWind 0.8
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
}

textures/yavin/grasspatchy_sm_sprite
{
	qer_editorimage	textures/yavin/grasspatchy3
	q3map_nolightmap
    {
        map textures/yavin/grasspatchy3
    }
    {
        map gfx/sprites/ss_grass_grasspatchy
            surfaceSprites vertical 8 8 40 200
            ssFademax 600
            ssFadescale 2
            ssVariance 1 1
            ssWind 0.2
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

textures/yavin/dirtgrasscorner_sprite
{
	qer_editorimage	textures/yavin/dirtgrasscorner
	q3map_nolightmap
    {
        map textures/yavin/dirtgrasscorner
    }
    {
        map gfx/sprites/rock_sm
            surfaceSprites oriented 2 2 100 200
            ssFademax 400
            ssVariance 2 1.5
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

textures/yavin/dirtgrassedge_sprite
{
	qer_editorimage	textures/yavin/dirtgrassedge
	q3map_nolightmap
    {
        map textures/yavin/dirtgrassedge
    }
    {
        map gfx/sprites/rock_sm
            surfaceSprites oriented 2 2 100 200
            ssFademax 400
            ssVariance 2 1.5
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

textures/yavin/dirt1_sprite
{
	qer_editorimage	textures/yavin/dirt1
	q3map_nolightmap
    {
        map textures/yavin/dirt1
    }
    {
        map gfx/sprites/rock_sm
            surfaceSprites oriented 2 2 64 200
            ssFademax 400
            ssVariance 2 1.5
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen vertex
    }
}

textures/yavin/grasspatchy2
{
	qer_editorimage	textures/yavin/grasspatchy3
	q3map_nolightmap
    {
        map textures/yavin/grasspatchy3
    }
    {
        map gfx/sprites/ss_grass_grasspatchy
            surfaceSprites vertical 32 20 35 800
            ssFademax 1500
            ssFadescale 2
            ssVariance 1 2
            ssWind 0.8
        alphaFunc GE192
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen vertex
    }
}

textures/yavin/doorlight_red
{
    {
        map $lightmap
    }
    {
        map textures/yavin/doorlight_red
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/doorlight_red_glw
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/doorlight_green
{
    {
        map $lightmap
    }
    {
        map textures/yavin/doorlight_green
        blendFunc GL_DST_COLOR GL_ZERO
    }
    {
        map textures/yavin/doorlight_green_glw
        blendFunc GL_ONE GL_ONE
	glow
    }
}

textures/yavin/swater1
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.113725 0.121569 0.0431373 ) 512.0
}

textures/yavin/swater1_2sided
{
	qer_editorimage	textures/yavin/water1
	q3map_tesssize	256
	surfaceparm	slick
	q3map_nolightmap
	q3map_onlyvertexlighting
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ZERO
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ZERO
        tcMod turb 0 0.1 0.4 0.09
    }
    {
        clampmap gfx/sprites/rainring2
            surfaceSprites effect 1 1 48 300
            ssVariance 2 1
            ssFaceup
            ssFXDuration 800
            ssFXGrow 10 10
            ssFXAlphaRange 1 0.05
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
    {
        clampmap gfx/sprites/bubble2
            surfaceSprites effect 0.6 0.45 48 300
            ssVariance 1 1
            ssFXDuration 800
            ssFXGrow 2 2
            ssFXAlphaRange 0.4 0
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
    {
        clampmap gfx/sprites/fog
            surfaceSprites effect 20 8 50 300
            ssFademax 1000
            ssFadescale 2
            ssVariance 2 1
            ssWind 4
            ssFXDuration 5000
            ssFXGrow 2 2
            ssFXAlphaRange 0.75 0
            ssFXWeather
        blendFunc GL_ONE GL_ONE
        detail
    }
}

textures/yavin/swater_opaque_bottom
{
	qer_editorimage	textures/yavin/water1
	q3map_tesssize	256
	surfaceparm	slick
	q3map_nolightmap
	q3map_onlyvertexlighting
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ZERO
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_SRC_COLOR
        tcMod turb 0 0.1 0.4 0.09
    }
}

textures/yavin/water_yavintrail
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	nonsolid
	surfaceparm	nonopaque
	surfaceparm	water
	surfaceparm	fog
	surfaceparm	trans
	q3map_material	Water
	q3map_nolightmap
	q3map_onlyvertexlighting
	fogparms	( 0.145098 0.192157 0.0431373 ) 512.0
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod turb 0 0.08 0.04 0.08
        tcMod scroll -0.05 -0.001
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod turb 0 0.08 0.04 0.08
        tcMod scale 3 3
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scale 3 3
        tcMod scroll 0.05 0.1
    }
}

textures/yavin/groundjungle
{
	qer_editorimage	textures/yavin/groundjungle
	q3map_nolightmap
	cull	disable
    {
        map textures/yavin/groundjungle
    }
}

textures/yavin/waterfall
{
	qer_editorimage	textures/yavin/water1
	surfaceparm	slick
	surfaceparm	metalsteps
	q3map_nolightmap
	q3map_onlyvertexlighting
	cull	disable
    {
        map textures/yavin/water1
        blendFunc GL_ONE GL_ZERO
        rgbGen exactVertex
        alphaGen const 0.9
        tcMod scroll 0 -1
        tcMod scale 3 3
    }
    {
        map textures/yavin/water1
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        alphaGen const 0.33
        tcMod scroll 0 -0.25
    }
    {
        map textures/yavin/water_test
        blendFunc GL_DST_COLOR GL_ONE
        tcMod scroll 0 -0.15
        tcMod scale 3 3
    }
}

