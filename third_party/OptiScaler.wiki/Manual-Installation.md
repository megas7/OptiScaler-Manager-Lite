> [!CAUTION]
> ***0a. Download the latest stable build of Optiscaler [from here](https://github.com/optiscaler/OptiScaler/releases/latest).***  
> 
> _We also have the Rolling/Nightly releases which might have some newer bleeding edge things_ - [_**Nightly builds**_](https://github.com/optiscaler/OptiScaler/releases/tag/nightly)
> 
> Example image  
> ![image](https://github.com/user-attachments/assets/bdd6bc63-1729-4d53-872a-1ca482859a81)

> [!NOTE]
> _If you want to send a log file, set `LogLevel=0` and `LogToFile=true`, reproduce the issue and zip the log if it's too big._

> [!NOTE]
> * Pressing **`Insert`** should open the Optiscaler **Overlay** in-game with all of the options (_`ShortcutKey=` can be changed in the config file_). 
> * Pressing **`Page Up`** shows the performance stats overlay in the top left, and can be cycled between different modes with **`Page Down`** (_keybinds customisable in the overlay_).  
> * If Opti overlay is instantly disappearing after trying Insert a few times, maybe try **`Alt + Insert`** ([reported workaround](https://github.com/optiscaler/OptiScaler/issues/484) for alternate keyboard layouts).
> * For some games (e.g. Dying Light 2), mouse doesn't work in the overlay, so keyboard navigation is required - Arrow keys, Tab and Space  
>  
For more info, please check [Readme - How it works?](https://github.com/optiscaler/OptiScaler?tab=readme-ov-file#how-it-works)


> [!TIP]
> **0b.** Please check the [**General Compatibility List**](https://github.com/optiscaler/OptiScaler/wiki/Compatibility-List) first for already documented fixes and workarounds - same for [**FSR4 Compatibility List**](https://github.com/optiscaler/OptiScaler/wiki/FSR4-Compatibility-List)

## <img src="https://github.com/user-attachments/assets/09ef68f2-34f0-41ce-8065-66c87c51f7cb" width="200" />

**`Step-by-step installation:`**  

**1.** Extract **all** of the Optiscaler files **by the main game exe**  

> [!IMPORTANT]
> _(for _**Unreal Engine**_ games, look for the _**win_shipping.exe**_ in one of the subfolders, generally **`<path-to-game>\Game-or-Project-name\Binaries\Win64 or WinGDK\`**, **ignore** the `Engine` folder)_  
> 
> **Example paths** - _`Expedition 33\Sandfall\Binaries\Win64`_, `Jedi Survivor\SwGame\Binaries\Win64`, `Cyberpunk 2077\bin\x64`, `HITMAN 3\Retail`, `Warhammer 40,000 DARKTIDE\binaries`, `The Witcher 3\bin\x64_dx12`

**2.** Rename OptiScaler's `OptiScaler.dll` (for old versions, it's `nvngx.dll`) to one of the [supported filenames](#optiscaler-supports-these-filenames) (preferred `dxgi.dll`, but depends on the game)$`^1`$  

> [!NOTE]
> _For FSR2/3-only games that **don't have DLSS** (e.g. The Callisto Protocol or The Outer Worlds: Spacer's Choice Edition), you have to **provide** the `nvngx_dlss.dll` in order to use DLSS in Optiscaler - download link e.g. [TechPowerUp](https://www.techpowerup.com/download/nvidia-dlss-dll/)_

## <img src="https://github.com/user-attachments/assets/5f84e646-2e9f-4405-9ec7-3342177040d0" width="150" /> / <img src="https://github.com/user-attachments/assets/2f09d69b-bf4b-40a6-91c1-e4de61f9fd64" width="100" />

**`Step-by-step installation:`**  

**1.** Extract **all** of the Optiscaler files **by the main game exe**  

> [!IMPORTANT]
> _(for _**Unreal Engine**_ games, look for the _**win_shipping.exe**_ in one of the subfolders, generally **`<path-to-game>\Game-or-Project-name\Binaries\Win64 or WinGDK\`**, **ignore** the `Engine` folder)_  
> 
> **Example paths** - _`Expedition 33\Sandfall\Binaries\Win64`_, `Jedi Survivor\SwGame\Binaries\Win64`, `Cyberpunk 2077\bin\x64`, `HITMAN 3\Retail`, `Warhammer 40,000 DARKTIDE\binaries`, `The Witcher 3\bin\x64_dx12`  

**2.** Rename OptiScaler's `OptiScaler.dll` (for old versions, it's `nvngx.dll`) to one of the [supported filenames](#optiscaler-supports-these-filenames) (preferred `dxgi.dll`, but depends on the game)$`^1`$  

* **Spoofing** is always **enabled** by default. If you want to disable it, then set `Dxgi=false` in _**OptiScaler.ini**_.
* **Step 3** is no longer needed since **0.7.7-pre12/0.7.8** - only required in some rare edge case where Optiscaler can't automatically locate `nvngx_dlss.dll` when spoofing - in such cases, just notify us and we'll improve the locator
* With the changes mentioned above, spoofing working properly means `nvngx replacement: Exists` will be mentioned in the Opti Overlay, and that's the important part, `nvngx.dll: Doesn't Exist` is irrelevant if nvngx replacement exists.

~**3a.** **Either** locate the `nvngx_dlss.dll` file (for UE games, generally in one of the subfolders under `Engine/Plugins`), create a copy, rename the copy to `nvngx.dll` and put it beside Optiscaler~    

~**3b.** **OR** download `nvngx_dlss.dll` from e.g. [TechPowerUp](https://www.techpowerup.com/download/nvidia-dlss-dll/) or [Streamline SDK repo](https://github.com/NVIDIAGameWorks/Streamline/tree/main/bin/x64) if you don't want to search, rename it to `nvngx.dll` and put it beside Optiscaler~   

> [!TIP]
> If **AMD/Intel** and **DLSS inputs** aren't visible even after selecting **Yes** during Install, game requires adding [**Fakenvapi**](https://github.com/optiscaler/OptiScaler/wiki/Fakenvapi)

---

> [!CAUTION]
> ### If you need **Nukem** or **Fakenvapi**, now check their respective pages:
> * [**NukemFG mod**](https://github.com/optiscaler/OptiScaler/wiki/Nukem's-dlssg%E2%80%90to%E2%80%90fsr3)
> * [**Fakenvapi**](https://github.com/optiscaler/OptiScaler/wiki/Fakenvapi)

---
> [!TIP]
> *[1] Linux users should add renamed dll to overrides:*
> ```
> WINEDLLOVERRIDES=dxgi=n,b %COMMAND% 
> ```

> [!IMPORTANT]
> **Please don't rename the ini file, it should stay as `OptiScaler.ini`**.

> [!NOTE]
> ### OptiScaler supports these filenames:
> * dxgi.dll 
> * winmm.dll
> * d3d12.dll _(from 0.7.7 nightlies)_  
> * dbghelp.dll _(from 0.7.7 nightlies)_  
> * version.dll
> * wininet.dll
> * winhttp.dll
> * OptiScaler.asi _(needs [Ultimate ASI Loader x64](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) or similar)_


---
> [!NOTE]
> ### _Example of correct AMD/Intel installation (with additional Fakenvapi and Nukem mod)_
> ![Installation](https://github.com/user-attachments/assets/977a2a68-d117-42ea-a928-78ec43eedd28)
