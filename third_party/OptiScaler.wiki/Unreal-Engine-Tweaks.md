>[!TIP]
> To locate the game's `Engine.ini`, the easiest way is to go to **PCGW** and look at the **`Configuration file(s) location`** section.

---

#### Enable `OutputScaling` and `XeFG`
Edit `Engine.ini` and add
```ini
[SystemSettings]
r.NGX.DLSS.DilateMotionVectors=0
r.Streamline.DilateMotionVectors=0
```

#### When using FSR2 inputs and the game is crashing
Edit `Engine.ini` and add
```ini
[SystemSettings]
r.FidelityFX.FSR2.UseNativeDX12=1
```

#### When using FSR3 inputs and the game is crashing
Edit `Engine.ini` and add
```ini
[SystemSettings]
r.FidelityFX.FSR3.UseNativeDX12=1
```

#### When using FSR3.1 inputs and the game is crashing
Edit `Engine.ini` and add
```ini
[SystemSettings]
r.FidelityFX.FSR3.UseNativeDX12=1
r.FidelityFX.FSR3.UseRHI=0