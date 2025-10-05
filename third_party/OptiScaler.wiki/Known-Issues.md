## In-Game Menu
* If you are using RTSS (MSI Afterburner, CapFrameX), please try enabling this setting in Setup/Settings and/or try updating RTSS if not working.

![image](https://github.com/optiscaler/OptiScaler/assets/35529761/8afb24ac-662a-40ae-a97c-837369e03fc7)

> [!TIP]
> * If Opti overlay is instantly disappearing after trying Insert a few times, maybe try **`Alt + Insert`** ([reported workaround](https://github.com/optiscaler/OptiScaler/issues/484) for alternate keyboard layouts).
> * For some games (e.g. Dying Light 2), **mouse doesn't work in the overlay**, so **keyboard navigation** is required - Arrow keys, Tab and Space
> * Sometimes pressing **Esc** to pause the game unlocks the mouse input in the overlay  
  

## Resource Barriers
* The Unreal Engine DLSS plugin is known to send DLSS resources in the wrong state.  
* Normally OptiScaler checks engine info from NVSDK and automatically enables necessary fixes for Unreal Engine games, but some games do not report engine info correctly.  
* This problem usually manifests itself as colored areas at the bottom of the screen or dark/black blocks at top right corner of the screen.
---
<details>
<summary><b> Click here for screenshot examples </b></summary>

![christmas lights](https://github.com/optiscaler/OptiScaler/blob/master/images/christmas.png)<br>*Deep Rock Galactic*
  
![rdna4_fun](https://github.com/user-attachments/assets/ee1fe9f7-021d-4b22-85a8-86d52ffbf35f)<br>*Ghostrunner 2*

</details>

---
> [!NOTE]
> Workaround is to set `ColorResourceBarrier=4` from `OptiScaler.ini` or select `RENDER_TARGET` for `Color` at `Resource Barriers (Dx12)` from the in-game menu.

## Exposure Texture
* Sometimes games exposure texture format is not recognized by upscalers. 
* Most of the time manifests itself as crushed colors (especially in dark areas) or white screens instead of normal rendering with XeSS (like Jedi Survivor). 
---
<details>
<summary><b> Click here for screenshot examples </b></summary>

![exposure](https://github.com/optiscaler/OptiScaler/blob/master/images/exposure.png)<br>*Shadow of the Tomb Raider*

</details>

---
> [!NOTE]
> In most cases, enabling `AutoExposure=true` in `OptiScaler.ini` or selecting `Auto Exposure` in `Init Parameters` from the in-game menu should fix these issues.

## XeSS - black/garbled/white screen, crashes or degraded performance over time
* Some users have reported that when using XeSS upscaler backend, the result is a black/garbled screen with UI or crashes (in Guardians of the Galaxy e.g.). 

> [!NOTE]
> * Try **turning on Auto Exposure** if it's disabled as some games require AE enabled for XeSS to work properly.
> * If the game crashes with XeSS, or performance degrades over time (e.g. **GTX 1600 series**, [example](https://github.com/optiscaler/OptiScaler/issues/468#issuecomment-2935149409)), please try running game with these settings from ini

```ini
; Building pipeline for XeSS before init
; true or false - Default (auto) is true
BuildPipelines=false

; Creating heap objects for XeSS before init
; true or false - Default (auto) is false
CreateHeaps=false
```
* OUTDATED WITH NEWER XESS - In some cases downloading the latest version of [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler/releases) and extracting `dxcompiler.dll`, `dxil.dll` from `bin\x64\` next to the game exe file resolved this issue.

## Intel Arc spoofing issues
* Intel Arc GPUs seem to present weird colour "rainbow" artifacts in some games when spoofing to Nvidia (e.g. Robocop Rogue City, STALKER 2). 
* Also check [below](#graphical-corruption-and-crashes) for more UE5 issues
---
<details>
<summary><b> Click here for screenshot examples </b></summary>

![Intel Arc spoofing issue](https://github.com/user-attachments/assets/aa7e5660-14ae-488f-9dde-eb711908648f)


</details>

---
> [!NOTE]
> **Solution** 
> * Either disable spoofing with `Dxgi=false` in OptiScaler.ini and use FSR or XeSS inputs
> * Or use [OptiPatcher](https://github.com/optiscaler/OptiPatcher) if that game is supported

## Performance Issues
* In general XeSS is heavier than FSR for GPUs, so it's expected to be have lower performance even on Intel Arc GPUs.
* As a result of spoofing to an Nvidia card to enable DLSS, some games would use an Nvidia-optimized codepath which may result in lower performance on other GPUs.

## Display Resolution Motion Vectors
* Sometimes games would set the wrong `DisplayResolution` init flag, resulting in excessive motion blur. 
* Setting or resetting `DisplayResolution` would help resolve this issue.
---
<details>
<summary><b> Click here for screenshot examples </b></summary>

![mv wrong](https://github.com/user-attachments/assets/e10f3e2e-c2a8-4167-a474-8a128e8870dd)<br>*Deep Rock Galactic*

</details>

---

## Graphical Corruption and Crashes
* As mentioned above, spoofing an Nvidia card can cause games to use special codepaths that can cause graphichal corruptions. 
* Screenshot below is from a pre-UE 5.3 game where these blue texture corruptions used to be seen - The Talos Principle 2, Black Myth Wukong, Ranch Simulator...
> [!NOTE]
> * If possible disable spoofing and use FSR or XeSS inputs on these situations, or use [OptiPatcher](https://github.com/optiscaler/OptiPatcher) on supported games.
---
<details>
<summary><b> Click here for screenshot examples </b></summary>

![talos principle 2](https://github.com/optiscaler/OptiScaler/blob/master/images/talos.png)<br>*Talos Principle 2*

</details>

---
* And crashes, especially when raytracing is enabled.

## Shader Compilation error on Linux
* If you are using OptiScaler with Linux and you have problems with `RCAS`, `Reactive Mask Bias` or `Output Scaling`, you will probably notice this message in your logs.
```
CompileShader error compiling shader : <anonymous>:83:26: E5005: Function "rcp" is not defined.
```
> [!NOTE]
> * To solve this problem you can use `Precompiled Shaders` option from menu or install `d3dcompiler_47` with `WineTricks`/`ProtonTricks`. OptiScaler uses custom shaders for these features and depends on this compiler file to compile these shaders at runtime. 

## DirectX 11 with DirectX 12 Upscalers
* These implementations use a background DirectX 12 device (D3D11on12) to be able to use DirectX 12-only upscalers. There is an up-to 10-15% performance penalty for this method, but allows many more upscaler options. 

## Legacy In-game Menu
* Please try opening menu while you are in-game (while 3D rendering is happening)

* On some system and game combinations, opening the old in-game menu may cause the game to crash or cause graphical corruption (especially in Unreal Engine 5 games).

<details>
<summary><b> Click here for screenshot examples </b></summary>

![Banishers](https://github.com/optiscaler/OptiScaler/blob/master/images/banishers.png)<br>*Banishers: Ghosts of New Eden*

</details>

* In games that use Unity Engine, legacy in-game menu will be upside down.

<details>
<summary><b> Click here for screenshot examples </b></summary>

![barrel roll](https://github.com/optiscaler/OptiScaler/blob/master/images/upsidedown.png)<br>*Sons of Forest*

</details>
