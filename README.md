# SpatialMix Pro — VST3 Atmos Spatial Simulator

**64-bit VST3** | **4K UI (16:9)** | **HRTF Binaural + Stereo + 2.1**

> Crea musica in ambiente Dolby Atmos-style direttamente in FL Studio,
> senza impianto surround — simula tutto in cuffia o 2.1.

---

## ✅ Come ottenere il .vst3 SENZA installare nulla

### Passo 1 — Crea un account GitHub gratuito
Vai su [github.com](https://github.com) e registrati (gratis).

### Passo 2 — Crea un nuovo repository
1. Clicca **"New repository"**
2. Nome: `SpatialMixPro`
3. Visibilità: **Public** (necessario per Actions gratuiti)
4. Clicca **"Create repository"**

### Passo 3 — Carica i file
Usa il pulsante **"Upload files"** su GitHub e carica TUTTI i file
di questa cartella mantenendo la struttura (Source/, .github/, ecc.).

Oppure con Git (se lo hai):
```bash
git clone https://github.com/TUO_USERNAME/SpatialMixPro.git
# copia i file dentro
git add .
git commit -m "Initial commit"
git push
```

### Passo 4 — GitHub compila automaticamente
Dopo il push, vai su **Actions** nel tuo repository.
Vedrai la build in corso (dura ~15-25 min la prima volta, ~5 min dopo).

### Passo 5 — Scarica il VST3
Quando la build è verde ✅, clicca sul workflow → scorri in fondo →
trovi **"SpatialMixPro-Win64-VST3"** → **Download**.

### Passo 6 — Installa in FL Studio
1. Estrai lo zip scaricato
2. Copia `SpatialMixPro.vst3` in:
   ```
   C:\Program Files\Common Files\VST3\
   ```
3. In FL Studio: **Options → Manage plugins → Scan** (seleziona VST3)
4. Cerca "SpatialMix" nel browser plugin

---

## 🎛️ Come si usa

### Workflow consigliato in FL Studio
Metti **un'istanza del plugin per ogni traccia** che vuoi posizionare
nello spazio (voce, chitarra, synth, ecc.).

```
[Mixer Channel 1: Voce]   → SpatialMix Pro (X:-1.5, Y:1.5, Z:-3)
[Mixer Channel 2: Basso]  → SpatialMix Pro (X:0,    Y:0.5, Z:-2)
[Mixer Channel 3: Pad]    → SpatialMix Pro (X:3,    Y:3,   Z:1)
[Master]                  → riceve tutto il mix binaurale
```

### Modalità output
| Modalità | Quando usarla |
|----------|---------------|
| **HRTF Binaural** | In cuffia — massima immersione 3D |
| **Stereo** | Monitor / diffusori stereo normali |
| **2.1 Sim** | Stereo + simulazione sub (per il tuo 2.1) |

### Controlli
- **Top View**: vista dall'alto della stanza — trascina il punto arancione per muovere la sorgente sul piano orizzontale (X/Z)
- **Elev View**: vista laterale — trascina per cambiare altezza e profondità (Y/Z)
- **Slider X**: sinistra/destra (-5m … +5m)
- **Slider Y**: altezza (0 … 5m)
- **Slider Z**: avanti/indietro (-5m … +5m)
- **Slider V**: volume della sorgente
- **Nome** (in alto a sinistra): doppio click per rinominare

### Casse di riferimento nel viewer
Le losanghe colorate sono i **canali bed Atmos** di riferimento visivo
(non sono uscite audio separate — servono per orientarsi nello spazio).

| Colore | Canali |
|--------|--------|
| 🔵 Blu | L, R (front stereo) |
| 🟢 Verde | C (center) |
| 🟠 Arancione | Ls, Rs (surround) |
| 🟣 Viola | Ltf, Rtf (top front) |
| 🩷 Rosa | Ltr, Rtr (top rear) |

---

## ⚙️ Tecnica

### Processing HRTF (Binaural mode)
- **Panning equal-power** dall'azimuth
- **ITD** (Interaural Time Delay): ritardo dell'orecchio controlaterale
  simulando la distanza testa–orecchio (215mm / 343 m/s)
- **High-shelf elevation filter**: suoni in alto hanno più alte frequenze
  (approssimazione delle HRTF pinnae cues)
- **Distance attenuation**: 1/distanza

### VST3 vs VST2
VST3 è più efficiente: quando il plugin non riceve audio, il motore
lo **sospende automaticamente** (suspend/resume). Zero CPU quando idle.

---

## 🔧 Build locale (opzionale)

Se vuoi compilare sul tuo PC:
```bash
# Requisiti: CMake 3.22+, Visual Studio 2022 (Windows) o Xcode (Mac)
cmake -B build -DCMAKE_BUILD_TYPE=Release -A x64
cmake --build build --config Release --parallel
```

Il VST3 finito si trova in:
```
build/SpatialMixPro_artefacts/Release/VST3/SpatialMixPro.vst3
```
