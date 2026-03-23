# shorkfont

A utility for SHORK Operating Systems that changes the console terminal's foreground (text) colour or PC Screen Font (PSF) font. It takes two arguments (type of change and new name); running it without an argument shows how to use the utility and a list of possible colours. Any changes made will be stored in `/etc/shorkfont.conf` and will be reloaded upon reboot.

**This is not intended for use on non-SHORK systems!**

## Example usage

### Changing colour

    shorkfont -c cyan_bright

### Changing font (full path)

    shorkfont -n /home/terminus.psf

### Changing font (name only)

    shorkfont -n Lat15-Fixed16

_Note: for this to work, the font file must be stored in `/usr/share/consolefonts`_
