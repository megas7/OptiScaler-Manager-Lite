**Automatic**

* Should be enough to run `Remove OptiScaler.bat`

**Manual**

* Run `DisableSignatureOverride.reg` file if previously enabled
* Delete `EnableSignatureOverride.reg`, `DisableSignatureOverride.reg`, `OptiScaler.dll` (for old versions, it's `nvngx.dll`), `OptiScaler.ini` files (if you used Fakenvapi and/or Nukem mod, then also delete `fakenvapi.ini`, `nvapi64.dll` and `dlssg_to_fsr3` files)
* If there was a `libxess.dll` file and you have backed it up, delete the new file and restore the backed up file. If you overwrote/replaced the old file, **DO NOT** delete `libxess.dll` file. If there was no `libxess.dll` before, it's safe to delete. Same goes for FSR files (`amd_fidelityfx`).