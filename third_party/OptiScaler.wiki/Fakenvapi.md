## Adding support for Fakenvapi

**0.** **Do not use with Nvidia**, only required for AMD/Intel  

**1.** Download the mod - [**Fakenvapi**](https://github.com/FakeMichau/fakenvapi/releases/latest)  

**2.** Extract the files and transfer `nvapi64.dll` and `fakenvapi.ini` to the same folder as Optiscaler (by the main game exe)   

***

* To enable Fakenvapi's conversion of Reflex-to-AL2/LFX, **Reflex** must be **enabled** in game settings. Since DLSS-FG incorporates Reflex, enabling DLSS-FG automatically enables Reflex. In the case you don't need FG, and the game doesn't expose Reflex on its own, it's possible to force Reflex with `force_reflex=2` in `fakenvapi.ini`.

_**Anti-Lag 2** only supports RDNA cards and is Windows only atm (shortcut for cycling the overlay - `Alt+Shift+L`). For information on how to verify if Anti-Lag 2 is working, please check [Anti-Lag 2 SDK](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK?tab=readme-ov-file#testing)._   
_**Latency Flex** is cross-vendor and cross-platform, can be used as an alternative if AL2 isn't working (if you suffer from low fps stutters, **LFX** might be at fault - set `force_reflex=1` in `fakenvapi.ini` to disable)_ 

* [In-game example of AL2 overlay](#in-game-working-example-of-al2)

---
> [!NOTE]
> ### _Example of correct installation (with additional Nukem mod)_
> ![Installation](https://github.com/user-attachments/assets/2ea9b2da-04fe-4a5e-ad69-9bd666768c65)

> [!TIP]
> ### In-game working example of AL2
> * **Reflex** needs to be **enabled** in game settings (DLSS FG automatically activates it)
> * Overlay in the top left is the **AL2 overlay** which is possible to cycle with `Alt+Shift+L`
> * Some games have AL2 enabled on RDNA GPUs at all times with no in-game AL2 toggle: e.g. Cyberpunk 2077 as of Patch 2.3.0
> * Some games have **Reflex** enabled at all times with no in-game Reflex toggle, so AL2 gets enabled automatically: e.g. Alan Wake 2
>
> ![The Witcher 3 Wild Hunt_2025 04 18-20 34](https://github.com/user-attachments/assets/89a3807a-dbf1-4271-8b75-c824759a5013)




