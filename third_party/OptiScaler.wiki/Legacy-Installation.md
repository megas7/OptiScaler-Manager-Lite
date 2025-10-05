`Step-by-step installation:`
1. Download the latest relase from [releases](https://github.com/optiscaler/OptiScaler/releases).
2. Extract the contents of the archive next to the game executable file in your games folder. (e.g. for Unreal Engine games, it's `<path-to-game>\Game-or-Project-name\Binaries\Win64 or WinGDK\`)$`^1`$
3. Rename `OptiScaler.dll` to `nvngx.dll` (For older builds, file name is already `nvngx.dll`, so skip this step)
4. Run `EnableSignatureOverride.reg` from `DlssOverrides` folder and confirm merge.$`^2`$$`^3`$

*[1] This package contains latest version of `libxess.dll` and if the game folder contains any older version of the same library, it will be overwritten. Consider backing up or renaming existing files.*

*[2] Normally Streamline and games check if nvngx.dll is signed, by merging this `.reg` file we are overriding this signature check.*

*[3] Adding signature override on Linux - There are many possible setups, this one will focus on Steam games:*
* *Make sure you have protontricks installed*
* *Run in a terminal protontricks <steam-appid> regedit, replace <steam-appid> with an id for your game*
* *Press "registry" in the top left of the new window -> `Import Registry File` -> navigate to and select `EnableSignatureOverride.reg`*
* *You should see a message saying that you successfully added the entries to the registry*

*If your game is not on Steam, it all boils down to opening regedit inside your game's prefix and importing the file.*