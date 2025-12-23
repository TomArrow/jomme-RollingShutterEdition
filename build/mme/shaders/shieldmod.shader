halfShieldShell
{
	qer_editorimage	gfx/effects/shieldshell1.tga
	nomipmaps
	polygonOffset
	cull	disable
    {
        map gfx/effects/shieldshell1
        blendFunc GL_ONE GL_ONE
        rgbGen entity
        tcMod stretch sin 0.9 0.1 0 2
        tcMod rotate 55
    }
    {
        map gfx/effects/shieldshellring
        blendFunc GL_ONE GL_ONE
        rgbGen wave inversesawtooth 0 1 0 1.5
        tcMod stretch sawtooth 0.6 3.4 0 1.5
    }
}