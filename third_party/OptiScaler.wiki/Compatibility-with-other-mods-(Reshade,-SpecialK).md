## Renaming Optiscaler to other supported filenames

* Optiscaler supports various names - list of supported ones - [link](https://github.com/optiscaler/OptiScaler/wiki/Automated-Installation#optiscaler-supports-these-filenames)  
* If another mod is already using `dxgi.dll` for example, you can try renaming Optiscaler to another name, like `winmm.dll`  
* Depending on the game and mods in question, not all names will work  


## General "plugins" folder method

If there is another mod (e.g. **Reshade** etc.) that uses the same filename (e.g. `dxgi.dll`), you can create a new folder called `plugins` where you can put other mod files. OptiScaler will check this folder and if it finds the same dll file (for example `dxgi.dll`), it will load that file instead of the original library.  

> [!TIP]
> * Above `plugins` folder method also works for loading **ASI plugins**  
> * Requires setting `LoadAsiPlugins=true` in **Optiscaler.ini**

![plugins](https://github.com/optiscaler/OptiScaler/assets/35529761/c4bf2a85-107b-49ac-b002-59d00fd06982)

## Reshade
Another option for **Reshade** 

**1.** Rename **Reshade dll** (usually `dxgi.dll`) to `ReShade64.dll` and put it next to Optiscaler  
**2.** Set `LoadReshade=true` in **OptiScaler.ini**  

If it's working successfully, you should see the Reshade boot notification.

![reshade64](https://github.com/user-attachments/assets/3d21a36d-a86d-418d-9a03-1e8b53cb3f6c)


## SpecialK

### **Option 1** - launching the game through SpecialK GUI (SKIF) 

**1.** Install OptiScaler as usual  
**2.** Launch the game through SpecialK GUI (SKIF)  

If it's working successfully, you should see the SK boot notification.

---

### **Option 2** - using the Optiscaler `plugins` method above

**1.** Open SK GUI (SKIF)  
**2.** Click the `Config folder-Centralized` option (this will open the install folder)  
**3.** Copy `SpecialK64.dll`  
**4.** Go to your game's folder where you installed Optiscaler  
**5.** Create a `plugins` folder  
**6.** Paste the SK DLL inside and rename it so Optiscaler and SK DLL have the **same name** (`dxgi.dll` in this example).  

If it's working successfully, you should see the SK boot notification.

_Screenshots below for illustration._


![SKplugins](https://github.com/user-attachments/assets/b41b4743-079c-4f34-a3ea-cf11f4d332c9)
![SKplugins2](https://github.com/user-attachments/assets/4ca8e772-234f-4937-a37e-5288c0f89479)

---

### **Option 3** - using LoadSpecialK option in **Optiscaler.ini**

> [!NOTE]
> _**This method will not work with OptiFG!**_

1. Find the `SpecialK64.dll` (check option 2 above)  
2. Paste the SK DLL inside the game folder where you installed Optiscaler  
3. Either create a **copy** of the SK DLL and rename it to **SpecialK.dxgi** (_**dxgi being the extension**_)  
3a. OR create a **new file** and name it **SpecialK.dxgi** (_**dxgi being the extension**_)  
4. Now you should have **2 files** (`SpecialK64.dll` and `SpecialK.dxgi`)  
5. Open **Optiscaler.ini** and set `LoadSpecialK=true`.  

If it's working successfully, you should see the SK boot notification.

![loadSK](https://github.com/user-attachments/assets/a0657143-6e9e-4002-b3bc-5a8d3781f806)

