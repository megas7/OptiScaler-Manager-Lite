> [!IMPORTANT]
> FSR4 officially supported on **RDNA4 GPUs only** (9060, 9060 XT, 9070 GRE, 9070 and 9070 XT)
>
> * FSR4 **doesn’t support Vulkan** yet!
> * FSR4 should work on **DX11** when using **FSR3.X/4 w/Dx12** - [UE DX11 note](#fsr4-shimmering-on-unreal-engine-dx11)
> * FSR4 doesn't seem to like **Ultra Quality** preset in general, best to avoid it
> * **Windows 10** might need Agility SDK, check [here](#directx-12-agility-sdk-for-windows-10) 
> ---
> * Please check the [general compatibility list](https://github.com/optiscaler/OptiScaler/wiki/Compatibility-List) first for already documented fixes and workarounds  
> * If the game doesn’t expose DLSS inputs, most likely it requires [Fakenvapi](Fakenvapi)  
> * **Linux** devs managed to get FSR4 FP8 working on RDNA3 – performance overhead explains why no official support (yet?) 

### TEST EXPERIMENTAL BUILDS
> [!TIP]
> * **0.7.9** was released, there are no newer Nightlies at the moment
> <details>
>  <summary><b>Click here for more info</b></summary>  
>  
> * ~While the new stable build is taking a while to get released, everyone can try the nightly test builds (mentioned as Experimental in the table)~  
> * Can try it here – [**Nightly builds**](https://github.com/optiscaler/OptiScaler/releases/tag/nightly)  
>  
> </details>  

### FSR4 DLL Autolocating
> [!NOTE]
> OptiScaler will try to automatically locate `amdxcffx64.dll` from Windows folder. If it fails, you’ll need to search for `amdxcffx64.dll` in your `Windows` folder or download it from below and copy it next to OptiScaler.  
> * [**FSR 4.0.1 – 1.0.0.40394**](https://github.com/user-attachments/files/19853595/FSR4.0.1_1.0.0.40394.zip)  
> * [FSR 4.0.0 – 1.0.0.39849](https://github.com/user-attachments/files/20740129/FSR.4.0.0.-.1.0.0.39849.zip)  

---

### FSR4 Linux Setup
> [!NOTE]
> <details>
>  <summary><b>Click here for more info</b></summary>
>  
> * _**FSR 4.0.2 requires [Proton-EM 10.0-2D](https://github.com/Etaash-mathamsetty/Proton/releases/tag/EM-10.0-2D) or any other newer which supports FSR 4.0.2/FFX SDK 2.0.0 in order to work**_  
> * FFX SDK 2.0 (which Opti 0.7.9 uses) now comes with the Upscaler dll which contains built-in FSR 4.0.2 also. To downgrade back to FSR 4.0.0, you'll have to just replace the AMD files with the old ones from FFX SDK 1.1.4/Opti 0.7.8. 
>  
> Setup guide is for Linux Proton with FSR4 
> * Use [ProtonUp-QT](https://github.com/DavidoTek/ProtonUp-Qt) or similar to download and install a supported version of proton for steam games. When installing a new version with steam make sure that steam is closed or restart steam to access the installed version.
> * Grab release [Proton-EM](https://github.com/Etaash-mathamsetty/Proton) 10.0-2D, or newer OR [Proton-GE](https://github.com/GloriousEggroll/proton-ge-custom) 10.4, OR [proton-cachyos](https://github.com/CachyOS/proton-cachyos) 10.0-20250623, OR Steam Proton Experimental Bleeding Edge
> * Extract the proton into `~/.local/share/Steam/compatibilitytools.d/`, then either set this proton globally in steam as default or manually set game to use it
> * Env Vars/Settings (e.g Steam Launch Options) <br>
    RDNA4: `PROTON_FSR4_UPGRADE=1` <br>
    RDNA3: `PROTON_FSR4_UPGRADE=1` + `DXIL_SPIRV_CONFIG=wmma_rdna3_workaround`, and in OptiScaler.ini set`Fsr4Update=true`
> * Make sure to be on Mesa 25.2.0 or newer
> * Setup OptiScaler as usual, and all should work :)
> 
> </details>

---

### DirectX 12 Agility SDK for Windows 10
> [!NOTE]  
> [**PotatoOfDoom**](https://github.com/PotatoOfDoom) added experimental support for updating the DirectX 12 Agility SDK which should help games crashing on Windows 10.  
> To make it work, copy `D3D12_Optiscaler` folder from OptiScaler archive to games exe folder and set `FsrAgilitySDKUpgrade=true` in **OptiScaler.ini**

### RDNA4 Detection
> [!NOTE]  
> FSR4 will be automatically enabled when OptiScaler detects an RDNA4 card.  
> If detection fails, FSR4 can be manually enabled by setting `Fsr4Update=true` in OptiScaler.ini file.

### Image Quality
> [!NOTE]  
> **If you have image quality issues, please try changing `Non-linear sRGB`& `Non-linear PQ` settings**  
> * FSR4 doesn't officially support **Ultra Quality (UQ)** preset - some games exhibit either broken rendering or white screen flashes
>   * Solution - either try the `Non-Linear colour space` toggle, or stick to **Quality/Native AA**
> 
> ![image](https://github.com/user-attachments/assets/6328dd19-274f-4be1-a2b5-acaea8cba102)
> * **RDNA3 on Linux** will have broken visuals unless this command is set: `DXIL_SPIRV_CONFIG=wmma_rdna3_workaround`

### Weird black artifacts with FSR 4.0.1 & 4.0.2 in some games
> [!NOTE]  
> ![image](https://github.com/user-attachments/assets/7fc9e247-94cf-4e5f-a2a3-e7df07b37ee0)
> <details>
>  <summary><b>Click here for more info</b></summary>
>  
> * **Presented with weird black artifacts on the top and bottom of your screen during movement, as shown in the image**  
> * Seems to happen on some games when FSR 4.0.1 uses a preset ratio above Quality (1.5), meaning Native AA (1.0) or Ultra Quality (1.3) may cause this issue.  
> * Affected games – Horizon Forbidden West, Cyberpunk 2077, Stalker 2, Star Wars Jedi: Survivor.  
> * Not present in God of War Ragnarok  
> 
> **„FIX“** – either try switching to a lower preset like Quality/Balanced, or download the FSR4.0.0 `amdxcffx64.dll` and place it in the same folder as `OptiScaler.dll`. OptiScaler will use the local 4.0.0 DLL instead of the global driver 4.0.1 for that particular game.  
 
>  
> </details>

### FSR4 shimmering on Unreal Engine DX11 
> [!NOTE]
> <details>
>  <summary><b>Click here for more info</b></summary>
> 
> * Shimmering seems to be related to **enabled Dilated Motion Vectors** when using **DLSS inputs** in **DX11**  
> * Using the **Engine.ini command** to disable Dilated MVs fixes the issue  
> 
>   * Edit game's `Engine.ini` and add
> ```ini
> [SystemSettings]
> r.NGX.DLSS.DilateMotionVectors=0  
> ```
> * Check PCGamingWiki for config file location
> * If the **Engine.ini doesn't exist**, you'll have to create it and possibly make it Read-Only if the game deletes it on boot
> ---
> * **If you're using/forcing Native AA** (1.0 scaling), you can just **disable Dilated MV** under `Advanced Init Flags` in OptiScaler overlay without editing the Engine.ini
>   * This **_won't work for upscaling presets_** (e.g. Quality), as motion vectors will be broken due to mismatched resolution
> 
> </details>

---

> [!IMPORTANT]
>
>### Editing the Compatibility list 
> When adding a game – please add _**game name**_, working or not working _**checkmark**_, _**inputs used**_, any _**note**_ regarding what you used, and a _**game screenshot**_ with the open Opti overlay (convert to jpeg to save bandwidth)  
> ${{\color{orangered}\{\textsf{Before saving the page, please add a message for the changelog at the bottom if adding a new game or screenshot. \}}}}\$

>
>  ### This is `not` a complete list of games supported by OptiScaler.

> [!TIP]  
> **Working** :muscle: – **401**  
> **Not working** :skull: – **13**  
> 
> _**ATM the only FSR4 games not working are either Anti-Cheat enabled or use Vulkan**_  
> 
> _Last updated – 5 October 2025_

<!--
TEMPLATE FOR NEW ENTRIES
| GAME NAME           | ✔️/❌ | ✅/⛔ | DLSS/FSR2/XeSS | Notes go here                | [#](url) |
-->

| Game | Win | Linux | Inputs | Notes   | Images | 
| ---- | :-: | :---: | :----: | ------- | :----: |
| [7 Days To Die](https://github.com/optiscaler/OptiScaler/wiki/7-Days-to-Die) | ✔️ | ✅ | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12, also make sure **EAC** is disabled. | [1](https://github.com/user-attachments/assets/da4e6e92-cb3b-47cb-b6fd-ffd0748ee0f5) <br>🐧[1](https://github.com/user-attachments/assets/40f725dc-81e2-472b-96ae-821fc7c32384) |
| Achilles: Legends Untold | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/f2c4a63e-b662-4216-9ed9-c33a2d321338) |
| Ad Infinitum | ✔️ | | DLSS | Create a shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/6ecb1c94-fbdb-40da-be48-5e9e7717930a)|
| [AI Limit](https://github.com/optiscaler/OptiScaler/wiki/AI-Limit) | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/e726d30e-8143-4edc-96f2-a7361dee78cf) |
| AirportSim | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/f1f42cda-38e3-46ba-98bc-aa04d9a1a5ca)|
| Akimbot | ✔️ |  | DLSS | Both TAA and DLSS must be enabled in game settings | [1](https://github.com/user-attachments/assets/66920e67-7064-49f1-9254-0c7d2c67e1d1)|
| Alan Wake Remastered | ✔️ | ✅ | DLSS | Game suffers from blacks screens and vertex explosions on AMD Zen3 and newer CPUs, it’s fine on Intel. IT IS NOT an OptiScaler problem. | [1](https://github.com/user-attachments/assets/bbb496ec-fcee-494e-b9b9-68b053495696) |
| [Alan Wake 2](https://github.com/optiscaler/OptiScaler/wiki/Alan-Wake-II) |  ✔️ | ✅ | DLSS | [Fakenvapi](Fakenvapi) is required, crashes on Win 10 fixed with `FsrAgilitySDKUpgrade=true`. | [1](https://github.com/user-attachments/assets/a6072762-3068-4403-83f6-c8024f1324a4) [2](https://github.com/user-attachments/assets/bc844918-78a9-48f1-b6ad-fce85d61ca1c) |
| [Alien: Rogue Incursion Evolved Edition](https://github.com/optiscaler/OptiScaler/wiki/Alien-Rogue-Incursion-Evolved-Edition) | ✔️ |  | DLSS, FSR3.1 |  | [1](https://github.com/user-attachments/assets/fe4dcef1-33d5-4860-8358-37621aacc8c6)|
| Alone in the Dark (2024) | ✔️ |  | DLSS  | Launch executable with `-dx12` [launch option](https://github.com/user-attachments/assets/ac0dd6de-4f34-4f4b-bdf8-7e2a12eb347b), Opti does not enable FSR4 until DLSS mode is changed and applied in game settings. | [1](https://github.com/user-attachments/assets/5466b801-9c87-4894-857d-514d31c349de)|
| Ambulance Life: A Paramedic Simulator | ✔️ | | DLSS, XeSS | | [1](https://github.com/user-attachments/assets/6353b20e-6362-4c16-93cd-eb361522e749) |
| Amid Evil | ✔️ | ✅ | DLSS | Linux: use `dxgi.dll`, hudfix=true. | 🐧[1](https://github.com/user-attachments/assets/e24dd648-a7bd-4449-b488-95b8bd8b891c) |
| [A Plague Tale: Requiem](https://github.com/optiscaler/OptiScaler/wiki/A-Plague-Tale-Requiem) | ✔️ | ✅ | DLSS | Use [Fakenvapi](Fakenvapi), with [Nukem’s](Nukem's-dlssg‐to‐fsr3) FG HUD will ghost due to no HUDless | [1](https://github.com/user-attachments/assets/83c3ee2d-0879-46a3-be14-d4e54b55c307) |
| Apocalypse 2.0 | ✔️ | | DLSS | Disable Display Res.MV to fix shimmer issues| [1](https://github.com/user-attachments/assets/42efc3aa-d0eb-445d-860f-8c6114b1f995)|
| A Quiet Place: The Road Ahead | ✔️  | ✅ |  DLSS | **Officially supported game**. Use OptiScaler as `winmm.dll`. | [1](https://github.com/user-attachments/assets/565ba961-fbf5-4542-8787-76b8b0e9823b) 🐧[1](https://github.com/user-attachments/assets/d26b051b-aaac-48ea-a91c-f08d63ac90c2) |
| Arcadegeddon | ✔️ | | DLSS, XESS | Free online co-op game, no anticheat | [1](https://github.com/user-attachments/assets/782e2869-b7d6-490b-a70a-871fe5f74675) [2](https://github.com/user-attachments/assets/373a887d-ab8c-4f07-87bb-af322bd74b44)|
| Ark: Survival Ascended |  ✔️  |  | DLSS | **Officially supported game**. FG doesn’t work. | [1](https://github.com/user-attachments/assets/4bd67cd4-9559-400e-b67d-6be8a32a9c1d) |  
| Arons Adventure | ✔️ |  | DLSS, FSR2 | | [1](https://github.com/user-attachments/assets/d1116d3b-61e4-4a31-bd66-7010c555e6d4)|
| ASKA | ✔️ | ✅ | DLSS, FSR |  **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/374bfd3d-c5e9-4d2c-9d06-2ad327160aa4) <br>🐧[1](https://github.com/user-attachments/assets/5865a7a3-cd82-4ea8-8f66-fb3dfbcf3eae) |
| Assetto Corsa Competizione | ✔️  | ✅ | DLSS | Use `-dx12` to force Dx12 mode, default DX11 mode also works |  [1](https://github.com/user-attachments/assets/6e6c391d-4971-4c6d-b692-c3d91a58aa1b) [2](https://github.com/user-attachments/assets/25eff92c-505f-4068-96c7-a15e30bcd5ab) <br>🐧[1](https://github.com/user-attachments/assets/bf621b61-704a-437f-8773-e316aea8d81f) |
| Assetto Corsa EVO | ✔️  |  | DLSS |  | [1](https://github.com/user-attachments/assets/6f440340-b829-4502-b982-06467f37b35b) |
| Asterigos: Curse of the Stars | ✔️ | ✅ | FSR2 | Looks suprisingly good for FSR2 input | 🐧[1](https://github.com/user-attachments/assets/ceebbada-ce42-4cff-865c-672a4f13c7eb) |
| [Assassin’s Creed Mirage](https://github.com/optiscaler/OptiScaler/wiki/Assassin%E2%80%99s-Creed-Mirage) |  ✔️  |  | DLSS, XeSS |   | 
| [Assassin’s Creed Shadows](https://github.com/optiscaler/OptiScaler/wiki/Assassin%E2%80%99s-Creed-Shadows) |  ✔️  |  | DLSS |  **Officially supported game**. Need to disable Ubisoft overlay. Click on your profile in the Ubisoft app, than settings, interface and uncheck `Enable overlay in the compatible games`. DLSS inputs and [Nukem’s](Nukem's-dlssg‐to‐fsr3) FG works without issues. Don’t use HDR, it causes extreme ghosting. | [1](https://github.com/user-attachments/assets/8917fa89-cb67-4e6e-9b96-402301c7ff89) |
| Atelier Yumia | ✔️ | | XESS | | [1](https://github.com/user-attachments/assets/888d2b59-b950-4450-9a5c-377edce8be0a) [2](https://github.com/user-attachments/assets/95294b65-a9ff-4d6d-a31a-fbae1d69f840) [3](https://github.com/user-attachments/assets/58e92ae9-6f15-4e5b-b020-1e3201052cce)|
| Atomic Heart |  ✔️  | ✅ | DLSS, FSR2 | Use `Fsr2Pattern=true` in the .ini for FSR2 inputs, if you cant turn FSR2 inputs on make sure ray tracing is off. | [1](https://github.com/user-attachments/assets/124b31af-8079-4397-abb0-b966772f729e) <br>🐧[1](https://github.com/user-attachments/assets/e59e1bf9-6c01-4160-89cb-f68142ae89d3) [2](https://github.com/user-attachments/assets/47974f31-b2fb-4aba-b171-1aece02c08f5) |
| Automate It : Factory Puzzle | ✔️  | | FSR3 | FSR needs setting to be changed for it to be detected, DLSS is not detected by Optiscaler | [1](https://github.com/user-attachments/assets/5ed624ee-2f7a-4abe-b7a0-d659e0ff35b5) |
| [Avatar: Frontiers of Pandora](https://github.com/optiscaler/OptiScaler/wiki/Avatar-Frontiers-of-Pandora) | ✔️ | | XeSS, DLSS | Disable spoofing (select „No“ during setup) and use XeSS inputs, spoofing seems to break interior ray tracing (too dark). | [1](https://github.com/user-attachments/assets/7f5468df-4ba7-4c36-a28d-100382997d17) [2](https://github.com/user-attachments/assets/8631587d-16ee-47df-97d4-2668747a35ec) |
| [Avowed](https://github.com/optiscaler/OptiScaler/wiki/Avowed) |  ✔️  |  | DLSS |   | [1](https://github.com/user-attachments/assets/8fd1e3f0-ede0-4590-8a06-bbe261b0786a) |
| Baldur’s Gate 3 |  ✔️  | ✅ |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/a4d12eb9-b1d5-4cd0-be19-10ebd10d4e9d) <br>🐧[1](https://github.com/user-attachments/assets/02f349fa-f3ea-450b-80e4-a8605f5cad22) |
| [Banishers: Ghosts of New Eden](https://github.com/optiscaler/OptiScaler/wiki/Banishers-Ghosts-of-New-Eden) |  ✔️  |  | DLSS, FSR2 | For FSR2 inputs please check [UE Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks) page | [1](https://github.com/user-attachments/assets/d008abc2-9efe-46aa-9612-c1cf151c6a7d) |
| Batora : Lost Haven |  ✔️  |  |  DLSS | Run in Dx12 mode | [1](https://github.com/user-attachments/assets/e694e6a2-49ab-490b-aadc-e334010d0aab) |
| Bears in Space | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/94ff3251-b5f6-401e-9cce-c215af92e3d9)|
| BEAST: False Prophet | ✔️ || DLSS || [1](https://github.com/user-attachments/assets/9b713ba1-c655-4a06-a1a5-b7a248624c2a) |
| Beyond Hanwell | ✔️ | | DLSS, FSR3.1, XESS | | [1](https://github.com/user-attachments/assets/1fc55ad4-7ff7-4823-a6cf-67affd8d9a02) [2](https://github.com/user-attachments/assets/ff5568e9-6a76-4674-8969-c4959034dc94)|
| [Black Myth: Wukong](https://github.com/optiscaler/OptiScaler/wiki/Black-Myth-Wukong) |  ✔️  |  | FSR3, XeSS  | | [1](https://github.com/user-attachments/assets/834b3d00-0e06-40be-bea0-df924ead47c9)
| Blacktail |  ✔️  |  | DLSS | Run Dx12 version, use [Fakenvapi](Fakenvapi), also requires a copy of `nvngx_dlss.dll` in<br>`.\BLACKTAIL\BLACKTAIL\Binaries\Win64`. | [1](https://github.com/user-attachments/assets/a3b0aee1-374d-4096-ad55-f0c10e8b7ca1) |
| Blades of Fire |  ✔️  |  | DLSS | **Officially supported game**. Use keyboard to navigate through Opti overlay, mouse doesn’t work. | [1](https://github.com/user-attachments/assets/b34f676a-b32b-40eb-8fb3-de8d148f258c) |
| Bleak Faith: Forsaken |  ✔️  |  | DLSS  | Launch game with `-dx12` parameter  | [1](https://github.com/user-attachments/assets/84936a42-aa0a-4c32-a5df-f63a904b59fb) |
| Blind Fate: Edo no Yami | ✔️| | DLSS | | [1](https://github.com/user-attachments/assets/02207259-81f7-416a-8004-b89d8ce63a39)|
| Bloodhound | ✔️ | | DLSS, FSR3 | Create a shortcut with `-dx12` launch parameter |[1](https://github.com/user-attachments/assets/93a83d27-5fb3-4563-a6f6-10c9a149bda1)|
| [Borderlands 4](https://github.com/optiscaler/OptiScaler/wiki/Borderlands-4) | ✔️ |  | DLSS, FSR3, XeSS | spoofing causes a big performance overhead, so recommended to use FSR or XeSS inputs. Reflex works without spoof | [1](https://github.com/user-attachments/assets/923526e7-bc57-4369-8b27-c7e759c02550) |
| Break Arts III | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/12a47bc0-2a2a-4185-8eb6-45cdd221c5d5) |
| Bright Memory | ✔️ | ✅ | DLSS | Launch game with `-dx12`. DLSS inputs are only enabled if RT is also enabled. Requires Nvidia spoof to expose DLSS and „RTX“ options. | 🐧[1](https://github.com/user-attachments/assets/d73d5495-214d-4e81-8879-9efa574b588f)|
| Bright Memory: Infinite |  ✔️  | ✅ | DLSS, FSR | Launch game with `-dx12` | [1](https://github.com/user-attachments/assets/0854c111-4732-4305-99ce-91247b2c6c53) [2](https://github.com/user-attachments/assets/2455d4c1-e865-40c6-8b8c-e68a789f311d) |
| Broken Pieces | ✔️ |  | DLSS | Create a shortcut and add a `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/8fe2f502-7041-4f20-8e1f-f3c3d2203d34)|
| Brothers: A Tale of Two Sons Remake | ✔️ | |  DLSS | Use [Non-Linear sRGB Input](#Image-Quality) to fix screen flicker | [1](https://github.com/user-attachments/assets/cc6978e6-57e9-4c6e-b792-363b7dc6ee4f)|
| Burning Sword: Death | ✔️ | |  FSR3.1 | | [1](https://github.com/user-attachments/assets/3ff6dab6-ddab-418e-8d5f-c1cdcaa4e848) |
| Bus Simulator 21 |  ✔️  |  | DLSS | Run Dx12 version, copy OptiScaler files, [Fakenvapi](Fakenvapi) and `nvngx_dlss.dll` into `\Bus Simulator 21\BusSimulator21\Binaries\Win64` | [1](https://github.com/user-attachments/assets/95f1e56d-6fe8-446d-ae1f-f030517c826c) |
| [The Callisto Protocol](https://github.com/optiscaler/OptiScaler/wiki/The-Callisto-Protocol) | ✔️  |  | FSR2 | Check [wiki page](https://github.com/optiscaler/OptiScaler/wiki/The-Callisto-Protocol) for setup | [1](https://github.com/user-attachments/assets/479f7c3c-36fd-432f-a307-b4824f51c8de) [2](https://github.com/user-attachments/assets/6b08618e-ff60-4b1f-998a-13e311a7b33a) |  
| Capes | ✔️ |  | DLSS | Create a shortcut and add a `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/af1a8338-3772-435f-b0fd-66edf7906af9)|
| Car Dealer Simulator | ✔️ | | FSR, DLSS | Create a shortcut and add a `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/5daaf8b5-a20e-4395-b8a3-91051984971d)|
| Caravan SandWitch | ✔️ | ✅ | FSR3.1 | Crashes without [Unreal Engine Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks) for FSR3.1 | 🐧[1](https://github.com/user-attachments/assets/5228d065-4ce0-4772-a95c-6f2eb60d1031) |
| Chernobylite | ✔️ |  | DLSS | Run with `-dx12` launch option | [1](https://github.com/user-attachments/assets/a7c54e74-3f68-4280-87d6-0f9646490efc)|
| Chernobylite 2: Exclusion Zone | ✔️ | | DLSS, FSR3.1, XESS | Cinematic Reflections can cause sparkling artifacts on some surfaces, reduce to Ultra or lower setting | [1](https://github.com/user-attachments/assets/2b549a77-337c-4c96-978c-5faab88b72ac) [2](https://github.com/user-attachments/assets/fd69f319-b79d-4165-8ec1-5dc13405798c)|
| Choo Choo Charles | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/267ebae4-16bb-426b-8096-ceafe51d90af)|
| Chorus | ✔️ |  | DLSS |  | [1](https://github.com/user-attachments/assets/8a21a54e-ee8a-457c-8e01-4411e48c357b) |
| Chronicles of Sagrea | ✔️ | | FSR2, DLSS| |[1](https://github.com/user-attachments/assets/9b8d5331-4caa-4c60-947a-3508cb4fb997) |
| Cions of Vega | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter, Dx11 mode has shimmer | [1](https://github.com/user-attachments/assets/2b5ff056-f8ad-4fd0-b2a9-692d1d13e299)|
| Cities: Skylines II |  ✔️  |  | DLSS  |  Crashes with default Dx11. Launch with `-force-d3d12` parameter. Due to the Unity engine, it is necessary to check whether everything is rendered correctly. |  
| City of Springs | ✔️ | | DLSS, XESS | | [1](https://github.com/user-attachments/assets/ea81801f-7824-425c-acc4-5a4b7e2d0ca7)|
| [Clair Obscur: Expedition 33](https://github.com/optiscaler/OptiScaler/wiki/Clair-Obscur-Expedition-33) |  ✔️  | ✅ | DLSS, XeSS | Avoid using Ultra Quality preset, broken rendering. Running in full-screen instead of borderless windowed mode may also help prevent crashes. If cutscenes flicker with FSR4, try using `Non-Linear or sRGB option`. | [1](https://github.com/user-attachments/assets/28f9f8cf-562c-4e84-8031-df6b0eccfdcc) <br>🐧[1](https://github.com/user-attachments/assets/0ab7d0a4-b389-4196-a9a1-269ff0290f37)|
| Clash: Artifacts of Chaos | ✔️  |  | FSR | Run with `-dx12` launch option   |  [1](https://github.com/user-attachments/assets/0d4f87ad-e28f-46e5-9977-3e6e089b2fe4) |
| Commandos: Origins | ✔️ | | FSR, DLSS | | [1](https://github.com/user-attachments/assets/1a1de561-5b97-4de8-9a27-958e96484661) [2](https://github.com/user-attachments/assets/ac2e13f9-ec7d-4ae0-9787-0576aa59e8dd)|
| Company of Heroes 3 | ✔️ | | FSR2 | | [1](https://github.com/user-attachments/assets/a98b1dd0-1792-47d1-949a-f53c593771cf)|
| [Control](https://github.com/optiscaler/OptiScaler/wiki/Control-Ultimate-Edition)  |  ✔️  | ✅ | DLSS | | [1](https://github.com/user-attachments/assets/949e99eb-974b-4ee2-b2d8-f38381667bdb) [2](https://github.com/user-attachments/assets/6b5f0c32-e6a8-47f6-b47f-3c945cfed73e) <br>🐧[1](https://github.com/user-attachments/assets/88703be5-6f25-4527-9fb1-7804cd88d846)  |
| Creatures of Ava | ✔️ | | FSR3.1, DLSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/38bab100-3496-460d-9bbc-49cfc7b6215f) [2](https://github.com/user-attachments/assets/3e11e934-4029-4ad3-a3e1-f610c84214d1) [3](https://github.com/user-attachments/assets/17c69984-941c-49fa-9166-84bff691b31f)|
| Crime Boss : Rockay City | ✔️ | | DLSS, XESS | Online co-op game, does not use anticheat | [1](https://github.com/user-attachments/assets/4594e508-21d5-469d-b86c-039d1914e7b9) [2](https://github.com/user-attachments/assets/e1b81a4a-6fe3-453b-bdf4-a14f90516d64) |
| Crisol: Theater of Idols (Demo) | ✔️ |  | DLSS, XeSS |  | [1](https://github.com/user-attachments/assets/beab5c55-52ad-4d1b-8efd-d99b9aaae73f) |
| [Cronos: The New Dawn](https://github.com/optiscaler/OptiScaler/wiki/Cronos-The-New-Dawn) | ✔️ | ✅ | DLSS, FSR3.1, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/56b5af85-8db0-4ac1-86bb-54656aeced10)|
| Crysis Remastered | ✔️ | ✅ | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/4ac94e13-eb44-468a-8545-331ac3d709ca) <br>🐧[1](https://github.com/user-attachments/assets/a0196327-c582-438c-a927-e0f221b8a2cb) |
| Crysis 2 Remastered | ✔️ |  | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/5d7bcb18-cd7a-4bf4-9b94-f7140c92d50b) |
| [Crysis 3 Remastered](https://github.com/optiscaler/OptiScaler/wiki/Crysis-3-Remastered) | ✔️ |  | DLSS | **Dx11 game**, use FSR3 w/Dx12. | [1](https://github.com/user-attachments/assets/3a411e35-874d-4ef5-92be-36238b04a087) |
| Cyberpunk 2077 |  ✔️ | ✅ | FSR3, XeSS, DLSS, FSR4  | **Officially supported game**.<br>On Windows 10, [FsrAgilitySDKUpgrade](#directx-12-agility-sdk-for-windows-10) is required. If FSR3 inputs aren’t working, use XeSS inputs. When using [Nukem’s](Nukem's-dlssg‐to‐fsr3) FG, please restart the game after enabling DLSS-FG in-game to apply the changes.<br>On Linux, DLSS inputs work fine. FSR4 in-game is a preset that can be exposed in-game from game ini when using Opti, regardless of the GPU hardware. It’s using FFX inputs (FSR3.1) – can also be used as an upscaler input for FSR3.1/XeSS/FSR4. | [1](https://github.com/user-attachments/assets/1fdb323e-7979-4338-a51b-53e8fa45347c) <br>🐧[1](https://github.com/user-attachments/assets/d1f01314-78aa-495a-8c40-2a5735e9e23a)|
| Daemon x Machina Titanic Scion | ✔️ | | DLSS | Use [Non-Linear sRGB Input](#Image-Quality) to fix screen flicker if present | [1](https://github.com/user-attachments/assets/9398300e-5059-4820-baa8-57f3c3139ffe), [2](https://github.com/user-attachments/assets/be17aa1f-47e7-4c2d-84f7-2ce4e28550c1) |
| Dakar Desert Rally | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/bea14cc6-6766-4587-b8f3-8c442cf32517) |
| Daydream: Forgotten Sorrow | ✔️ | |  DLSS | | [1](https://github.com/user-attachments/assets/fa45b692-3264-47ec-ba19-c10fa692066c)|
| Daymare - 1994 Sandcastle | ✔️ | | DLSS, XESS | | [1](https://github.com/user-attachments/assets/f1928ce9-0e74-42a1-a47c-9bc2a8ddf391) [2](https://github.com/user-attachments/assets/82765d0f-1195-4b05-b809-a9d1691ab2b8)|
| DCS World | ✔️ |  |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12, install into `bin` folder, not `bin-mt` folder | [1](https://github.com/user-attachments/assets/3f00b18f-baaf-4671-8673-54b33d2c9794) [2](https://github.com/user-attachments/assets/caf32493-aaf4-429b-b109-9d8ab1a64788) |
| [Dead Island 2](https://github.com/optiscaler/OptiScaler/wiki/Dead-Island-2) |  ✔️  | |  FSR2  | ✅ | [1](https://github.com/user-attachments/assets/3018f01d-d73d-4206-9169-8fc3179c9b03) |
| Deadlink | ✔️ | |  DLSS, XeSS | DLSS or XeSS settings must be changed everytime game is loaded for Opti to work | [1](https://github.com/user-attachments/assets/a9284b2f-2f56-4188-973d-7465345f3161)|
| Dead Rising Deluxe Remaster | ✔️ | | FSR3, XeSS, DLSS | Enable Restore Compute Root Signature to fix DLSS input graphical issues | [1](https://github.com/user-attachments/assets/c580b55c-ceb9-4f9f-81db-7558656c8fa0) |
| [Dead Space (2023)](https://github.com/optiscaler/OptiScaler/wiki/Dead-Space-Remastered) | ✔️ | ✅ |  FSR2, DLSS |  For DLSS inputs [Fakenvapi](Fakenvapi) is needed. Disable Motion Blur to fix ghosting around Isaac’s head. | [1](https://github.com/user-attachments/assets/f08c5e9f-64b6-492e-a90c-ce47f7ea9bc6) <br>🐧[1](https://github.com/user-attachments/assets/6a0fe85d-9d4c-45d1-a047-d7273f61b7f7) |
| Deadzone Rogue | ✔️ | |  DLSS, FSR3.1 |  | [1](https://github.com/user-attachments/assets/eea3fbdb-15d3-4cda-9063-7a2bb74df2be)|
| Deathloop |  ❌  |  |   | Built-in EAC, Opti doesn’t work on paid, up-to-date versions  |
| [Death Stranding: Director’s Cut](https://github.com/optiscaler/OptiScaler/wiki/Death-Stranding-Director's-Cut)|  ✔️  | ✅ | XeSS, DLSS | Try [Non-Linear PQ Input](#Image-Quality) for better image quality. | [1](https://github.com/user-attachments/assets/c4b98e2d-fa4b-4c59-be28-92097aa4b715) <br>🐧[1](https://github.com/user-attachments/assets/54a1d125-6881-4c84-ae17-d500dd023295)   |
| Deconstruction Simulator | ✔️ | | DLSS, XeSS | enable Non-Linear sRGB input to fix flashing graphics | [1](https://github.com/user-attachments/assets/18b41415-9760-4475-abee-2df9a9fe205e)|
| Deep Rock Galactic | ✔️ | ✅ | DLSS, FSR2 | On Windows 10 needs [FsrAgilitySDKUpgrade](#directx-12-agility-sdk-for-windows-10) | [1](https://github.com/user-attachments/assets/2704548d-8986-4992-b037-32765b9ca714) <br>🐧[1](https://github.com/user-attachments/assets/478f42e2-0ce4-4e5f-b549-565d0de82ba1) [2](https://github.com/user-attachments/assets/0187e472-0539-4221-a21a-8a6ec33d4a19) |
| Deliver Us Mars | ✔️ | | FSR | Use Dx12 mode | [1](https://github.com/user-attachments/assets/2d714874-e9eb-430d-83d8-475ac5bf51ef)|
| Deliver Us the Moon | ✔️ | ✅ | DLSS| Use DX12 |  |
| Desordre | ✔️ | |  DLSS | XeSS may have graphical issues with some Opti versions, FSR is not detected | [1](https://github.com/user-attachments/assets/454fbfec-17f5-4b16-88c4-4e025bd7e789)|
| Destroy All Humans! 2 – Reprobed | ✔️ | |  DLSS | Use Dx12 |
| Diablo II: Resurrected |  ✔️  | |  DLSS |   | [1](https://github.com/user-attachments/assets/5c9f43a9-4bf3-4022-ac01-52af8854fd2e) |
| [Diablo IV](https://github.com/optiscaler/OptiScaler/wiki/Diablo-4) | ✔️ | |  DLSS | Use Dx12 |
| [Directorate Novitiate (Demo)](https://github.com/optiscaler/OptiScaler/wiki/Directorate-Novitiate) | ✔️ | |  DLSS, FSR3.1, XeSS |  | [1](https://github.com/user-attachments/assets/246d68e8-cef7-42f7-aeef-27d10b69a403)
| Dolmen | ✔️ | |  DLSS | | [1](https://github.com/user-attachments/assets/4df9f4af-fbac-4d2f-b366-26314ac25945)|
| [Doom Eternal](https://github.com/optiscaler/OptiScaler/wiki/DOOM-Eternal) |  ❌  | |  DLSS  | Vulkan  | |
| [Doom: The Dark Ages](https://github.com/optiscaler/OptiScaler/wiki/Doom-The-Dark-Ages) |  ❌  | |  DLSS, FSR, XeSS  | Vulkan  | |
| Dragon Age: The Veilguard | ✔️ | ✅ |  DLSS, XeSS  | FSR2 has broken graphics, For Linux use OptiScaler as `winmm.dll` | 🐧[1](https://github.com/user-attachments/assets/eec6cc7b-6c72-404e-8123-51203bb1370e) |
| [Dragon’s Dogma II](https://github.com/optiscaler/OptiScaler/wiki/Dragons-Dogma-2) |  ✔️  | |  FSR3, DLSS  | Set `FGType=nofg`; DLSS input also „works“, but breaks GUI, to fix GUI try enabling options under Root Signatures tab (specifically `Restore Compute Root Signature`, reported by a user)  | [1](https://github.com/user-attachments/assets/f9df3dca-10d1-4471-a89e-65e3354a9c08)  |
| Dragonkin: The Banished | ✔️ | | DLSS, FSR3.1 | FSR crashes without [Unreal Engine Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks). Engine.ini must be created and made read-only | [1](https://github.com/user-attachments/assets/9dbd7a26-0ba1-4f80-ac8d-c0898b1a8fec) [2](https://github.com/user-attachments/assets/876fc0d0-200c-49f0-8479-53e385b52881) |
| [Dying Light 2](https://github.com/optiscaler/OptiScaler/wiki/Dying-Light-2)  |  ✔️  | ✅ |  DLSS, XeSS | | [1](https://github.com/user-attachments/assets/f6fbbba6-7917-4799-a141-c98fe0ee2112) <br>🐧[1](https://github.com/user-attachments/assets/91ac73ea-0e37-41c5-8192-1e4152e72c24) |
| [Dying Light The Beast](https://github.com/optiscaler/OptiScaler/wiki/Dying-Light-The-Beast)  |  ✔️  |  |  DLSS, XeSS, FSR | | [1](https://github.com/user-attachments/assets/ee1cc337-ecea-4690-8302-5988652af483) [2](https://github.com/user-attachments/assets/d9f996fa-3ff5-4d0e-a3ab-e7d122d478f8) [3](https://github.com/user-attachments/assets/da207f5e-6368-4fd8-beda-de6ef83ab2eb) |
| EA Sports WRC | ❌ | |  DLSS, FSR2 | EA Anti-cheat blocks unknown `.dlls` |
| Echoes of the End | ✔️ | | DLSS, FSR3.1 | Use [Non-Linear sRGB Input](https://github.com/optiscaler/OptiScaler/wiki/FSR4-Compatibility-List#Image-Quality) to fix screen flickering. Selecting FSR3.1 input causes a crash. To fix this, create an `Engine.ini` file in<br>`.\AppData\Local\TheDarken\Saved\Config\Windows\` and add the FSR3.1 commands from the [UE Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks) page. NOTE: Set `Engine.ini` to read-only to prevent the game from deleting it on startup. | [1](https://github.com/user-attachments/assets/cbdcb032-cf75-445a-9477-8ba0cbe21165)|
| Echoes of Yi: Samsara | ✔️ | |  DLSS | Create a shortcut with `-dx12` launch parameter  | [1](https://github.com/user-attachments/assets/92331d66-75bb-44cb-80ef-352f81a90f64) |
| Echo Point Nova | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/2ba7e57b-e2d3-4321-a6bb-10b9a5877f4c) |
| Edge of Eternity | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/c0059e05-f134-46af-8b85-aa00234ca702)|
| Elden Ring | ✔️ | | FSR3 | Use ERSS-FG mod as basis for Upscaling and FG  | [1](https://github.com/user-attachments/assets/deaab6a1-376f-4655-af10-9a111e3afd4e) |
| Empire of the Ants|  ✔️  | |   DLSS |  Copy files into<br>`.\Empire of the Ants\Empire\Binaries\Win64\` |[1](https://github.com/user-attachments/assets/9204d23d-c9a1-4b1a-a9f7-53d4350f08e9)|
| Empyreal | ✔️ | | FSR3, XeSS | | [1](https://github.com/user-attachments/assets/2ded9213-b865-484b-b7f2-7df797f2991f)|
| Enshrouded  |  ❌  | |  DLSS  | Vulkan, shimmering grass and fog. |
| Enotria: The Last Song | ✔️ | |  DLSS, FSR3, XeSS | **Officially supported game**. Run game with `-dx12` launch parameter. | [1](https://github.com/user-attachments/assets/ad70015d-da59-4873-b77b-e366b88f54cb)|
| Ereban: Shadow Legacy | ✔️ | | DLSS | Unity game, Dx11 only. | [1](https://github.com/user-attachments/assets/4e055654-6362-4c2a-ae07-35e044dd04e1) |
| Ertugrul of Ulukayin | ✔️ | | DLSS, XeSS, FSR3.1 | FSR3.1 crashes without [Unreal Engine Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks). Engine.ini must be created and also made read-only | [1](https://github.com/user-attachments/assets/ac444025-6ca2-4364-92b3-38294457004d) |
| Escape from Naraka | ✔️ | |  DLSS | |[1](https://github.com/user-attachments/assets/26340fdd-105b-45b8-b49c-f93a5ed8595f)|
| [Escape from Tarkov – SPT](https://github.com/optiscaler/OptiScaler/wiki/Escape-from-Tarkov-(SPT)) | ✔️ | |  DLSS | Works only for Singleplayer Tarkov (SPT) mod without EAC. Unity game, Dx11, use FSR3.X/4 w/Dx12. |[1](https://github.com/user-attachments/assets/ee327080-60e0-4cf8-98b2-9d9e729b3f56)|
| Escape Simulator 2 | ✔️ | ✅ | DLSS | (Demo) forced Ultra Performance preset. Needs scale override, DLSS option is found in the dynamic resolution setting. | [1](https://github.com/user-attachments/assets/66116ee9-e432-4724-8843-45794e74bc21) <br>🐧[1](https://github.com/user-attachments/assets/ef7c2346-0d00-4d5c-9d20-116dcb5c181c) |
| Eternal Strands |  ✔️  | |   DLSS, FSR, XeSS  |   | [1](https://github.com/user-attachments/assets/e0e90471-97d1-46cc-b076-b4446f9cbda6)|
| Everspace 2 | ✔️ | |  DLSS, FSR3.1, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/8daeed33-599e-4cc9-aee9-446a73d5ed27) |
| Evil West |  ✔️  | |   DLSS  | Run game with `-dx12` launch option, Dx11 mode has low performance and severe shimmer. | [1](https://github.com/user-attachments/assets/54ab14aa-52fa-4f07-bf80-b611ff67e09d) |
| EVOTINCTION | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/25ba74ff-6870-4e00-884f-65e570f6d6ea) |
| F1 2020 | ✔️ |  | DLSS |  | [1](https://github.com/user-attachments/assets/95f17dc5-fd23-4f1c-a85d-d8c95ee01b7f) |
| F1 2022 |  | ✅ | DLSS | Disable FSR2 inputs in OptiScaler.ini as they crash game, requires [Fakenvapi](Fakenvapi) and Keep DLSS at Quality as trying to switch crashes game, use optis upscale ratio override instead (works in real time) | 🐧[1](https://github.com/user-attachments/assets/238f1c4c-207f-4e4d-98bc-4ec2e5d68575) |
| F1 2024 | ✔️ | | FSR, XeSS | It works, but spoofing DLSS on an AMD card doesn’t work.  | [1](https://github.com/user-attachments/assets/7a7d1614-f0f9-45c5-b328-909ca8e2fd82) |
| [F1 Manager 2024](https://github.com/optiscaler/OptiScaler/wiki/F1-Manager-2024) | ✔️ | |  DLSS |   |  |
| Faraday Protocol | ✔️ || DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/9dc404a9-ca4f-4018-9ada-58eeb07164c4)|
| Farming Simulator 22  | ✔️ | ✅ |  DLSS, FSR3 | | [1](https://github.com/user-attachments/assets/27d3be1b-b8ad-4b6f-a5f0-6addb904629c) <br>🐧[1](https://github.com/user-attachments/assets/c3cd6d07-3919-4443-bd30-1ab90c02338a) |
| Farming Simulator 25 | ✔️  | | DLSS, FSR3, XeSS | Ensure Detours API Hooking is enabled in RTSS overlay | [1](https://github.com/user-attachments/assets/4921cc63-d5ec-4bea-9e7f-69b8dbdbba20)|
| [Final Fantasy VII Rebirth](https://github.com/optiscaler/OptiScaler/wiki/FINAL-FANTASY-VII-REBIRTH) |  ✔️  | ✅ |  DLSS  | Spoofing within the mod will cause some stutter. It’s needed to be able to use DLSS inputs though. Turning off SAM will help reduce that. | [1](https://github.com/user-attachments/assets/6ee5e3f7-a3f5-47ab-b71f-d9b2e7a71756)  |
| [Final Fantasy XIV](https://github.com/optiscaler/OptiScaler/wiki/Final-Fantasy-XIV-Dawntrail) | ✔️ |  |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. Requires [Fakenvapi](Fakenvapi). | [1](https://github.com/user-attachments/assets/86f03747-3a4c-4660-a464-1b9d408034f8) [2](https://github.com/user-attachments/assets/cfb4f0cd-03aa-4bf1-b09c-4b4fd857b188) |
| [Final Fantasy XVI](https://github.com/optiscaler/OptiScaler/wiki/FINAL-FANTASY-XVI) |  ✔️  | ✅ |  DLSS, FSR3  | Enable [Non-Linear PQ Input](#Image-Quality) for better image quality  | [1](https://github.com/user-attachments/assets/9295a4b7-3391-4773-8909-2d0abe625a17) [2](https://github.com/user-attachments/assets/4530911f-7c29-4f10-9e0a-e4b79fe69325) |
| Firefighting Simulator: Ignite | ✔️ | | DLSS, FSR3 | | [1](https://github.com/user-attachments/assets/05bc7b8e-36eb-4cfc-9779-070898eae9f8) |
| Firmament | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/65cd4842-84bb-4393-809f-28dadf40f182)|
| F.I.S.T.: Forged In Shadow Torch | ✔️ | |  DLSS | Copy files to `.\F.I.S.T. - Forged In Shadow Torch\ZingangGame\Binaries\Win64` | [1](https://github.com/user-attachments/assets/48c10f08-dd93-4886-8cc8-b4c924dfa4a0)|
| Flipscapes | ✔️ | | FSR3.1 | FSR setting must be changed for Optisclaer to detect it FSR3.1 input. DLSS input is only detected if games is restarted after choosing DLSS | [1](https://github.com/user-attachments/assets/498f01b9-072f-4604-ba20-2f8f9ad16158)|
| Flintlock: The Siege Of Dawn | ✔️ | |  DLSS | Please Refer To [UE Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks), XeSS has graphical issues. | [1](https://github.com/user-attachments/assets/9dd9a8b0-1d5f-4219-ac0a-a96b50705e9d) |
| Fobia – St. Dinfna Hotel | ✔️| | DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/c8aca27b-7d30-4acc-b47e-5e94500e5a83) |
| Forgive Me Father 2 | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter, enable [Non-Linear sRGB Input](#Image-Quality) to fix screen flicker. | [1](https://github.com/user-attachments/assets/db2d8b31-3a9b-446e-a16e-8418875d76fd)|
| [Forspoken](https://github.com/optiscaler/OptiScaler/wiki/Forspoken) | ✔️ || DLSS | `OptiScaler.dll ` must be renamed to `d3d12.dll ` to detect DLSS input | [1](https://github.com/user-attachments/assets/c5a632eb-8a97-47d9-8b4a-ea26b6d904a8)|
| Fort Solis |  ✔️  | ✅ |  FSR3, DLSS | **Officially supported game**. Use FSR3 in game settings, DLSS has some ghosting next to the main character and more shimmer. | [1](https://github.com/user-attachments/assets/81157eb2-1f71-4058-890a-4ddf4d1f6b54) <br>🐧[1](https://github.com/user-attachments/assets/cdf3fc82-b887-4b75-b93a-52b92e3fa5f0) | 
| [Forza Horizon 5](https://github.com/optiscaler/OptiScaler/wiki/Forza-Horizon-5) |  ✔️  |  |  DLSS | Needs [Fakenvapi](Fakenvapi) and using the [ASI method](https://github.com/optiscaler/OptiScaler/wiki/Forza-Horizon-5), if crashing, try setting `EnableFsr2Inputs=false` in OptiScaler.ini. | [1](https://github.com/user-attachments/assets/e4156d14-43c2-4a3d-8b9c-1d0eb21b7a30) |
| Forza Motorsport |  ✔️  |  |  DLSS | Requires [Fakenvapi](Fakenvapi) and setting `EnableFsr2Inputs=false` in OptiScaler.ini | [1](https://github.com/user-attachments/assets/81157eb2-1f71-4058-890a-4ddf4d1f6b54) [2](https://github.com/user-attachments/assets/5b509cd1-e0ed-4300-9334-0602477d1866) <br> |
| Five Nights at Freddy’s: Security Breach  | ✔️| | DLSS | DLSS is enabled by default – no option to disable it in game settings | [1](https://github.com/user-attachments/assets/c91a187d-0f45-4901-9c6a-d32ef888e606)|
| [Frostpunk 2](https://github.com/optiscaler/OptiScaler/wiki/Frostpunk-2) |  ✔️  | |  DLSS, XeSS, FSR  | **Officially supported game**. Enable [Non-Linear sRGB Input](#Image-Quality) for better image quality, FSR Inputs require [FSR3.1 inputs crash fix](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks#when-using-fsr31-inputs-game-is-crashing) or just disable them with `Fsr3=false` and `Ffx=false` in OptiScaler.ini. | [1](https://github.com/user-attachments/assets/8c8e73af-0f92-48ba-a183-8c70f4492076) |  
| Frozenheim | ✔️ | |  DLSS | Use Dx12 mode | [1](https://github.com/user-attachments/assets/cb5d6401-31d9-4f67-b5a5-21dd61f80ce3)|
| Funko Fusion | ✔️ | | FSR3, DLSS, XeSS | **Officially supported game**. Use FSR input for best result. Enable [Non-Linear sRGB Input](#Image-Quality) if screen flickers with DLSS Balanced. | [1](https://github.com/user-attachments/assets/35a6dbe8-0d8a-4de6-ad02-106393aa1a2c)|
| Gears of War: Reloaded |  ❌  | | | EAC. Game has FSR3.1, but no FSR dll. | |  
| Ghost of Tsushima |  ✔️  | |  FSR3, DLSS, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/a7575128-2bc3-4016-933d-3946bace222e) [2](https://github.com/user-attachments/assets/1f646192-4a56-4390-91ae-a0f032271294) [3](https://github.com/user-attachments/assets/539b63ba-b566-408b-9d01-057155d742f5) |  
| [Ghostrunner](https://github.com/optiscaler/OptiScaler/wiki/Ghostrunner) |  ✔️  | ✅ |  DLSS  | Game must run in Dx12 (`-dx12`) | [1](https://github.com/user-attachments/assets/82c68aa0-4586-4612-a9fe-3b33f6886f58) |  
| [Ghostrunner 2](https://github.com/optiscaler/OptiScaler/wiki/Ghostrunner-2) | ✔️ | ✅ | FSR2, XeSS, DLSS | Game must run in Dx12 (`-dx12`), spoofing seems to cause a big performance overhead, so recommended to use FSR or XeSS inputs. | [1](https://github.com/user-attachments/assets/c23d2906-3504-4277-b59c-e588b7bc1a4e) <br>🐧[1](https://github.com/user-attachments/assets/631052a3-c49c-45e2-a09d-4af1731d45e1) [2](https://github.com/user-attachments/assets/29ec94b0-d02f-4a92-888c-4b910d2e9c4c) [3](https://github.com/user-attachments/assets/205e7c8f-d55d-43b0-903a-6311aced513e) |
| Ghostwire: Tokyo |  ✔️  | |  DLSS, FSR2  |   | [1](https://github.com/user-attachments/assets/292486eb-a0ab-4b49-af6d-90c9170f6f1b) |
| God of War |  ✔️  | ✅ |  DLSS  | **Dx11 game**, use FSR3.X/4 w/Dx12, [Non-Linear sRGB Input](#Image-Quality) seems to look the best; use [Fakenvapi](Fakenvapi) to fix performance.  | [1](https://github.com/user-attachments/assets/c1abc93c-2727-4985-a2b7-25ed230c20bc) <br>🐧[1](https://github.com/user-attachments/assets/5b4eb08b-cdae-404f-b21f-4d5c2feb0e0a) |
| [Gotham Knights](https://github.com/optiscaler/OptiScaler/wiki/Gotham-Knights) | ✔️ |  | DLSS |   | [1](https://github.com/user-attachments/assets/613f2bdd-23ea-404a-94fd-375d5ce0af2e) |
| Gori: Cuddly Carnage | ✔️ |  | DLSS | XeSS has graphical issues | [1](https://github.com/user-attachments/assets/cbeb5ce7-f6ea-457f-b089-b498f2fcb508)|
| [Grand Theft Auto: San Andreas – Definitive Edition](https://github.com/optiscaler/OptiScaler/wiki/Grand-Theft-Auto:-San-Andreas-%E2%80%90-Definitive-Edition) | ✔️ |  | DLSS | Use [Fakenvapi](Fakenvapi) to fix performance under Dx11 as game crashes with `-dx12`. Image is jittery. | [1](https://github.com/user-attachments/assets/d408bbd9-da19-4d15-ac4c-47a8c110e492) |  
| [Grand Theft Auto V Enhanced](https://github.com/optiscaler/OptiScaler/wiki/Grand-Theft-Auto-V-Enhanced) |  ✔️  | |  FSR, DLSS  | **Officially supported game**. To stop crashes with OptiScaler, either set `Dxgi=false` or add [Fakenvapi](Fakenvapi) (also enables DLSS inputs). | [1](https://github.com/user-attachments/assets/72224e43-9c2b-41c2-9040-69feec7e96bb) [2](https://github.com/user-attachments/assets/3419d7da-feca-45d7-80f9-74a481bdd304) |
| Greedland | ✔️ | | FSR3, DLSS | If DLSS input is selected then the game needs to be reloaded for Optiscaler to detect DLSS. FSR3 input works fine without restarting. | [1](https://github.com/user-attachments/assets/d1cfa45e-b5b7-446a-a3c1-b2b41e20a506) [2](https://github.com/user-attachments/assets/1e05b647-e391-490d-a66a-41d129e3c707)|
| Grid Legends | ✔️ |  | XeSS |  Game forces TAA when XeSS is turned on -> even with FSR4 looks very blurry | [1](https://github.com/user-attachments/assets/cc2fe998-4b22-478e-892d-09cda8c26c62) |  
| Gripper | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/1e39b1c6-25ca-4759-a3f5-938709ab0069)|
| [Guardians of the Galaxy](https://github.com/optiscaler/OptiScaler/wiki/Guardians-of-the-Galaxy) |  ✔️  | |  DLSS | **Yes** to DLSS inputs and set `Dxgi=false` under `[Spoofing]` – critical for proper performance | [1](https://github.com/user-attachments/assets/2b5620bc-7a85-442c-ac68-7c6075fdbdec) |
| Gungrave G.O.R.E | ✔️ | |  DLSS |  | [1](https://github.com/user-attachments/assets/19341cb7-2155-467b-82c7-f83f5f630c73) |
| Gun Jam | ✔️ | | DLSS || [1](https://github.com/user-attachments/assets/abc80f9e-16bb-44ee-9309-6bd9b33410bd)|
| Haste | ✔️ |  |  DLSS, FSR2 | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/045adedd-69af-4e7d-bea8-8e3014c6d072)|
| [Hellblade: Senua’s Sacrifice](https://github.com/optiscaler/OptiScaler/wiki/Hellblade:-Senua's-Sacrifice) |  ✔️  | ✅ | DLSS | Use `dxgi.dll`, OptiFG, hudfix=true. | [1](https://github.com/user-attachments/assets/030309d2-1c6f-47d8-96fa-0b81635dc245) <br> 🐧[1](https://github.com/user-attachments/assets/d8181e47-fb07-4ed6-a4fb-23f29209c26b) |
| Hell Is Us | ✔️ | ✅ | FSR3, DLSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/a5014781-671f-4719-838f-c62c6b5475ed) <br>🐧[1](https://github.com/user-attachments/assets/80162f92-ef76-43d4-b16d-379ae770e82f) |
| Hell Pie | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/62d35020-21c3-49c3-9d7e-72d6e4cedd15)|
| Hello Neighbor 2 | ✔️ | | DLSS | No DLSS option visible in game settings but DLSS input is working in Opti menu | [1](https://github.com/user-attachments/assets/2e433c3c-49ae-4bd4-bdde-2205986dffc5) |
| Hi-Fi RUSH | ✔️ |  | DLSS, XeSS |   | [1](https://github.com/user-attachments/assets/155af122-b972-4cc0-8636-e8e3fe13784b) [2](https://github.com/user-attachments/assets/2b431be0-2079-4fad-b84a-3f9aeebc7ab2) |
| High On Life | ✔️ | |  DLSS |   | [1](https://github.com/user-attachments/assets/fc856856-6db4-47b9-b71c-203a11c231fa) |
| Highrise City | ✔️ | | DLSS | Run in Dx12 mode | [1](https://github.com/user-attachments/assets/12c6c442-8381-44bc-b1e8-ffe0b8e82e1e) |
| [Hitman: World of Assassination](https://github.com/optiscaler/OptiScaler/wiki/HITMAN-3-World-of-Assassination) | ✔️ | ✅ | DLSS, XeSS | Spoofing disables RT options, better use XeSS inputs on Intel/AMD. | [1](https://github.com/user-attachments/assets/2bfb6f4a-0eae-4603-b0e5-2cdfc6e3677f) [2](https://github.com/user-attachments/assets/f6ab9be9-1249-45f0-bd6c-2e2025f10bfa) <br>🐧[1](https://github.com/user-attachments/assets/c105461f-ddd5-47d6-9f21-8e6ba97e73a6) [2](https://github.com/user-attachments/assets/b323f5d8-f574-41f6-a4fe-996ca854ece1) |
| [Hogwarts Legacy](https://github.com/optiscaler/OptiScaler/wiki/Hogwarts-Legacy) |  ✔️  |  | DLSS | DLSS inputs have some dark checkboard artifacts, try FSR or XeSS inputs to fix. Spoofing Nvidia/DLSS inputs causes significant performance overhead in both raster and ray tracing on Windows. Set `Dxgi=false` in the OptiScaler.ini if DLSS FG/Reflex aren’t needed. | [1](https://github.com/user-attachments/assets/93efac59-5a15-4d24-991c-2d45ec02923c) [2](https://github.com/user-attachments/assets/9b12fa96-a8fa-484b-84e3-2e18d4c37a40) |
| Homeworld 3 | ✔️ | |  DLSS, FSR2, XeSS |   | [1](https://github.com/user-attachments/assets/98964edd-6134-487d-bf4b-bc7ad5ec211d) |
| [Horizon Forbidden West](https://github.com/optiscaler/OptiScaler/wiki/Horizon-Forbidden-West) | ✔️ | ✅ |  DLSS, FSR3.1, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/f768c7ad-79a7-4c80-aedc-e9c88916caa0) [2](https://github.com/user-attachments/assets/9839f046-69da-4976-b4e3-b3100b00d9dc) [3](https://github.com/user-attachments/assets/9bfb417e-caf8-4d65-89b6-1d801e881485) <br>🐧[1](https://github.com/user-attachments/assets/fac9a6de-e910-4b15-8098-f430ba5edb8e) [2](https://github.com/user-attachments/assets/1f38d4c4-d964-403f-89af-ee1987f0b2b8) [3](https://github.com/user-attachments/assets/10ae6e28-dcf4-447b-b56c-cee30ddcaee9) |
| [Horizon Zero Dawn Complete Edition](https://github.com/optiscaler/OptiScaler/wiki/Horizon-Zero-Dawn) |  ✔️  | |  DLSS  | Not the Remastered Version |
| [Horizon Zero Dawn Remastered](https://github.com/optiscaler/OptiScaler/wiki/Horizon-Zero-Dawn-Remastered) |  ✔️  | |  DLSS, FSR3.1, XeSS  | **Officially supported game** | [1](https://github.com/user-attachments/assets/3f053411-d2d5-4289-83ff-7e8ac3988603) | 
| HOT WHEELS UNLEASHED | ✔️ | |  DLSS |   | [1](https://github.com/user-attachments/assets/e0159d5a-24ed-4e0a-9bf7-0323a34b615b) |
| Hydroneer | ✔️ | |  DLSS |  | [1](https://github.com/user-attachments/assets/c52c30a1-c641-4cc0-88be-b7e793d7c1a3) |
| Ikonei Island: An Earthlock Adventure | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/f1f76bda-b07f-4094-8d0f-f8ba862b21cb) | 
| Immortals of Aveum |  ✔️  | |  DLSS, FSR3 | Use [Fakenvapi](Fakenvapi), [Nukem’s](Nukem's-dlssg‐to‐fsr3) needs DLSS inputs for correct rendering on FSR3.1/4/XeSS, with FSR inputs, everything except native looks bad. Loaded upscaler is often reset when loading a new level and must be reselected. Used Nukem for FSR3.1 FG  for the entire game. Nukem causes crashes in chapter 12 during a specific, unskippable cutscene, regardless of the settings. Remove Opti to progress. | [1](https://github.com/user-attachments/assets/09450374-df78-4f00-a0d2-2675a58f3463) |  
| [Indiana Jones and the Great Circle](https://github.com/optiscaler/OptiScaler/wiki/Indiana-Jones-and-the-Great-Circle) |  ❌  |  |  | Vulkan |
| Indika | ✔️ | | DLSS | |[1](https://github.com/user-attachments/assets/5158b129-3696-49b2-b0d1-e669d9d6e1e3) |
| Industria | ✔️ | ✅ | DLSS | | [1](https://github.com/user-attachments/assets/995bffa5-c62b-4f98-a987-4ab9028cffe8) |
| Infinity Nikki | ✔️ | | DLSS | May occasionally crash during intro videos. You can fix it by removing „Login...“ files from<br>`.\Infinity Nikki\X6Game\Content\Movies\`. | [1](https://github.com/user-attachments/assets/67e44d99-3305-4298-b04f-dc5d38020480) |
| Influx Redux | ✔️| | DLSS, FSR3.1 | FSR3 Quality mode has flashing lights on water, use Balanced/Performance modes or DLSS. | [1](https://github.com/user-attachments/assets/9ec4242d-a4d0-475b-b329-b04b7faa3379)|
| Into the Dead: Our Darkest Days |  ✔️  | |  DLSS | Use [Fakenvapi](Fakenvapi) for DLSS inputs |  [1](https://github.com/user-attachments/assets/53ca7450-fe94-4a8a-9da8-cba31e0e543d) |
| inZOI |  ✔️  | |  DLSS, FSR | **Officially supported game**. Use [Fakenvapi](Fakenvapi) for DLSS inputs. |  [1](https://github.com/user-attachments/assets/cbef1271-b25b-41bd-910b-b5adec385247) |  
| Jagged Alliance 3 | ✔️ | |  DLSS, XeSS | | [1](https://github.com/user-attachments/assets/7f082810-9f49-4bf2-956a-db825ef6d551), [2](https://github.com/user-attachments/assets/b7fa0722-5095-4d9d-afe3-517a123d0c27)   |
| JDM: Japanese Drift Master | ✔️ | | DLSS, XeSS | Use DLSS for the best result |[1](https://github.com/user-attachments/assets/5ba61821-0b56-418c-a3c5-38043e55cdf0)|
| Jotunnslayer - Hordes of Hel | ✔️ | | DLSS | Unity game. FSR3.1 and XeSS inputs are not detected by Optiscaler | [1](https://github.com/user-attachments/assets/56163ffb-683d-4a2e-bf2c-5d63753cceda)|
| Judgment | ✔️ | | DLSS | Use DLSS input. FSR2.1 and XeSS do not work. | [1](https://github.com/user-attachments/assets/1beb9236-5834-4e5f-b5f9-cd8f1a2e41bf) |
| Jump Space (formerly Jump Ship) | ✔️ | ✅ |  DLSS  | *Dx11* Online co-op Unity game (no anti-cheat), create shortcut with -force-d3d12 launch paramater to enable DX12 mode | [1](https://github.com/user-attachments/assets/7f3a7aa0-def6-4470-9144-8dd4b68b4ad9) [2](https://github.com/user-attachments/assets/8dc7f278-3bfb-4c01-b15a-7922983e1ae3) 🐧[1](https://github.com/user-attachments/assets/3fec454a-d5d0-4c96-b149-56786ea0e0c8) |
| Jurassic World Evolution 2 | ✔️ | |  DLSS | Requires [Fakenvapi](Fakenvapi). | [1](https://github.com/user-attachments/assets/6f002b67-474d-49b0-8f14-e19d02d8b917) |
| Jusant |  ✔️  | |  DLSS  | Enable [Non-Linear sRGB Input](#Image-Quality) to fix flicker |
| KARMA: The Dark World | ✔️ | ✅ |  FSR3, DLSS, XeSS |  | [1](https://github.com/user-attachments/assets/4e50ebd2-a7ea-4dd1-aca2-d8fe3694311b)  🐧[2](https://github.com/user-attachments/assets/16afad44-40d8-4f60-883c-a19ddd4ff3c2) |
| Kena: Bridge of Spirits |  ✔️  | ✅ |  DLSS |   |
| Kiborg | ✔️ | |  DLSS | Enable [Non-Linear sRGB Input](#Image-Quality) to fix flicker | [1](https://github.com/user-attachments/assets/75cfe7df-8d03-4b82-8e0e-8ee1c959576e)|
| Kingdom Come: Deliverance II |  ✔️  | |  DLSS, FSR3 | **Officially supported game**. On Windows 10 needs [FsrAgilitySDKUpgrade](#directx-12-agility-sdk-for-windows-10). |
| Kunitsu-Gami: Path of the Goddess | ✔️ | | DLSS | Enable Restore Compute Root Signature to fix graphics corruption (Under Advanced Settings > Root Signatures), FSR3 input is not detected. | [1](https://github.com/user-attachments/assets/3f2a0f91-de63-4741-9689-9d8c59e50911) |
| Land of the Vikings | ✔️ | | DLSS| Create a shortcut with `-dx12` launch parameter, use Model 5 to reduce shimmering trees. | [1](https://github.com/user-attachments/assets/f6a18948-5fe5-4912-83fd-5b6977167ac2)|
| LANESPLIT (Demo) | | ✅ | DLSS |  | 🐧[1](https://github.com/user-attachments/assets/6a640ef3-6efe-4406-8776-cd0cb0a7ee2b) |
| Last Train Home | ✔️ | |  DLSS | Create a shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/d4cf79b0-6c58-435e-9ad2-2642c0838b66)|
| Layers of Fear (2023) | ✔️ | |  DLSS, FSR2 | XeSS has graphical corruption | [1](https://github.com/user-attachments/assets/09784440-623a-415c-a20b-b26370e982bb)|
| LEGO Builder’s Journey | ✔️ | ✅ |  DLSS | Requires [Fakenvapi](Fakenvapi) | [1](https://github.com/user-attachments/assets/d9bb1b0c-dd1f-4732-8f95-ff10b74980eb) <br>🐧[1](https://github.com/user-attachments/assets/753344dd-dbd6-4ddc-b0eb-835ad29f3312) |
| LEGO Horizon Adventures | ✔️ | |  DLSS |   | [1](https://github.com/user-attachments/assets/58c86f9a-e77a-4dbd-8d1f-25c388b4483b)|
| [Lies of P](https://github.com/optiscaler/OptiScaler/wiki/Lies-of-P) |  ✔️  |  | DLSS, FSR3.1  | **Officially supported game** | [1](https://github.com/user-attachments/assets/21917947-d1e0-453e-8270-e4f6540a442a) [2](https://github.com/user-attachments/assets/b8dce327-5eec-47ea-aae8-5a19fdc2e278)|
| Lightyear Frontier | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/2c9b88ca-1dd3-4d1a-b526-ff4acbeaf941)|
| Like A Dragon: Infinite Wealth |  ✔️  |  | DLSS  |   |
| Like a Dragon Gaiden: The Man Who Erased His Name | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/e12d4159-c65c-48d0-b57f-6349fda85ffc) |
| Liminalcore | ✔️ | | DLSS, FSR3 | FSR Inputs require [FSR3.1 inputs crash fix](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks#when-using-fsr31-inputs-game-is-crashing) or just disable them with `Fsr3=false` and `Ffx=false` in OptiScaler.ini  | [1](https://github.com/user-attachments/assets/ee1cd8f9-d829-40bf-bb16-01fc871b7e5f)|
| Little Nightmares III (Demo) |  ✔️  |  | DLSS, XeSS  |   | [1](https://github.com/user-attachments/assets/da4b018f-ff41-4649-b317-4eff106f0c96) |
| Loopmancer | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/01626879-c542-4608-87f6-38199a7d23b4)|
| Lost Judgment |  ✔️  |  | DLSS  |   |
| Lost Records: Bloom and Rage | ✔️ |  | DLSS | Requires disabling FSR3 inputs, otherwise it crashes – set `Fsr3=false` in OptiScaler.ini. | [1](https://github.com/user-attachments/assets/0afb7b92-c432-49f6-a58c-3c0295395197)|
| Lost Skies | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/c534024d-03d8-4853-9cca-6192cdcc6973)|
| Lost Soul Aside | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/0afff9ae-b1f9-4ddf-8397-66a42732bffe)|
| [Lords of the Fallen (2023)](https://github.com/optiscaler/OptiScaler/wiki/Lords-of-the-Fallen) | ✔️ | ✅ | FSR3, DLSS, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/863ee484-3082-4a93-b558-160a912c711a) [2](https://github.com/user-attachments/assets/74adcbf8-ec38-43d4-ab0c-4db6f38dd18a) <br>🐧[1](https://github.com/user-attachments/assets/6ab84dc1-11a0-4022-9a37-191ad8da6fef) [2](https://github.com/user-attachments/assets/bb91b9d1-a72f-45a9-8cc8-fe6fb05e46c1) [3](https://github.com/user-attachments/assets/7d04a92f-d8eb-4947-b5af-f76d4cadf48e) |
| Lunacy : Saint Rhodes | ✔️| | DLSS | Create a shortcut with `-dx12` launch parameter |[1](https://github.com/user-attachments/assets/456c9857-6e53-4938-bdd2-4752faa2d219) |
| Mandragora: Whispers of the Witch Tree | ✔️ |  | DLSS, XeSS | Tested on DLSS input, but XeSS should also be working. | [1](https://github.com/user-attachments/assets/a917fb83-f8c4-47cd-8ca3-3eea792cbdfd) [2](https://github.com/user-attachments/assets/bc9e1bc5-b0cb-4fa6-915a-910c1ab7d566) |
| Manor Lords | ✔️  |  | DLSS, XeSS  | DLSS inputs use custom ratios, Quality 1.3, Balanced 2.0, Performance 3.0 and are hardcoded - Balanced seems to work fine, but others require `Non-Linear (sRGB)` to work properly. XeSS inputs also require `Non-Linear sRGB` to work, and have hardcoded XeSS 1.3+ ratios | [1](https://github.com/user-attachments/assets/1571be41-c8de-4d90-a82a-04462815eba1), [2](https://github.com/user-attachments/assets/f228f75a-d0aa-49a8-85fb-17a8ded51a5e) |
| Martha is Dead | ✔️ |  | DLSS | Window in the screenshot is very pixelated depending on your angle, might need [Non-Linear PQ Input](#Image-Quality) for better image quality. | [1](https://github.com/user-attachments/assets/b9a4d0cb-a097-4213-a985-206e95ec198a) [2](https://github.com/user-attachments/assets/6a72ef67-9278-4064-bd5e-29e09dabda99) |
| Marvel’s Avengers | ✔️ |  | DLSS | Use [Fakenvapi](Fakenvapi) | [1](https://github.com/user-attachments/assets/92f91b44-6f80-4f38-8530-81bbdaeaefeb) | 
| [Marvel's Midnight Suns](https://github.com/optiscaler/OptiScaler/wiki/Marvel's-Midnight-Suns) | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/982d72f8-a3dc-462c-a253-b8d79fd58d04)  [2](https://github.com/user-attachments/assets/29dfa1ab-ccc2-48e0-b23c-7e99dec6d9c0)|
| MechWarrior 5: Mercenaries |  ✔️  |  | DLSS |  Run game with `-dx12` commandline, requires [Fakenvapi](Fakenvapi) for DLSS inputs. | [1](https://github.com/user-attachments/assets/5a3a5497-f216-4257-97d6-69421cfd59fc) [2](https://github.com/user-attachments/assets/2eb3313f-e1f7-4aec-87a7-42c8169317d6) |
| Medieval Dynasty | ✔️ |  | XeSS |  | [1](https://github.com/user-attachments/assets/f072086b-bd14-4a06-bf84-f557304a6589) |
| Memory’s Reach (Demo) | ✔️ | ✅ | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. In-game FSR is just FSR1 | [1](https://github.com/user-attachments/assets/f624ef20-403e-4111-8e37-0eda6f0e2526) <br>🐧[1](https://github.com/user-attachments/assets/19c25c1c-2ab3-45ac-91d0-7143e15d820b) |
| Menace |  ✔️  |   |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/f560e5a0-70c3-456f-90d0-20bc3ba29c86) |  
| METAL EDEN |  ✔️  |  | DLSS, FSR, XeSS |  FSR3.1 inputs require `Engine.ini` commands from [Wiki link](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks#when-using-fsr31-inputs-game-is-crashing) | [1](https://github.com/user-attachments/assets/9fb00197-1aba-4b7f-825e-2cbac4c38b1e) [2](https://github.com/user-attachments/assets/8739fd12-fb82-47d9-87f7-adc9b825a2b6) [3](https://github.com/user-attachments/assets/c5e46ea3-77b0-4c5c-b879-8e5d58ec5951)|  
| [Metal Gear Solid Delta: Snake Eater](https://github.com/OptiScaler/OptiScaler/wiki/Metal-Gear-Solid-%CE%94-Snake-Eater) |  ✔️  |  | DLSS | Be aware that the DLSS presets Quality (75%) and Balanced (67%) use higher internal resolutions than expected. Check OptiPatcher to see how to force FG, albeit it might not be implemented yet with reason, as it crashes at certain scenes. | [1](https://github.com/user-attachments/assets/f4c218e6-74f9-448e-8db8-2f9040e538ec) |
| [Metro Exodus: Enhanced Edition](https://github.com/optiscaler/OptiScaler/wiki/Metro-Exodus-Enhanced-Edition) |  ❌  |  | DLSS  | On-screen corruption with all presets but best with Balanced/Ultra Performance, recommended to use FSR 4.0.0, as 4.0.1 is worse in that regard. | [1](https://private-user-images.githubusercontent.com/29551044/490954939-ec28835c-4dc3-4462-9e93-246ca2321a5f.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NTgxOTA0NjYsIm5iZiI6MTc1ODE5MDE2NiwicGF0aCI6Ii8yOTU1MTA0NC80OTA5NTQ5MzktZWMyODgzNWMtNGRjMy00NDYyLTllOTMtMjQ2Y2EyMzIxYTVmLnBuZz9YLUFtei1BbGdvcml0aG09QVdTNC1ITUFDLVNIQTI1NiZYLUFtei1DcmVkZW50aWFsPUFLSUFWQ09EWUxTQTUzUFFLNFpBJTJGMjAyNTA5MTglMkZ1cy1lYXN0LTElMkZzMyUyRmF3czRfcmVxdWVzdCZYLUFtei1EYXRlPTIwMjUwOTE4VDEwMDkyNlomWC1BbXotRXhwaXJlcz0zMDAmWC1BbXotU2lnbmF0dXJlPTZkY2YxODY4OWZkNDAzNDk5M2U3ZTczZjU5MGFjYjhmNGE4ZGVhM2M4MzRkMzExNDZjNDZhODAwNjVmMDQwY2UmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0In0.c0yDbjDyW5VR5pRaB-mqZZJMucuTttFdxkSFRvFc0lI) |
| [Microsoft Flight Simulator 2020](https://github.com/optiscaler/OptiScaler/wiki/Microsoft-Flight-Simulator-2020) |  ✔️  |   | DLSS  |  Works with Windows 11, crashes with Windows 10 even with [FsrAgilitySDKUpgrade](#directx-12-agility-sdk-for-windows-10). |
| [Microsoft Flight Simulator 2024](https://github.com/optiscaler/OptiScaler/wiki/Microsoft-Flight-Simulator-2024) |  ✔️  |  | DLSS |  Tested with Windows 11 | [1](https://github.com/user-attachments/assets/0b87c97a-a21a-4b3f-b5ed-c6d618d6b02c) |
| MindsEye |  ✔️  |  | FSR, DLSS, XeSS |  **Officially supported game**. ATM ships an unsigned `ffx.dll` which breaks FSR4 driver toggle – replace with Prebuilt Signed one. | [1](https://github.com/user-attachments/assets/c2704807-c540-4321-a83f-59c99e9d282b) [2](https://github.com/user-attachments/assets/d41edbb9-6807-48ce-9937-a81a0ddcc36f) |
| [Minecraft RTX](https://github.com/optiscaler/OptiScaler/wiki/Minecraft-RTX) | ✔️  | ⛔ | DLSS  | Won’t work on Linux since its a UWP game, and Wine does not have support for UWP. | [1](https://github.com/user-attachments/assets/72ebc1d0-cea1-400f-ab17-b9d6afdc6a97) [2](https://github.com/user-attachments/assets/d77f53c9-65c3-49de-aea5-0d754bb75e46) |
| Monster Hunter Rise |  ✔️  |  | DLSS | Requires [Fakenvapi](Fakenvapi). Also delete `config.ini` if dlss in disabled when game is launched.| [1](https://github.com/user-attachments/assets/95e0eb92-6f3a-44eb-8e0b-ec464bb10344)|
| [Monster Hunter Wilds](https://github.com/optiscaler/OptiScaler/wiki/Monster-Hunter-Wilds) | ✔️ | ✅ | DLSS, FSR, XeSS | **Officially supported game**. Requires REFramework, set `Dxgi=false` in the OptiScaler.ini to avoid crashes. | |
| Moroi | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/73ec4125-41be-4b7d-9bb1-246c9c499b9d)|
| Mortal Shell |  ✔️  |  | DLSS |  | [1](https://github.com/user-attachments/assets/05f80568-4d9f-40d8-a527-ec56373b63d6) |   
| Mount & Blade II Bannerlord |  ✔️  |   |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/aebbc2cf-a601-4aaa-b97b-60ecf5d4978d)  [2](https://github.com/user-attachments/assets/f8d6154b-f5dc-4971-8660-26cd481fcc39) |  
| MYST | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/6ef365b0-7a08-4615-a278-3190dc618522)|
| Need for Speed: Unbound | ✔️ | ✅ | DLSS, FSR2, XeSS  | On Linux set dll name to `d3d12.dll` | [1](https://github.com/user-attachments/assets/ffe052ca-8741-4f43-ad54-e5b1c06e5a8c) [2](https://github.com/user-attachments/assets/bb74052c-c198-498c-92aa-87ec60c17286) [3](https://github.com/user-attachments/assets/8ba2a264-5312-4eaf-bad1-db197909e3b4) <br>🐧[1](https://github.com/user-attachments/assets/f99969b9-f98f-42b4-8c65-f1e29bfdfffa) [2](https://github.com/user-attachments/assets/f63d6e8b-c401-4c6c-b435-f704d4161c33) [3](https://github.com/user-attachments/assets/4324b070-d0d6-495c-8f8c-7c777132445e) |
| Necromunda: Hired Gun | ✔️ |  | DLSS | Necessary to force enable Nvidia GPU spoofing for DXGI to access games DLSS options|[1](https://github.com/user-attachments/assets/d4129cfd-0127-4d02-8009-bf3e18625d22)|
| Necrophosis | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/30d61fdf-bae7-4a9c-94f6-c02eaccca405) |
| Neverness to Everness | ✔️ | | DLSS | Use OptiScaler as `winmm.dll`, requires Fakenvapi | [1](https://github.com/user-attachments/assets/75ef5d3c-addf-40e2-b292-7bc2000667ac) |
| Nightingale | ✔️ | | DLSS, FSR3.1, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/22d70a57-bac5-47bf-89f8-106afa67cfad)|
| [NINJA GAIDEN 2 Black](https://github.com/optiscaler/OptiScaler/wiki/NINJA-GAIDEN-2-Black) | ✔️  |  | DLSS, XeSS  | **Officially supported game**. Played Gamepass version with XeSS: place OptiScaler next to `NINJAGAIDEN2BLACK.exe`, not the `wingdk.exe`. Observed flickering in cutscenes, checking [Non-Linear sRGB Input](#Image-Quality) fixed this. | [1](https://github.com/user-attachments/assets/f7e45723-70a1-4f46-b384-e92e17654ed3) [2](https://github.com/user-attachments/assets/840062a2-1856-42fd-9742-a651c57be5d5) |
| [Nioh 2](https://github.com/optiscaler/OptiScaler/wiki/Nioh-2-The-Complete-Edition) | ✔️ |  | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12, needs AutoExposure. | [1](https://github.com/user-attachments/assets/6f840a68-6de5-4ee2-919d-22363e50985e) |
| Nobody Wants To Die | ✔️ | ✅ | DLSS, FSR3, XeSS | OptiFG works with both DLSS and FSR, HUDFix 1(?) is enough for UI. | [1](https://github.com/user-attachments/assets/c31e3cbf-dba8-4ed0-b4cc-3372f2793f43)  |
| [No Man’s Sky](https://github.com/optiscaler/OptiScaler/wiki/No-Man's-Sky) |  ❌  | |    | Vulkan |
| No Rest For The Wicked | ✔️ | ✅ | DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/dd0b99ca-9b03-4625-aeaf-7704b119b8ae) <br>🐧[1](https://github.com/user-attachments/assets/5d9a821c-1337-440e-93b4-f21b32823f7c) |
| Observer: System Redux | ✔️ |  | DLSS  | | [1](https://github.com/user-attachments/assets/42a2eda1-47a5-46cd-8e97-90705ac6e340)|
| Oddsparks: An Automation Adventure | ✔️ | | DLSS | Enable Linear sRGB Input to fix flicker | [1](https://github.com/user-attachments/assets/ed3e28a1-790a-48a3-bb36-91652588759b)|
| Once Alive | ✔️ | | DLSS, FSR3 | | [1](https://github.com/user-attachments/assets/3cd14a63-1da6-49c3-ad0c-6e9f381df326)|
| Orbit.Industries | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/5bad06d7-7b2d-448a-be3e-4decb29fcd3a) |
| Otherskin | ✔️ |  | DLSS |  | [1](https://github.com/user-attachments/assets/672720b7-5df4-467f-a7a4-8b8ab943df52) |
|Outcast: A New Beginning |  ✔️ |  | DLSS | Run game with `-dx12` launch option, copy files into<br>`.\Outcast - A New Beginning\O2\Binaries\Win64\`. | [1](https://github.com/user-attachments/assets/8e448366-8977-47a5-8452-5acba09978c1)|
| Outriders |  ✔️  |  | DLSS | Dx12 | [1](https://github.com/user-attachments/assets/dba6e9f4-bd0d-4e23-b2c0-f28bba5651a6)|
| Outpost: Infinity Siege |  ✔️  |  | DLSS, FSR3, XeSS |  | [1](https://github.com/user-attachments/assets/71cbbfe3-9beb-4ab6-9fad-b4fac49b80e2) |
| Pacific Drive |  ✔️  | ✅ | DLSS  | Use [Fakenvapi](Fakenvapi) – Linux RDNA3 with FSR4+FG: HUDfix to stabilize the UI. Use OptiFG to enable FG. | 🐧[1](https://github.com/user-attachments/assets/4f0e228d-6477-4455-9514-49aa1bd5d7d7)
| [Palworld](https://github.com/optiscaler/OptiScaler/wiki/Palworld) | ✔️ | ✅ | DLSS | Dx12 must be forced to prevent jitter artifacts. Linux requires Dx12, otherwise it crashes on the intro screen. | [1](https://github.com/user-attachments/assets/28812f00-a1e2-4b8f-a438-a90f0c246eaf) <br>🐧[1](https://github.com/user-attachments/assets/d36bf709-065c-4dae-811b-aace2408991e) |
| Panicore | ✔️ | | DLSS, FSR2.1, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/0f6bb504-9719-43c0-8567-1a31372e9148)|
| Party Animals |  ✔️  | ✅ | DLSS  |   | [1](https://github.com/user-attachments/assets/befe066b-fb69-4b14-a1d1-bb227003ef5c)<br> 🐧[1](https://github.com/user-attachments/assets/4ad74da9-8901-4557-93dc-8f9af10b6779)  |
| [Path of Exile 2](https://github.com/optiscaler/OptiScaler/wiki/Path-of-Exile-2) |  ✔️  |  | DLSS  |  | [1](https://github.com/user-attachments/assets/c43620f1-c960-4031-8841-3b0b9a2ba12a) |
| PC Building Simulator 2 | ✔️ | | DLSS | Unity Dx11 Game, Use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/840f998b-ddfd-4508-979c-4fd5d13f8876) [2](https://github.com/user-attachments/assets/eee30251-87fa-4c2d-8c1c-9501f0d914b2)|
| Pizzapocalypse | ✔️  | | DLSS | | [1](https://github.com/user-attachments/assets/ac592156-cf50-416b-b3ca-ce8cda344ccb)|
| Postal 4 |  ✔️  |  | DLSS |   |
| POOLS | ✔️ | | DLSS | **Dx11 Game**, Use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/c76d4c88-7ec8-4100-86e7-fc046176911a) |
| [Prey 2017 Luma](https://github.com/optiscaler/OptiScaler/wiki/Prey-Luma-Remastered-mod) | ✔️  | | DLSS | Requires Prey Luma Remastered mod and carefully reading the [compatibility entry](https://github.com/optiscaler/OptiScaler/wiki/Prey-Luma-Remastered-mod) | [1](https://github.com/user-attachments/assets/b23c6598-44bd-4810-862d-424c73694e5b)|
| Project Borealis: Prologue |  ✔️  |  | DLSS, XeSS |   | [1](https://github.com/user-attachments/assets/56f0f846-8b06-4ef3-874a-ce85f7ecc4d6) |
| Psycho Fear | ✔️ | | DLSS | Use [Non-Linear sRGB Input](#Image-Quality) to fix screen flicker | [1](https://github.com/user-attachments/assets/a4c50666-d12c-4bd7-80f5-9f04f6bc3784) |
| Pumpkin Jack |  ✔️  |  | DLSS |   | [1](https://github.com/user-attachments/assets/e5bd1376-feac-4bea-b9da-165794efeaad) |
| Q.U.B.E. 10th Anniversary | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/4e467b99-7e4e-4f1d-8958-dc48196ceb80) |
| Raji: An Ancient Epic | ✔️ | | DLSS | Use DLSS, FSR input isn’t detected. | [1](https://github.com/user-attachments/assets/34f4278f-8a82-454d-9cfe-ee59d4bb72b6) |
| Ready or Not |  ✔️  |  | FSR  |    | [1](https://github.com/user-attachments/assets/78782d7b-859e-430e-b0bb-c76fbeb074c9)  |
| [Red Dead Redemption](https://github.com/optiscaler/OptiScaler/wiki/Red-Dead-Redemption) |  ✔️  | ✅ | DLSS  |  Use [Fakenvapi](Fakenvapi) | [1](https://github.com/user-attachments/assets/fdec99dc-5341-4174-9006-f210ac1a4c15) <br>🐧[1](https://github.com/user-attachments/assets/202dc8e7-7033-453f-9a36-e796a6f1c5ac) |
| [Red Dead Redemption 2](https://github.com/optiscaler/OptiScaler/wiki/Red-Dead-Redemption-II) |  ✔️  | ✅  | DLSS |  Use Dx12 & [Fakenvapi](Fakenvapi).Recommended turning off NV Reflex as it causes stutters.<br>Linux: use Proton, latest vkd3d-proton-git & Dx12 ingame API. | [1](https://github.com/user-attachments/assets/2175d656-29fd-4263-b235-eb96145414b3) <br>🐧[1](https://github.com/user-attachments/assets/60e2a3f9-0e09-4446-ab23-98f2d23772fe) |
| Redfall |  | ✅ | DLSS | Use OptiScaler as `winmm.dll` | 🐧[1](https://github.com/user-attachments/assets/3e8ae45f-b817-4189-b5a6-c558b4f86de8) |
| Redout: Space Assault | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/2c390fe6-f8e3-416c-9e45-12b654c70ca8) |
| [Redout 2](https://github.com/optiscaler/OptiScaler/wiki/Redout-2) | ✔️ | | XeSS | Game has XeSS only. Black block artifacts on right side of screen in cutscenes and some levels | [1](https://github.com/user-attachments/assets/3c16a864-577c-4cfa-b485-8d7e8edfcb6c)|
| REFramework Games |  ✔️  |  | DLSS  |  See [Wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-7-Biohazard) on how to set up |
| Remnant II |  ✔️  |  | DLSS, FSR3, XeSS  | **Officially supported game** | [1](https://github.com/user-attachments/assets/76d067ce-0093-44f0-bdfa-17aa3dd4846f) | 
| [Resident Evil 2](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-2) |  ✔️  |  | XeSS  | Requires REFramework (pd-upscaler branch) + PDUpscaler plugin + OptiScaler, check [Wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-2)  | [1](https://github.com/user-attachments/assets/981a794b-ce09-4319-be7c-803252ca451c) [2](https://github.com/user-attachments/assets/380668f2-32b4-4864-86f6-a7c09c8c8630) [3](https://github.com/user-attachments/assets/94e3a15b-0b7c-46cc-8b2a-fdb84bdcdfa2) [4](https://github.com/user-attachments/assets/96d96f8b-7e0d-44a3-aae2-7c412ac3df3c) | 
| [Resident Evil 3](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-3) |  ✔️  |  | XeSS  | Requires REFramework (pd-upscaler branch) + PDUpscaler plugin + OptiScaler, check [Wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-3)  |  | 
| [Resident Evil 4](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-8-Village) |  ✔️  |  | XeSS  | Requires REFramework (pd-upscaler branch) + PDUpscaler plugin + OptiScaler, check [Wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-4-Remake)  | [1](https://github.com/user-attachments/assets/4dcdb34e-d85d-4eed-90c1-434a4e4a06e1) [2](https://github.com/user-attachments/assets/bc2f0109-7f5a-4b0e-8a15-7845c3ace7d5) |
| [Resident Evil 8 Village](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-4-Remake) |  ❌  |  |   | Black screen when switching to FSR4, cause unknown  | |
| [Returnal](https://github.com/optiscaler/OptiScaler/wiki/Returnal) |  ✔️  | |  XeSS, DLSS  | [Non-Linear sRGB Input](#Image-Quality) seems to work best | [1](https://github.com/user-attachments/assets/1e98d7f9-f181-4e26-b4ba-5dab62591533) |
| Revenge of the Savage Planet | ✔️ | | DLSS, XeSS, FSR | FSR Inputs require [FSR3.1 inputs crash fix](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks#when-using-fsr31-inputs-game-is-crashing) or just disable them with `Fsr3=false` and `Ffx=false` in OptiScaler.ini | [1](https://github.com/user-attachments/assets/fc816f64-856d-463f-8316-ee31cfd38790) |
| Ripout | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/f9170478-767c-4067-98d6-ab2c5bd11f48) |
| [Rise of the Ronin](https://github.com/optiscaler/OptiScaler/wiki/Rise-of-the-Ronin) |  ✔️  | ✅ | DLSS | Had to use [Fakenvapi](Fakenvapi) to show DLSS and had to enable [Nukem’s](Nukem's-dlssg‐to‐fsr3) to enable OptiScaler menu | [1](https://github.com/user-attachments/assets/697c63c3-532c-4f12-870d-4a92421dacbe) [2](https://github.com/user-attachments/assets/73bfbb9b-74ef-4180-9fd9-5eebd3dfbb5b) [3](https://github.com/user-attachments/assets/4ea5e889-f409-4a8e-abfe-70627053cf9c) <br>🐧[1](https://github.com/user-attachments/assets/c17c1478-4b5d-4f91-bdea-8e0b34dc9433) |
| [Rise of the Tomb Raider](https://github.com/optiscaler/OptiScaler/wiki/Rise-of-the-Tomb-Raider) | ✔️ | ✅ | DLSS | | [1](https://github.com/user-attachments/assets/81038153-0b77-4515-9401-a7501c6faaac) <br>🐧[1](https://github.com/user-attachments/assets/4d7b2b0d-6205-451a-b511-d0621f13d7a3) |
| Riven | ✔️ |  | DLSS | Use [Non-Linear sRGB Input](#Image-Quality)  | [1](https://github.com/user-attachments/assets/a074623c-9634-403b-95e4-954cc1689426) |
| Roadcraft | ✔️ | ✅ | DLSS, FSR3 | **Officially supported game** | [1](https://github.com/user-attachments/assets/a012c78c-faed-4ff0-8fab-d01f7c40a4c2) <br>🐧[1](https://github.com/user-attachments/assets/ed099602-3c75-46d0-8ab9-7b5fb52a5072) |  
| [RoboCop: Rogue City](https://github.com/optiscaler/OptiScaler/wiki/Robocop-Rogue-City) |  ✔️  | ✅ | FSR3  |   | [1](https://github.com/user-attachments/assets/0f4f5399-7162-48ee-a86f-eb3582fd76dc) |  
| [RoboCop: Rogue City – Unfinished Business](https://github.com/optiscaler/OptiScaler/wiki/Robocop-Rogue-City-%E2%80%90-Unfinished-Business) |  ✔️  |  | DLSS, FSR3.1, XeSS | DLSS inputs may flicker on balanced preset, use different inputs if so. Can also enable [Non-Linear sRGB Input](#Image-Quality) instead, but it may introduce ghosting in dark areas. | [1](https://github.com/user-attachments/assets/2bdaaa12-cd13-4478-be46-f220e958adcc) |
| Romancelvania | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter, Dx11 mode has low performance. | [1](https://github.com/user-attachments/assets/edebc6e5-208f-493a-ad40-d50b8e6f602d) |
| Rune Factory: Guardians of Azuma | ✔️ | | DLSS | `EnableFsr3Inputs=false` must be set in OptiScaler.ini to prevent a crash during startup. | [1](https://github.com/user-attachments/assets/ac38a779-bfd0-4505-b874-329010af8c54)
| Runescape: Dragonwilds | ✔️ | | DLSS, FSR3.1 | Early Access game - online co-op or offline singleplayer  | [1](https://github.com/user-attachments/assets/ddd88bf9-298c-4217-94e2-0c5b9b463f16) [2](https://github.com/user-attachments/assets/23fa56d6-bd9e-498d-8864-e5b0f5b2127c)|
| Sackboy: A Big Adventure | ✔️ | ✅ |  DLSS |  | [1](https://github.com/user-attachments/assets/d0cb23bd-9d29-4cfc-86d9-f24509891106) [2](https://github.com/user-attachments/assets/f02d48fc-97d0-442b-97b6-6b618c40cabb) |
| Saints Row | ✔️ |  | FSR2 | Use Dx12, need to enable [Non-Linear sRGB Input](#Image-Quality). Works but not worth it because of bad image quality (ghosting). | [1](https://github.com/user-attachments/assets/5857a5e6-dc47-4bf8-9135-0256738295df) |  
| Sands of Aura | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/edafadf5-f805-4b46-9975-d5e299c35ae7) |
| Satisfactory | ✔️ | ✅ | FSR3, DLSS, XeSS | Use Dx12 and install to `Satisfactory/Engine/Binaries/Win64` rather than `Satisfactory/FactoryGame/Binaries/Win64`. | [1](https://github.com/user-attachments/assets/1052fc10-54fa-4106-a897-4eafb94070ad) [2](https://github.com/user-attachments/assets/658e63cf-85ab-4274-94fb-d94426324f40) <br>🐧[1](https://github.com/user-attachments/assets/93cffe3c-61b6-4937-83b6-acce739d50bd) [2](https://github.com/user-attachments/assets/d2c576a2-c76a-4515-9042-4d8f2c65454c) [3](https://github.com/user-attachments/assets/ac7335d5-e903-43c5-ab1f-b79e755e7c60) |
| Scathe | ✔️ |  | DLSS | Create shortcut with `-dx12` launch parameter. FSR input causes crash. | [1](https://github.com/user-attachments/assets/d9d57bae-6802-4107-822e-16efd3e02733)|
| Scorn | ✔️ | ✅ | FSR2 | Linux: RDNA3 with FSR4 + FG |[1](https://github.com/user-attachments/assets/2549e4f1-afec-4ff5-be99-1ab90e389727) <br /> 🐧[1](https://github.com/user-attachments/assets/19fd883a-1ba1-4ef6-a550-39437a0463d2) |
| SCP: Secret Files | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter, FSR2 input crashes when selected, Dx11 mode has low performance and severe shimmer. | [1](https://github.com/user-attachments/assets/0d34f123-744c-4b37-8fad-8a663dabbb2c) |
| Season: A Letter to the Future | ✔️ | | DLSS | Launch with `-dx12` parameter | [1](https://github.com/user-attachments/assets/be754126-b90d-4a19-bd71-4df6519ea672)|
| Sengoku Dynasty | ✔️ |  |  DLSS |   |  [1](https://github.com/user-attachments/assets/6d055151-a0c4-4eb3-93e3-b337e8127895) |  
| [Senua’s Saga: Hellblade 2](https://github.com/optiscaler/OptiScaler/wiki/Senua's-Saga-Hellblade-II) |  ✔️  | ✅ | DLSS, FSR3, XeSS | Use [Nukem’s](Nukem's-dlssg‐to‐fsr3) for less crashing. Spoofing to Nvidia causes some minor shadow errors that would not occur otherwise - use [OptiPatcher](https://github.com/optiscaler/OptiPatcher) to unlock DLSS(FG) without spoofing. RDNA3 with FSR4 working. | [1](https://github.com/user-attachments/assets/033aeb83-4602-4a52-855a-2cc3d8a7f544) [2](https://github.com/user-attachments/assets/36ed1ab1-d916-44f3-af52-d3b03fb9f6af) <br>🐧[1](https://github.com/user-attachments/assets/0881d387-5e70-4aa2-80b5-1ac6866858c1) |
| Severed Steel | ✔️ |   | DLSS | Run Dx12 (With RTX) version |  
| Shadow Warrior 3 | ✔️ |  | DLSS | Run game with `-dx12` launch option, Dx11 mode has low performance and severe shimmer | [1](https://github.com/user-attachments/assets/8e5e86b6-a257-4c36-b022-a84b8593c373) |
| [Shadow of the Tomb Raider](https://github.com/optiscaler/OptiScaler/wiki/Shadow-of-the-Tomb-Raider) |  ✔️  | ✅ | DLSS, XeSS | Requires [Fakenvapi](Fakenvapi) for DLSS inputs | 🐧[1](https://github.com/user-attachments/assets/c52dc0de-2a94-4098-b7e8-27f3ba1f52b0) [2](https://github.com/user-attachments/assets/2b0de2ef-068e-4fc7-a42a-ee938450a886) |
| Shadowgunners | ✔️ |  | DLSS  |  | [1](https://github.com/user-attachments/assets/20dea75b-7869-4328-b2f0-c30b743ed0ba) |
| Shadows of Doubt | ✔️ |  |  DLSS | **Dx11 game**, use FSR3.X/4 w/Dx12. | [1](https://github.com/user-attachments/assets/0f89d384-2cb5-4f65-a7ed-d458b2fc1cfe), [2](https://github.com/user-attachments/assets/2ada6f24-3f9b-40f7-a8aa-ccb12d0ef284) |
| Shatterline | ✔️ | | DLSS | DX11 game. Image quality looks worse than native with all upscalers | [1](https://github.com/user-attachments/assets/497f64cc-2687-4ca9-a882-6106cb0c7f22)|
| Sherlock Holmes: The Awakened | ✔️ | |  DLSS |   | [1](https://github.com/user-attachments/assets/77a466c1-d4e9-4a82-9a97-c2f0b9183eaa) |
| Sifu | ✔️ |  | DLSS | Force Dx12 with `-dx12` launch option, otherwise requires [Fakenvapi](Fakenvapi) to fix performance. | [1](https://github.com/user-attachments/assets/18562049-e166-42ae-9eba-49ab57c59613) |
| [Silent Hill 2 (Remake)](https://github.com/optiscaler/OptiScaler/wiki/Silent-Hill-2-Remake) |  ✔️  | ✅ | DLSS, FSR3.1, XeSS | **Officially supported game**. [Non-Linear sRGB Input](#Image-Quality) seems to fix DLSS flickering.  Linux RDNA3 w/ FSR4 + requires UE5 [engine tweak](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks). | [1](https://github.com/user-attachments/assets/045643c6-0cff-4d48-8fd9-3709bb2db3bb) [2](https://github.com/user-attachments/assets/a6b46be4-d9b1-497f-993e-0b8a3648fae5) <br />🐧[1](https://github.com/user-attachments/assets/d9e9229a-ece3-42fe-806a-4e21c1e376c4)|
| [Silent Hill f](https://github.com/optiscaler/OptiScaler/wiki/Silent-Hill-f) | ✔️ | | FSR | DLSS input flickers on some presets (e.g. Balanced), stick to FSR inputs. | [1](https://github.com/user-attachments/assets/381202b7-9606-4986-ad81-3a0582c0579c) [2](https://github.com/user-attachments/assets/c232c8ac-2242-433e-993e-b5589e670140) |
| Simulakros | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/b5bcfbd2-a43a-4c21-986f-a1404228d9d4) |
| SlavicPunk: Oldtimer | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/8539759e-311e-41bb-9c90-52b271f52718)|
| Slender: The Arrival | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/1a7c8195-bb1d-427c-85b8-57967ba441c8)|
| Smalland: Survive the Wilds | ✔️ |   | DLSS, FSR2 |  |  [1](https://github.com/user-attachments/assets/989f529f-0bdb-4506-89dd-3e411135dfac) |  
| Someday You’ll Return: Director’s Cut | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/4f4308fa-abfd-44ab-9172-4d861c85528b) |
| Sons of the Forest |  ✔️  | ✅ | DLSS, FSR3 | **Dx11 game**, use FSR3.X/4 w/Dx12. Might have issues when opening/ closing inventory. RDNA3 with FSR4 working. | [1](https://github.com/user-attachments/assets/a8d1d8b2-f811-4f2d-acdf-d78430eab535) <br>🐧[1](https://github.com/user-attachments/assets/2d85a3db-3c6d-43b9-aeed-963c07d89528) [2](https://github.com/user-attachments/assets/47f8e4e2-4e3f-4413-9750-2c35af81b540) |
| Soulmask | ✔️  |  | DLSS  | Use [Fakenvapi](Fakenvapi), need to enable DLSS from `GameUserSettings.ini` in `%LocalAppData%\WS\Saved\Config\WindowsNoEditor` | [1](https://github.com/user-attachments/assets/f0315d9f-c21f-4780-b5de-ac68612ab854) |
| Soulslinger: Envoy of Death | ✔️ | | DLSS | Enable [Non-Linear sRGB Input](#Image-Quality) to fix screen flicker issues | [1](https://github.com/user-attachments/assets/035402d9-c057-4171-9640-f90201f98ba6)|
| Soulstice | ✔️ |  |  DLSS | Run from shortcut with `-dx12` launch parameter, reduce antialiasing to get better quality. Graphics turn white with XeSS (may need rechecking). | [1](https://github.com/user-attachments/assets/dbf768c7-82ba-4983-806f-3304045a2cea)|
| South of Midnight |  ✔️ | |   DLSS | Use [Fakenvapi](Fakenvapi) to expose DLSS input | [1](https://github.com/user-attachments/assets/1e755352-42e0-4cb6-9078-bcff0ae21541) [2](https://github.com/user-attachments/assets/d226f10e-f4f6-4e9a-9952-75b8403ebf2a) |
| Spirit of the North 2 | ✔️ | | FSR3.1, DLSS | | [1](https://github.com/user-attachments/assets/c7beb266-6654-4393-9d1e-5521e47ac9c9)|
| Spirit X Strike | ✔️ | | FSR3.1 | | [1](https://github.com/user-attachments/assets/d0990699-e33d-4d1d-9fe9-32ea9a13239a) [2](https://github.com/user-attachments/assets/a3845681-ea10-4f97-ae2e-b41d34c29c40)|
| Split Fiction |  ✔️  |  | FSR3 |   | [1](https://github.com/user-attachments/assets/17cecd7d-0c5a-4d9c-a96d-80e6dab1683a) |
| SPRAWL | ✔️ |  | DLSS |  Run from shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/1359a50a-8a23-4575-a0a0-7bf9fbf1f956) |
| Starfield |  ✔️  | ✅ | DLSS  | | [1](https://github.com/user-attachments/assets/cb33af99-56a6-4c18-991a-ed5a6ff3564d) <br>🐧[1](https://github.com/user-attachments/assets/0e8cd439-d6e5-416f-88af-11b40a36b538) |
| Star Overdrive | ✔️ | ✅ | FSR3,DLSS | | [1](https://github.com/user-attachments/assets/fdb8ef53-7277-4ac2-b634-749d8b4483cd) <br>🐧[1](https://github.com/user-attachments/assets/16f72d94-9d80-4718-8745-1d49b26e0a08) |
| [Star Wars Jedi: Survivor](https://github.com/optiscaler/OptiScaler/wiki/STAR-WARS-Jedi-Survivor) | ✔️  | ✅ | DLSS | Run in Fullscreen Windowed mode. May need to restart the game for DLSS toggle to function. | [1](https://github.com/user-attachments/assets/b5224b76-a86e-493f-8504-cf51a47fa3b3) <br>🐧[1](https://github.com/user-attachments/assets/ea7e8b65-be85-4323-b284-2a19adce66aa)|
| S.T.A.L.K.E.R. 2: Heart of Chornobyl |  ✔️  |  | FSR, DLSS  | **Officially supported game**. Also works for GamePass version with `dxgi.dll`. | [1](https://github.com/user-attachments/assets/51e2dc72-eb52-438d-8667-8c0ccc0f7345) [2](https://github.com/user-attachments/assets/1389534d-ceb1-44ab-9ccd-5cd98ec9629d) |
| [Star Wars Outlaws](https://github.com/optiscaler/OptiScaler/wiki/Star-Wars-Outlaws) |  ✔️  | ✅ |  | **Officially supported game** (Whitelist not supported in the not updated demo due to FSR3.0 still). | 🐧[1](https://github.com/user-attachments/assets/f880beca-ea9a-40b7-8c62-c85636c6ce15) |
| Stay in the Light | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/340d12d0-d5f9-40fc-a938-4b48486a7cb0)|
| Steel Seed | ✔️ |  | DLSS, FSR3, XeSS | **Officially supported game**. Use DLSS input for best results. | [1](https://github.com/user-attachments/assets/700408f4-2c5b-432b-a4ee-23b6f5c09e90)|
| Steelrising | ✔️ | ✅ | FSR, DLSS | [Fakenvapi](Fakenvapi) is needed for game to set DLSS. [Non-Linear sRGB Input](#Image-Quality) required to avoid ghosting. | [1](https://github.com/user-attachments/assets/fc1eb79e-dd09-487d-b0b6-d1374777f78c) <br>🐧[1](https://github.com/user-attachments/assets/58491a48-37e3-4c5a-bb80-1ec160ecb5aa) |
| [Stellar Blade](https://github.com/optiscaler/OptiScaler/wiki/Stellar-Blade) | ✔️ |  | DLSS, FSR3 | **Officially supported game**. Demo also worked fine with Opti FSR4. | [1](https://github.com/user-attachments/assets/1cbfa769-433b-4d1d-b16b-13299676fef8) [2](https://github.com/user-attachments/assets/736e9997-a8d8-484d-b3cc-5215eb45d9b3) [3](https://github.com/user-attachments/assets/927b954a-6fca-4e6a-ab25-cc72a6a04a37) |
| Still Wakes the Deep | ✔️ | ✅ | FSR3, DLSS, XeSS | | [1](https://github.com/user-attachments/assets/17b3788d-726d-4cca-8b7f-8b5ff3f023ff) <br>🐧[1](https://github.com/user-attachments/assets/0a2379cd-c491-42b1-96cc-69b6fa7744f9) |
| Storage Hunter Simulator | ✔️ | | DLSS, FSR3.1 | FSR3.1 crashes without [Unreal Engine Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks). Engine.ini must be created and made read-only  | [1](https://github.com/user-attachments/assets/44356f63-7f0c-4a59-beac-3382228e200e) [2](https://github.com/user-attachments/assets/c47eeb85-cb15-4c4f-b97a-22c2da2cc912)|
| Stranger of Paradise: Final Fantasy Origin | ✔️ | | DLSS | Use `Dx11Upscaler=fsr31_12` in OptiScaler.ini to prevent crashes on startup. | [1](https://github.com/user-attachments/assets/80b91588-6a13-49b5-aa54-c5397e17b97d)|
| Stranded: Alien Dawn | ✔️ | | DLSS | FSR2 inputs doesn’t seem to work | [1](https://github.com/user-attachments/assets/0967d562-333b-4d41-9ea1-175f1cfbb11f)|
| Strayed Lights | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/0f35df46-6824-49fc-9ac6-e659d30b286b) |
| [Suicide Squad: Kill the Justice League](https://github.com/optiscaler/OptiScaler/wiki/Suicide-Squad-Kill-The-Justice-League) |  |  | DLSS, FSR | EAC game in both single player and multiplayer – works with EAC bypass – tested FSR3.0/3.1/XeSS, LukeFz FG injection some time ago, OptiFG probably works. | |
| Sumerian Six | ✔️ |  | DLSS | Enable [Non-Linear sRGB Input](#Image-Quality) otherwise screen will flash constantly | [1](https://github.com/user-attachments/assets/08628021-7ebd-43f1-838d-03232e112171)|
| Supraland |  ✔️  |  | DLSS  | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/2d92e61c-68b5-4c25-bab8-14936e9d8abb) |
| Supraland Six Inches Under | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/b865f172-e46b-423d-9df4-cf07cdbbb273) |
| System Shock (2023) |  ✔️  |  | DLSS | Launch with `-dx12` | [1](https://github.com/user-attachments/assets/17869a32-3289-4fb6-a394-9f278e67a699) |
| Tainted Grail: The Fall of Avalon | ✔️  | ✅ | DLSS | Recommended to use `-force-d3d12` as launch command to force Dx12, much better performance .DLSS to FSR3 and FSR4 tested successfully on Linux with minor instability issues. Steam launch options `FSR4_UPGRADE=1 %command% -force-d3d12` with GE-Proton-10-4 | [1](https://github.com/user-attachments/assets/ed1be669-f974-40f2-b0ab-6ef3237e6c99), [2](https://github.com/user-attachments/assets/89d5dc78-ec5d-44ff-998d-9aafbc29a403) <br>🐧[1](https://github.com/user-attachments/assets/7d3fb185-abc3-4746-b8e4-ef7d9a9651a9) |
| Tales of Kenzera: ZAU | ✔️ | | FSR2 | | [1](https://github.com/user-attachments/assets/bdde0aaf-9873-479b-842b-e89fd72d38d1)
| Tankhead | ✔️ | | DLSS, FSR3, XeSS | | [1](https://github.com/user-attachments/assets/7c1260b7-f5e2-4b61-b662-1a693336fd50)|
| Tchia | ✔️ |  | DLSS | Use DLSS. XeSS has severe graphical corruption| [1](https://github.com/user-attachments/assets/4864e75f-43ec-48b0-b340-f7d97290885b)|
| TEKKEN 8 | ✔️ | ✅ | DLSS, FSR2, XeSS | Requires [Fakenvapi](Fakenvapi), hair is terrible on certain characters, the stage DESCENT INTO SUBCONSCIOUS shows severe graphical artifacts on using Optiscaler FSR4 on the second floor (seems to be either game or FSR4 specific). | [1](https://github.com/user-attachments/assets/02401cb5-2587-433b-94bb-f8c3c85317d2) [2](https://github.com/user-attachments/assets/e4cac364-caaa-449e-a3ce-7bf5d389516b) <br>🐧[1](https://github.com/user-attachments/assets/cf749c64-6104-422f-88b8-79a391f97953) [2](https://github.com/user-attachments/assets/585afd1b-14ce-423a-bdb9-839b28ff971c) [3](https://github.com/user-attachments/assets/60aada13-0641-4b8b-9406-78a8d21feda7) |
| Tempest Rising | ✔️ |  | FSR3, DLSS | May need to enable [Non-Linear sRGB Input](#Image-Quality) if there screen flashes |[1](https://github.com/user-attachments/assets/25ed12a2-253a-4e89-b6ce-516cbe5b7df8) [2](https://github.com/user-attachments/assets/345e2f7c-0da4-4329-a3b6-dfa6f93d5715) |
| Test Drive Unlimited Solar Crown | ✔️ |  | DLSS |  |[1](https://github.com/user-attachments/assets/c53f249b-93b6-4f93-8f6f-b5108b5a25de)|
| Testament : The Order of High Human | ✔️ | | DLSS | Create shortcut with `-dx12` launch parameter | [1](https://github.com/user-attachments/assets/d42c35bd-809c-4da1-8af1-6693f2513f89)| 
| The Alters| ✔️ |  | DLSS | **Officially supported game**. Enabling FSR3 in-game not detected by OptiScaler. DLSS working. | [1](https://github.com/user-attachments/assets/59b05b0a-d791-40b3-b56c-901f9dd17ead) |
| The Ascent | ✔️ |  | DLSS | Launch game with `-dx12` | [1](https://github.com/user-attachments/assets/060f0657-5c45-4cc3-a379-386b39b464a9) |
| The Axis Unseen | ✔️ | | DLSS, FSR3, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/d2d848fc-9af0-4b5a-ae22-a268b02308a8) |
| The Chant | ✔️ |  | DLSS | Use DLSS input, FSR2 has some shimmer. | [1](https://github.com/user-attachments/assets/b3b9a19b-47c6-4fe5-b8ee-4b6dc7e6bd99)|
| The Casting of Frank Stone | ✔️ | ✅ | DLSS, FSR3, XeSS |  For Linux use OptiScaler as `winmm.dll` | [1](https://github.com/user-attachments/assets/e20481b6-0268-4714-8a85-5e2efe90cf52)  <br>🐧[1](https://github.com/user-attachments/assets/ee8be311-c946-4277-a238-88bad866d1d4) |
| The Elder Scrolls IV: Oblivion Remastered | ✔️ | ✅ | DLSS, FSR3, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/910eba1e-451d-410a-9dd5-3295684747b6) [2](https://github.com/user-attachments/assets/54469105-b56c-4f32-b193-5b1976e2d6b0) <br>🐧[1](https://github.com/user-attachments/assets/e68b5965-a459-4215-bd3a-877b636123be) [2](https://github.com/user-attachments/assets/c3d0cafb-6804-4139-be79-3b0bbd18bdb7) [3](https://github.com/user-attachments/assets/a262eae7-d999-434c-878b-64558c515f48) |
| The First Berserker: Khazan |  ✔️  |  | DLSS | **Officially supported game**. Requires [Fakenvapi](Fakenvapi). | [1](https://github.com/user-attachments/assets/d72a50f5-848c-467e-9857-232a0e72f3be) |
| The Invincible | ✔️ | ✅ | FSR2, DLSS | For DLSS Inputs please refer to [UE Tweaks](https://github.com/optiscaler/OptiScaler/wiki/Unreal-Engine-Tweaks) | [1](https://github.com/user-attachments/assets/e3389d1a-6fd2-4e9b-93fd-c20c7a7c3a80) [2](https://github.com/user-attachments/assets/9ca9c9cc-740e-4753-b7ed-0fd03b22d1dc) |
| The Last of Us Part I | ✔️ |  | DLSS, FSR3 | **Officially supported game**. Use [Fakenvapi](Fakenvapi). | [1](https://github.com/user-attachments/assets/751828bf-fb95-4bdf-b451-bb59d560c1d2) [2](https://github.com/user-attachments/assets/0ad76971-9ece-4f54-878e-b79a5a306c18) |
| The Last Oricru | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/fe94d516-659e-455f-acbf-b91eb8b5c315)|
| The Lord of the Rings: Gollum | ✔️| | DLSS | | [1](https://github.com/user-attachments/assets/170bf3e5-8f14-49c1-bda7-292ba95204d0) |
| The Medium | ✔️ |  | DLSS | Run in Dx12 mode | [1](https://github.com/user-attachments/assets/c4b16935-68d9-4c5c-bdd7-1f5af3bc13d2) |
| The Orville – Interactive Fan Experience | ✔️ | |  DLSS | Launch with Dx12 | [1](https://github.com/user-attachments/assets/42236d63-3a26-4346-ad31-dd4fe5036998) |
| The Outer Worlds Spacer Choice Edition | ✔️ | ✅  | FSR2 |   | 🐧[1](https://github.com/user-attachments/assets/a98f92a9-0a31-4d42-a4bb-be01a9eb4ed0) |
| The Rift | | ✅ | DLSS | (Demo) | 🐧[1](https://github.com/user-attachments/assets/a2987382-7678-4567-9e75-af50f8a899cc) |
| The Riftbreaker | ✔️ | | XeSS | | [1](https://github.com/user-attachments/assets/71eb832a-7d0b-4741-a198-52bcd009485b)|
| The Sinking City Remastered | ✔️ | ✅ | DLSS, FSR |  | [1](https://github.com/user-attachments/assets/26fbacb4-236d-422d-9b81-b383b432209b) [2](https://github.com/user-attachments/assets/11a84923-5bb0-4003-a1bc-1c4a33816784) [3](https://github.com/user-attachments/assets/a2fe9f50-39e2-4aed-ac5e-66729c7e7976) <br>🐧[1](https://github.com/user-attachments/assets/770d05ed-8e55-4cc2-8132-217336391a5e) |
| The Talos Principle: Reawakened |  ✔️  |  | FSR3, DLSS, XeSS  | **Officially supported game** | [1](https://github.com/user-attachments/assets/c54eba82-05c8-4570-b84d-7b14314be8fc)|
| [The Talos Principle 2](https://github.com/optiscaler/OptiScaler/wiki/The-Talos-Principle-2) |  ✔️  |  | FSR3  | XeSS & DLSS cause graphics problems  |
| The Thaumaturge |  ✔️  |  | DLSS, XeSS | FSR input cause crashing. Requires [Non-Linear sRGB Input](#Image-Quality) or [Non-Linear PQ Input](#Image-Quality) to fix gray screen during dialogue. [Non-Linear sRGB Input](#Image-Quality) provides better IQ and less ghosting. | [1](https://github.com/user-attachments/assets/19cdd10a-25d0-436d-8bfd-58420591fd33) |
| The Witcher 3 Next Gen |  ✔️  | ✅ | DLSS, FSR2, XeSS | DLSS-FG working very well also. Linux RDNA3 tested with FSR3.1, FSR4 & FG. | [1](https://github.com/user-attachments/assets/fdda1efd-5f81-4551-9df0-b215175cb91c) [2](https://github.com/user-attachments/assets/dde66cbf-a54f-46d0-9a78-05908dfe7803) <br /> 🐧[1](https://github.com/user-attachments/assets/743500cd-3b92-4d3d-be8d-79415d53cf78) |
| Time Breaker | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/802739e0-a700-4c9f-9eef-5e9555c151bc)|
| [Tiny Tina’s Wonderlands](https://github.com/optiscaler/OptiScaler/wiki/Tiny-Tinas-Wonderlands) | ✔️ | ✅ | FSR2  |   |  🐧[1](https://github.com/user-attachments/assets/b4d40542-00b7-4542-a0ec-a5bfc5c65104) |
| Titan Station | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/c25a8c87-ad7b-4c46-b9b9-13d921b8844e)|
| Titan Quest II |  ✔️  |  |  DLSS, FSR3.1  |  FSR FG can be activated with [Nukem’s](Nukem's-dlssg‐to‐fsr3)  | [1](https://github.com/user-attachments/assets/c235dc55-abed-4d7a-9195-dbcbe1af5b2b)   |
| Togges | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/1301e2c9-20cd-459d-9a47-b5ac4b1d2dd9)|
| Tokyo Xtreme Racer |  ✔️  | |  DLSS  |   |
| [Tony Hawk's Pro Skater 3 + 4](https://github.com/optiscaler/OptiScaler/wiki/Tony-Hawk's-Pro-Skater-3-4)  | ✔️ |  | DLSS  | DX11 game - check [UE DX11 note](#fsr4-shimmering-on-unreal-engine-dx11)  |  [1](https://github.com/user-attachments/assets/bf564780-a5d7-4983-af71-107aae6cb8fc) |
| Trail Out |  ✔️  |  | DLSS |  | [1](https://github.com/user-attachments/assets/98dcea97-626e-427a-8038-f6aba37fcf46) |
| Trepang2 | ✔️ |  | DLSS | Recommended to use `-dx12` as launch command to force Dx12. Game tends to reset the preset to „Performance“ every time it is launched, despite of the preset specified in video settings (can be checked in Opti overlay). In this case, it is sufficient to switch the preset back and forth. | [1](https://github.com/user-attachments/assets/40c5cec5-3726-4030-b9e4-f5add2e4276e) [2](https://github.com/user-attachments/assets/ea913ae0-91c7-4b74-bad2-880924791c5c) |
| Troublemaker 2 - Beyond Dream | ✔️ | | FSR2 | | [1](https://github.com/user-attachments/assets/6b2751ac-7257-4df7-8967-3703133fe3e1) |
| Turbo Sloths | ✔️ |  | DLSS | | [1](https://github.com/user-attachments/assets/d45faad1-f2a7-44db-a688-e4ebbc32cce4)|
| Twin Stones: The Journey of Bukka | ✔️ | | DLSS | Launch with `-dx12` paramter | [1](https://github.com/user-attachments/assets/c044b1db-babc-4450-98b7-1d01b8a3e2bd)|
| [UNCHARTED: Legacy of Thieves Collection](https://github.com/optiscaler/OptiScaler/wiki/UNCHARTED-Legacy-of-Thieves-Collection) |  ✔️  | |  DLSS, FSR2  |  | [1](https://github.com/user-attachments/assets/d13cd1ae-9e40-4557-8f89-e6d87b668e07) [2](https://github.com/user-attachments/assets/f62ba970-44e3-42d1-b6a3-24da14a18541) |
| Unholy | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/db10b55f-c66d-4034-ab0d-de4a07d08a38)  [2](https://github.com/user-attachments/assets/ca5127ff-aa46-4c65-ae84-ac358d221af6)|
| Until Dawn | ✔️ | | DLSS | **Officially supported game**. Enable [Non-Linear sRGB Input](#Image-Quality) to fix flicker in some scenes. FSR3 crashes to desktop and will prevent loading of game unless `Fsr3=false` is set in OptiScaler.ini. | [1](https://github.com/user-attachments/assets/371976fb-a235-4920-a89c-138348144cdc)|
| Valkyrie Elysium | ✔️ | |  DLSS |     | [1](https://github.com/user-attachments/assets/0bedfb79-15a1-4705-9311-6182f93b4d6f) |
| Vindictus: Defying Fate | ✔️ |  | DLSS | Choosing FSR in game will crash the game (Windows 11) |[1](https://github.com/user-attachments/assets/5c6193db-7c22-4672-b657-5b55008c3787) | 
| Visions of Mana |  ✔️  |  | FSR2 | Check the FSR2 pattern matching under inputs or set `Fsr2Pattern=true` in OptiScaler.ini |   |
| VLADiK BRUTAL|  ✔️  | ✅ | DLSS |  |   |
| VOID/BREAKER | ✔️ | ✅ | DLSS |  | [1](https://github.com/user-attachments/assets/318fdee5-4ca3-4f7b-a234-763698da0ff2) <br>🐧[1](https://github.com/user-attachments/assets/a2d4de05-6d98-4342-823e-8317406350e9) |
| Wanted: Dead | ✔️ | |  DLSS |  | [1](https://github.com/user-attachments/assets/9b7ba1a1-f3c8-41a5-8617-a909a78dac5b)|
| Warhammer 40,000: Darktide | ✔️ |  | FSR3, DLSS, XeSS | **Officially supported game** | [1](https://github.com/user-attachments/assets/9425d082-4300-4873-9874-6e0945bfb1ed) [2](https://github.com/user-attachments/assets/f8891c18-37c2-4907-a97e-4b2721f0faa7) [3](https://github.com/user-attachments/assets/fd780d3a-8334-4efd-aba3-5faaa65c95b1) |
| Watch Dogs: Legion |  ✔️  | ✅ |  DLSS | | [1](https://github.com/user-attachments/assets/461d2267-50f7-4483-919f-cdec2a93865e) <br>🐧[1](https://github.com/user-attachments/assets/2bada1be-2dd3-425b-ad7d-15a05140afaf) |
| [Way of the Hunter](https://github.com/optiscaler/OptiScaler/wiki/Way-of-the-Hunter) |  ✔️  |  | DLSS, FSR2 | Run in DX12 (even FSR2 works), but FSR2 inputs crash the game in DX11 - `EnableFsr2Inputs=false`, DX11 mode requires [UE DX11 note](#fsr4-shimmering-on-unreal-engine-dx11) | [1](https://github.com/user-attachments/assets/c07c4966-0dd8-451c-a840-cdc1fcb09a46) |
| Wayfinder |  ✔️  |  |  DLSS | | [1](https://github.com/user-attachments/assets/2f65e484-dccc-481f-9311-e0810984342e) |
| Welcome to ParadiZe | ✔️ |  | FSR2 |  | [1](https://github.com/user-attachments/assets/08fd6925-1a9c-4f2b-ab37-cefffdfd1a44) |
| [Wild Hearts](https://github.com/optiscaler/OptiScaler/wiki/Wild-Hearts) |  ✔️  | ✅ | DLSS | Works on Linux with WIP FSR4 support as `winmm.dll`| [1](https://github.com/user-attachments/assets/092dd1f1-68a4-4a73-afa5-60d479600e68) <br>🐧[1](https://github.com/user-attachments/assets/20e5cda4-d1f5-4919-89e3-4594f022704b) |
| Witchfire |  ✔️  |  | DLSS |   |   |
| Wo Long: Fallen Dynasty | ✔️ | ✅ | DLSS, XeSS  | | [1](https://github.com/user-attachments/assets/3ea4f2fa-04e8-41f8-8e9e-9629f3017298) <br>🐧[1](https://github.com/user-attachments/assets/de32bf22-4af1-4645-a281-d64979b09051) | 
| Wolfenstein: Youngblood |  ❌  |  | DLSS | Vulkan – Optiscaler Nvidia GPU spoofing can’t expose ray tracing option on AMD/Intel – game uses early draft Nvidia Vulkan RT extensions |  |
| World War Z |  ❌  |  | FSR2 | Vulkan |   |
| WRC Generations | ✔️ |  | DLSS | [Fakenvapi](Fakenvapi) is required | [1](https://github.com/user-attachments/assets/3629b43e-f056-4292-975e-732e67110523) |
| Wreckfest 2 | ✔️ |  | DLSS, FSR3 | **Officially supported game**. Update brought FSR/DLSS upscaling. | [1](https://github.com/user-attachments/assets/df03e183-8594-4a1b-9d0b-03c55aa488ec) |
| Wrench | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/4acd382f-bd2f-4269-bcbf-3981c030ed04)|
| [WUCHANG: Fallen Feathers](https://github.com/optiscaler/OptiScaler/wiki/WUCHANG-Fallen-Feathers) | ✔️ |  | FSR3.1 | **Officially supported game**. | [1](https://github.com/user-attachments/assets/3c41e795-5959-424d-abf3-dae58882f501) |
| [Wuthering Waves](https://github.com/optiscaler/OptiScaler/wiki/Wuthering-Waves) | ✔️ |  | DLSS, FSR3.1 | Works with `dxgi.dll ` or `winmm.dll ` in latest game update, launch game from `Client-Win64-Shipping.exe`. | [1](https://github.com/user-attachments/assets/9179f590-c6fb-471d-b66b-66f08169349e) |
| Xuan-Yuan Sword VII | ✔️ | | DLSS | Use Dx12 mode |[1](https://github.com/user-attachments/assets/98b47db6-47fc-4b96-92c9-c7f3cea29f98) |
| Zoochosis | ✔️ | | DLSS | | [1](https://github.com/user-attachments/assets/6d1c9ade-c178-4879-979f-28c10cbffd60) |