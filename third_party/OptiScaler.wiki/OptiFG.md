## OptiFG (powered by FSR3 FG) + HUDfix (experimental HUD ghosting fix) 
* **OptiFG** was added with **v0.7**  
* **Only supported in DX12**  
* **Experimental way of adding FSR3-FG to games without native Frame Generation**, can also be used as a last case scenario if native FG is not working properly  
* _If the game has **native DLSS-FG**, it's recommended to rather use_ [_**Nukem's dlssg-to-fsr3 mod**_](https://github.com/optiscaler/OptiScaler/wiki/Nukem's-dlssg%E2%80%90to%E2%80%90fsr3)

---

> [!TIP]
> **How it works?**
> * Since FSR3-FG doesn't support HUD interpolation, it requires a HUDless resource to avoid HUD ghosting/garbling.  
> * In games without native FG, Optiscaler tries finding the HUDless resource when the user **enables HUDfix**.  
> * Depending on how the game draws its UI/HUD, Optiscaler may or may not be successful in finding the HUDless resource.  

> [!WARNING]
> * _Since OptiFG is **not a "native" solution** and is _hacking-in/forcing_ FSR FG, you can expect issues such as games hanging when exiting, game crashing on boot, game crashing after toggling FG Active, crashing after a while of using OptiFG, not all things getting interpolated properly..._  
> * **OptiFG will not work with XeSS inputs in Unreal Engine games (no depth provided)!**  
> * _OptiFG has gone through many modifications, but due to its **experimental nature**, it will always be possibly crash-prone. But when it works, it just works™._  

> [!NOTE]
> **IMPORTANT NOTES**
> * Due to compatibility issues, Optiscaler automatically disables overlays when OptiFG is enabled (Steam, RTSS, Ubisoft, EA App, Overwolf).  
> * _It seems **Steam Input** may get blocked too which can mess with controller layouts. If you're having **issues with your controller**, try setting `DisableOverlays=false` in `Optiscaler.ini`._  
> * While **RTSS** doesn't get blocked, it's **not recommended** to have it enabled as it can increase the possibility of crashes.  
> * _If you want to try using **RTSS** (MSI Afterburner, CapFrameX), please **enable Use MS Detours API hooking** in RTSS Setup/Settings (and try updating RTSS also if possible)_.  
> ![image](https://github.com/optiscaler/OptiScaler/assets/35529761/8afb24ac-662a-40ae-a97c-837369e03fc7)

---

> [!TIP]
> **0.** Before starting with OptiFG, please check the [OptiFG HUDfix incompatible games list](https://github.com/optiscaler/OptiScaler/wiki/Hudfix-incompatible) where OptiFG won't work properly, meaning there will be HUD ghosting or just duplicated frames.

### **1.** To enable **OptiFG**, set `FGType=optifg` in `Optiscaler.ini`  
  
![optifg ini](https://github.com/user-attachments/assets/e61c1298-f4bf-4aa5-9f60-5ff7c6d5f36b)


### **2.** After selecting the desired upscaler in Opti Overlay, **enable `FG Active` and `FG Debug View`** 
* Should look like below picture  
* _**Notice how all of the fields are active, except the bottom right (HUDless resource).**_  
* _FG Debug View fields legend if interested - [link](https://github.com/user-attachments/assets/5df8bf33-0f66-4515-978a-72acf75ca888)_

![this](https://github.com/user-attachments/assets/634e2abd-61bf-4ef7-a80b-ae54c3cfa564)  


### **3.** **Now enable `FG HUD Fix`** 
* All of the debug view fields need to be active now, but the main focus are the bottom 2 images. 
* The goal - find the appropriate **limit** where the **bottom middle and bottom right images look completely same, BUT the bottom middle one must not have UI/HUD**.  
* Limit is usually in single digits (1-9). Unreal Engine games generally work the best with OptiFG.

> [!IMPORTANT]
> * If there are missing fields, or the bottom 2 images are not the same, or the bottom middle has UI, OptiFG will not work properly - FSR-FG will either not work, or will duplicate frames.  
> * _**More examples of working and not working limits are listed at the end.**_

> [!NOTE]
> * While FG Debug View is active, it will always show the base/real FPS (meaning half of FG).  
> * Only after disabling FG Debug View, the overlay will report post-FG fake frames.  
> * Optiscaler's performance overlay can always be invoked with **Page Up** and cycled with **Page Down**, or enabled from `FPS Overlay` submenu.  

Examples are  
![screenshot 1](https://github.com/user-attachments/assets/f23d895f-806b-45b2-8e0c-2d452f0876f4)  
or  
![screenshot 2](https://github.com/user-attachments/assets/e2d503f7-50a2-482a-8c73-402f49a96c9d)  
or  
![hud](https://github.com/user-attachments/assets/01e885cd-0642-4413-8761-34dd10ba98f0)

  

### **4.** If you've managed to correctly set up HUDfix, you can also try enabling `FG Async` for extra performance.  
* Note - toggling `FG Async` while FG is active might cause crashes, in those cases try enabling it before enabling FG Active.  

### **5.** If HUD fix limit isn't able to find the correct images, you can try using `FG Immediate Capture` and `FG Extended` with Limit.  
* Previous rule of FG Debug View Fields still stands.  

> [!IMPORTANT]
> * If there are missing fields, or the bottom 2 images are not the same, or the bottom middle has UI, OptiFG will not work properly - FSR-FG will either not work, or will duplicate frames.  
> * _**More examples of working and not working limits are listed at the end.**_

### **6.** `FG Scale Depth to fix DLSS RR` - specific setting for DLSS (4) games when using DLSS Ray Reconstruction along with OptiFG.  
* New DLSS dlls (310+) have introduced some issues where OptiFG can't properly detect depth and interpolation doesn't work properly.  
* Enable this option if the top middle field (green depth) is missing outlines and is solid green.

---

## Examples of properly working HUDfix

* [Palworld](https://github.com/user-attachments/assets/d566e495-d761-492b-b80e-2a15745aab0e)  
* [HZD Remastered](https://github.com/user-attachments/assets/ac5d9700-6e84-4d4b-95c1-8b254a9d20a8)
* [Midnight Suns](https://github.com/user-attachments/assets/d42249de-b5eb-40eb-9d36-8145e3e2f021)  
* [Path of Exile 2](https://github.com/user-attachments/assets/0ac48ff8-2547-4bc8-9dca-5d2ed89c0229)  
* [Thymesia](https://github.com/user-attachments/assets/7a46b879-8c03-4ac8-9b8e-c1ef074d68ee)  




## Examples of incorrect HUDfix values

* [AC Mirage](https://github.com/user-attachments/assets/ed6a603f-7727-4c75-8a1d-808b0fd2ab61) - HUDfix incompatible  
* [Dying Light 2](https://github.com/user-attachments/assets/fb502e30-b662-4f4c-afc2-d84819acea7d) - HUDfix incompatible  
* [MH Rise](https://github.com/user-attachments/assets/4c28df3a-ad9d-4c0e-978c-c84be5f81595)  
* [FF16](https://github.com/user-attachments/assets/c359059a-c1a6-4374-bd7f-62b52a3c40da)  
* [Misconfigured PoE2](https://github.com/user-attachments/assets/923f7190-f8d7-43c4-a15b-0c27b3ca7f9a)  

> [!NOTE]
> * In these examples, you can see how the **bottom middle field** has different issues - wrong colours, different tonemapping, incorrect textures. All of which renders FSR-FG unuseable. 
> * Sometimes these issues can be worked around by using default brightness/gama, disabling HDR, but for a sizeable number of games nothing worked so far, and are listed on the HUDfix incompatible games list.
