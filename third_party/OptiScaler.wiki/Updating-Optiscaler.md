## Method 1

_**Probably the easiest and "safest" method**_

**1.** Run `Remove OptiScaler.bat`, press `Y` and this will remove Optiscaler files including the config file _(doesn't include Fakenvapi and Nukem)_  
**2.** Download **newer version** of Optiscaler  
**3.** Just follow the **regular install**  

## Method 2

Since the **Setup BAT** more or less renames the `OptiScaler.dll` to another name, like `dxgi.dll`, it's generally enough to just replace the renamed Opti DLL with a newer one.

**1.** Download **newer version** of Optiscaler  
**2.** Rename `OptiScaler.dll` to whatever name you used in the BAT file when installing Opti for that game (e.g. you chose `dxgi`, rename `OptiScaler.dll` to `dxgi.dll`)  
**3.** Move the **renamed Opti DLL** to game folder and **replace the old Opti DLL**  

To verify you did it right, just check if Opti Overlay is showing the new version in the title bar.