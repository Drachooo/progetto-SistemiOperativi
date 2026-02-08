# Progetto Sistemi Operativi: Calcolatore SHA-256 Concorrente

**Autore:** Matteo Drago  
**Matricola:** VR500241  
**Anno Accademico:** 2025/2026  
**Corso:** Sistemi Operativi - (Opzione 2: Pthread e FIFO)

---

## 📋 Descrizione del Progetto

Questo progetto implementa un sistema **Client-Server** per il calcolo concorrente di impronte digitali (Hash SHA-256) di file multipli. Il sistema è progettato per massimizzare il throughput e minimizzare la latenza utilizzando primitive di sincronizzazione POSIX e comunicazione IPC.

### 🚀 Caratteristiche Principali

* **Architettura Multi-thread:** Utilizzo di un **Thread Pool** con 6 worker fissi per gestire le richieste senza sovraccaricare il sistema (prevenzione del thrashing).
* **Comunicazione IPC:** Scambio dati tramite **Named Pipes (FIFO)** bidirezionali.
* **Scheduling Intelligente:** Implementazione dell'algoritmo **SJF (Shortest Job First)** per privilegiare i file più piccoli e ridurre il tempo medio di attesa.
* **Caching & Coalescing:**
    * **Cache Hit:** Memorizzazione dei risultati in una lista concatenata per fornire risposte immediate a richieste ripetute.
    * **Coalescing:** Gestione intelligente delle richieste simultanee per lo stesso file: il calcolo viene eseguito una sola volta e il risultato distribuito a tutti i richiedenti.
* **Sicurezza:** Prevenzione di Deadlock tramite Lock Ordering e gestione sicura della memoria.

---

## 🛠️ Istruzioni per l'Esecuzione

Gli eseguibili del progetto si trovano nella directory `project/build`. Assicurarsi di trovarsi in un ambiente Linux/Unix.

### 🔨 Compilazione
Per compilare il progetto e generare gli eseguibili nella cartella `build`:
```bash
make
```

### 1. Avvio del Server
Il server deve essere avviato per primo. Si occuperà di creare le FIFO pubbliche e inizializzare il pool di thread.

```bash
# Spostarsi nella directory degli eseguibili
cd project/build

# Avviare il server
./server
```

### 2. Utilizzo del Client (Calcolo Hash)
In un nuovo terminale, utilizzare il client per inviare al server il percorso di un file da processare.

```bash
# Spostarsi nella directory degli eseguibili
cd project/build

# Sintassi: ./client <percorso_file>
./client block_1.bin
```

### 3. Interrogazione della Cache (Query)
È possibile interrogare lo stato del server per visualizzare l'elenco dei file già elaborati e presenti in memoria.

```bash
# Utilizzare il flag -q per la modalità query
./client -q
```
